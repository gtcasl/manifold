#ifndef EACORE_H
#define EACORE_H

#include <limits.h>
#include "kernel/manifold.h"
#include "kernel/component.h"
#include "uarch/networkPacket.h"
#include "cache.h"
#include "utils.h"

namespace manifold {
namespace pagevault {

class EACoreBase;
class Controller;

struct cc_cfg_t {
  uint32_t entries;
  uint32_t assoc;
  uint32_t cycles;
};

struct mc_cfg_t {
  uint32_t entries;
  uint32_t assoc;
  uint32_t cycles;
};

struct dev_cfg_t {
  uint32_t cycles;
  uint32_t ports; 
};

class IReqCompleteCB : public refcounted {
public:
  IReqCompleteCB() {}
  virtual ~IReqCompleteCB() {}
  virtual void onComplete(int tag, bool is_speculative) = 0;
};

template <typename T>
class TReqCompleteCB : public IReqCompleteCB {
private:
  typedef void (T::*Pfn)(int tag, bool is_speculative);
  T*  m_obj;
  Pfn m_pfn;
  
public:
  TReqCompleteCB(T * obj, Pfn pfn) : m_obj(obj), m_pfn(pfn) {}  
  ~TReqCompleteCB() {}
  
  void onComplete(int tag, bool is_speculative) override {
    (const_cast<T*>(m_obj)->*m_pfn)(tag, is_speculative);
  }
};

class IMemCompleteCB : public refcounted {
public:
  IMemCompleteCB() {}
  virtual ~IMemCompleteCB() {}
  virtual void onComplete(int tag) = 0;
};

template <typename T>
class TMemCompleteCB : public IMemCompleteCB {
private:
  typedef void (T::*Pfn)(int tag);
  T*  m_obj;
  Pfn m_pfn;
  
public:
  TMemCompleteCB(T * obj, Pfn pfn) : m_obj(obj), m_pfn(pfn) {}  
  ~TMemCompleteCB() {}
  
  void onComplete(int tag) override {
    (const_cast<T*>(m_obj)->*m_pfn)(tag);
  }
};

enum  EA_PARAMS {
  EA_PREFETCH_MASK = 0xffffffff,
  
  EA_LOG2_BLOCK_SIZE = 6, // 64 bytes per block
  EA_BLOCK_SIZE      = (1 << EA_LOG2_BLOCK_SIZE),
  EA_BLOCK_SIZE_MASK = EA_BLOCK_SIZE - 1,
  
  EA_LOG2_BLOCKS_PER_PAGE = 6, // 64 blocks per page
  EA_BLOCKS_PER_PAGE      = (1 << EA_LOG2_BLOCKS_PER_PAGE),
  EA_BLOCKS_PER_PAGE_MASK = EA_BLOCKS_PER_PAGE - 1,
  
  AES_CALLS_PER_BLOCK  = 4, // = 64B / 128b
  HASH_CALLS_PER_BLOCK = 4, // = 64B / 128b
};

enum EA_DEVICE { 
  EA_MEM_DEV,         // user blocks request  device
  EA_MAC_DEV,         // MAC blocks request device
  EA_HTMEM_DEV,       // hash tree blocks request device
  EA_CTR_DEV,         // block counter device
  EA_HASH_DEV,        // block hash generator
  EA_AES_DEV,         // AES engine
  EA_XOR_DEV,         // XOR operator
  EA_SEED_DEV,        // SEED generator
  EA_HTLIST_DEV,      // hash tree MAC list generator
  EA_HTHASH_DEV,      // hash tree hash device
  EA_AUTH_DEV,        // hash tree MAC authentication
  EA_COMMIT_DEV,      // commit unit
  EA_PART_DEV,        // partition generator    
};

enum EA_STAGE {
  EA_MEM_REQ      = 1 << 0,   // fetch a user block from memory
  EA_CTR_GEN      = 1 << 1,   // lookup a counter from on-chip cache
  EA_SEED_GEN     = 1 << 2,   // generate a unique seed using counter value and block address
  EA_PAD_GEN      = 1 << 3,   // generate unique pad by encrypting unique seed using AES with private key
  EA_ENCRYPT      = 1 << 4,   // encrypt user block by XOR-ing with unique pad
  EA_DECRYPT      = 1 << 5,   // decrypt user block by XOR-ing with unique pad
  EA_HASH_GEN     = 1 << 6,   // compute user block hash value
  EA_HT_LIST      = 1 << 7,   // generate the list of memory blocks containing hashes on the user block hash tree path
  EA_HT_RD        = 1 << 8,   // fetch all memory blocks containing hashes on the user block hash tree path
  EA_HT_WR        = 1 << 9,   // update all memory blocks containing hashes on the user block hash tree path
  EA_HT_HASH_RD   = 1 << 10,  // generate the hash of all parent nodes on the user block hash tree path (for verification)
  EA_HT_HASH_WR   = 1 << 11,  // generate the hash of all parent nodes on the user block hash tree path (for update)
  EA_HT_AUTH      = 1 << 12,  // authenticate hash tree
  EA_BLOCK_AUTH   = 1 << 13,  // authenticate user block
  EA_MAC_REQ      = 1 << 14,  // fetch the user block's MAC block from memory
  EA_PMT_REQ      = 1 << 15,  // PMT partitioning
  EA_COMMIT0      = 1 << 16,  // early commit (skip hash tree authentication)
  EA_COMMIT       = 1 << 17,  // final commit    
};

enum EA_BLOCK_TYPE {
  EA_USER_BLOCK  = 1 << 0, // user block
  EA_CTR_BLOCK   = 1 << 1, // counter block
  EA_MAC_BLOCK   = 1 << 2, // MAC block
  EA_HT_BLOCK    = 1 << 3, // Hashtree block
  EA_EVICT_BLOCK = 1 << 4, // evicted clean user block 
};

const char* toString(EA_STAGE stage);

const char* toString(EA_BLOCK_TYPE dt);

const char* toRWString(bool is_read);

struct ea_stage_t {
  EA_STAGE  id;
  EA_DEVICE device; // the compute device associated with the stage    
  uint32_t  dependencies; // list of stages that should be completed before running this stage
  uint32_t  per_node_deps; // per node dependencies
  uint32_t  dt_types; // list of data types that apply to the stage   
};

class IMemLayout {
public:
  IMemLayout() {}
  virtual ~IMemLayout() {}
  
  // return the block address of specified user block
  virtual uint64_t get_user_mac_addr(uint32_t block_idx) = 0;
  
  // return the block address of specified counter
  virtual uint64_t get_ctr_addr(uint32_t ctr_idx) = 0;
  
  // return the block address of specified hash tree node
  virtual uint64_t get_ht_mac_addr(uint32_t node_idx) = 0;
};

struct ea_ht_node_t {
  ea_ht_node_t(uint32_t idx, uint32_t blockid, ea_ht_node_t* parent) 
    : idx(idx)
    , blockid(blockid)
    , parent(parent)    
    , stages_completed(0)
    , stages_pending(0)
    , fetched_childnodes(0)
  {}
  
  uint32_t idx;
  uint32_t blockid;
  ea_ht_node_t* parent;
  uint32_t stages_completed;
  uint32_t stages_pending;
  uint32_t fetched_childnodes;
  std::vector<ea_ht_node_t*> child_nodes;
};

struct memreq_t {
  memreq_t(uint64_t addr, bool is_read, bool is_evict, uint32_t onchip_mask,
           int src_nodeid, int src_port, 
           uint64_t cycle)
    : addr(addr)
    , is_read(is_read)
    , is_evict(is_evict)
    , onchip_mask(onchip_mask)
    , src_nodeid(src_nodeid)
    , src_port(src_port)
    , cycle(cycle) 
  {}
  
  memreq_t(const memreq_t& rhs) 
    : addr(rhs.addr)
    , is_read(rhs.is_read)
    , is_evict(rhs.is_evict)
    , onchip_mask(rhs.onchip_mask)
    , src_nodeid(rhs.src_nodeid)
    , src_port(rhs.src_port)
    , cycle(rhs.cycle) 
  {}
  
  bool is_prefetch() const {
    return (EA_PREFETCH_MASK == onchip_mask); 
  }
  
  uint64_t addr;
  bool is_read;
  bool is_evict;
  uint32_t onchip_mask;
  int src_nodeid;
  int src_port;
  uint64_t cycle;
};

typedef nlohmann::fifo_map<uint64_t, std::vector<ea_ht_node_t*>> ht_map;

struct ea_mrq_t : refcounted {
  ea_mrq_t(uint64_t mem_addr_,
              int tag_, 
              EA_BLOCK_TYPE dt_type_,
              bool is_read_,
              uint32_t onchip_mask,
              int src_nodeid,
              IReqCompleteCB* cb_,
              ea_mrq_t* parent_) 
    : mem_addr(mem_addr_)
    , tag(tag_)
    , dt_type(dt_type_)
    , is_read(is_read_)
    , onchip_mask(onchip_mask)
    , src_nodeid(src_nodeid)
    , cb(cb_)    
    , stages_completed(0) 
    , stages_pending(0) {
    if (parent_)
      parent_->add_ref();
    parent = parent_;
  }  
  
  ~ea_mrq_t() {
    if (parent)
      parent->release();
  }
  
  uint64_t mem_addr;
  int tag;
  EA_BLOCK_TYPE dt_type;
  bool is_read; 
  uint32_t onchip_mask;
  int src_nodeid;
  IReqCompleteCB* cb;
  ea_mrq_t* parent;
  uint32_t stages_completed;
  uint32_t stages_pending;  
  std::vector<ea_ht_node_t*> ht_pnode_list;  
  ht_map ht_update_list; 
  ht_map ht_fetch_list;         
  std::map<int, uint64_t> ht_blocks;
  std::vector<uint64_t> part_list;
};

struct ctr_t {  
  enum {
    MAX_MINOR_VALUE = 128,
  };
  uint8_t  minors[64]; 
  uint64_t major;
  int log2_psize;
  
  void reset(int log2_psize_) {    
    memset(minors, 0, sizeof(minors));
    major = 0;
    log2_psize = log2_psize_;
  }
  
  bool incr_minor(uint32_t index) {
    assert(index < __countof(minors));
    return (++minors[index] < MAX_MINOR_VALUE);
  }
  
  void incr_major() {    
    memset(minors, 0, sizeof(minors));
    ++major;
  }
};

class IEADevice {
public:
  IEADevice() {}
  virtual ~IEADevice() {}
  
  virtual bool does_memory_access() { return false; }
  virtual bool submit(ea_mrq_t* mrq, EA_STAGE stage) = 0;
  virtual void tick() = 0;
  virtual void print_stats(std::ostream& out) {}
};

class EADevice : public IEADevice {
public:  
  EADevice(const char* name, int cycles = 1, int ports = 1, int batch_size = 1);
  virtual ~EADevice();
  
  bool submit(ea_mrq_t* mrq, EA_STAGE stage) override;  
  void tick() override;  
  void print_stats(std::ostream& out) override;
  
protected:   
  
  struct task_t {    
    ea_mrq_t* mrq;
    EA_STAGE stage;
  };
  
  struct stats_t {
    stats_t() : invocations(0) {}
    uint64_t invocations;
  };
  
  void onComplete(const task_t& task);
  
  string m_name;
  int m_batch_size; 
  TClkDevice<task_t>* m_device;
  stats_t m_stats;
};

class EADevice0 : public IEADevice {
public:  
  EADevice0() : m_active_mrq(nullptr) {}
  
  bool submit(ea_mrq_t* mrq, EA_STAGE stage) override;  
  void tick() override;
  virtual bool tick(ea_mrq_t* mrq, EA_STAGE stage);  

protected:
  
  ea_mrq_t* m_active_mrq;
  EA_STAGE  m_active_stage;    
};

class EAHTListGen : public EADevice0 {
public:  
  EAHTListGen(EACoreBase* eacore) : m_eacore(eacore) {}
  
  bool tick(ea_mrq_t* mrq, EA_STAGE stage) override;

protected:
  
  int get_blockid(ea_mrq_t* mrq, int nodeid);
  
  EACoreBase* m_eacore;
};

class EACtrGen : public EADevice0 {
public:  
  EACtrGen(EACoreBase* eacore, uint32_t entries, uint32_t assoc, uint32_t cycles);
  ~EACtrGen();
  
  bool does_memory_access() override { return true; }
  bool submit(ea_mrq_t* mrq, EA_STAGE stage) override;
  void tick() override;
  bool tick(ea_mrq_t* mrq, EA_STAGE stage) override;    
  void print_stats(std::ostream& out) override;
  
  uint32_t get_log2_macs_per_page(uint64_t addr);

protected:
  
  struct rqid_t {    
    ea_mrq_t* mrq;
    EA_STAGE  stage;
  };
  
  struct ctrrq_t {  
    uint64_t  addr;
    bool      is_read;
    uint32_t  onchip_mask;
    EA_BLOCK_TYPE dt_type;
    ea_mrq_t* mrq;    
    EA_STAGE  stage;
  };
  
  struct stats_t {
    stats_t() : hits(0), misses(0), evictions(0), overflows(0), invocations(0), total_cycles(0), bulk_reads(0) {}
    uint64_t hits;
    uint64_t misses;
    uint64_t evictions;
    uint64_t overflows;    
    uint64_t bulk_reads;
    uint64_t invocations;
    uint64_t total_cycles;
    std::map<int, uint64_t> start_cycles;
  };
  
  typedef TCache<rqid_t> CtrCache;
  typedef nlohmann::fifo_map<int, std::vector<ctrrq_t>> WaitQ;
  typedef std::map<uint64_t, ctr_t> CtrStore;
  
  void cache_complete_cb(const rqid_t& rqdi, uint64_t addr, bool is_hit);
  
  void ctr_request_complete_cb(int tag, bool is_speculative);
  
  void usr_request_complete_cb(int tag, bool is_speculative);
  
  bool update_ctr(const std::vector<ctrrq_t>& crqs);
  
  bool insert_ctr(const std::vector<ctrrq_t>& crqs);
  
  void release(int tag);
  
  void ensure_counter_store(uint64_t ctr_addr);
  
  EACoreBase*         m_eacore;
  CtrStore            m_ctrStore;
  CtrCache*           m_cache;
  std::list<ctrrq_t>  m_inputQ;
  WaitQ               m_waitQ; 
  uint32_t            m_pending_reads;
  uint32_t            m_pending_writes;
  std::map<int, int>  m_pending_ctr_rd_reqs;
  std::map<int, int>  m_pending_ctr_wr_reqs;
  std::map<int, int>  m_pending_usr_rd_reqs;
  std::map<int, int>  m_pending_usr_wr_reqs;
  IReqCompleteCB*     m_cb_ctr;
  IReqCompleteCB*     m_cb_usr;  
  stats_t             m_stats;  
};

class EAMacReq : public EADevice0 {
public:  
  EAMacReq(EACoreBase* eacore, uint32_t entries, uint32_t assoc, uint32_t cycles);
  ~EAMacReq();
  
  bool does_memory_access() override { return true; }  
  void tick() override;
  bool tick(ea_mrq_t* mrq, EA_STAGE stage) override;  
  void print_stats(std::ostream& out) override;

protected:
  
  struct rqid_t {    
    ea_mrq_t* mrq;
    EA_STAGE  stage;
  };
  
  struct mac_req_t {    
    uint64_t  addr;    
    bool      is_read; 
    ea_mrq_t* mrq;    
    EA_STAGE  stage;
  };
  
  struct stats_t {
    stats_t() : hits(0), misses(0), evictions(0), invocations(0), total_cycles(0) {}
    uint64_t hits;
    uint64_t misses;
    uint64_t evictions;
    uint64_t invocations;
    uint64_t total_cycles;
    std::map<int, uint64_t> start_cycles;
  };
  
  typedef TCache<rqid_t> MCache;
  typedef nlohmann::fifo_map<int, std::list<mac_req_t>> WaitQ;
  
  void cache_complete_cb(const rqid_t& rqid, uint64_t addr, bool is_hit);
  
  void mem_complete_cb(int tag);
  
  std::list<mac_req_t> m_inputQ;
  
  EACoreBase* m_eacore;
  MCache*     m_cache;
  WaitQ       m_waitQ; 
  IMemCompleteCB* m_mem_complete_cb;
  std::map<uint64_t, int> m_active_rd_blocks;
  std::map<uint64_t, int> m_active_wr_blocks;
  stats_t     m_stats;  
};

class EAMemReq : public EADevice0 {
public:  
  EAMemReq(EACoreBase* eacore);
  ~EAMemReq();
  
  bool does_memory_access() override { return true; }
  bool tick(ea_mrq_t* mrq, EA_STAGE stage) override;      
  void print_stats(std::ostream& out) override;

protected:
  
  struct stats_t {
    stats_t() : invocations(0), total_cycles(0) {}
    uint64_t invocations;
    uint64_t total_cycles;
    std::map<int, uint64_t> start_cycles;
  };
  
  void mem_complete_cb(int tag);
  
  EACoreBase* m_eacore;
  std::map<int, std::list<ea_mrq_t*>> m_waitQ;
  std::map<int, int> m_pending_reqs;
  std::map<int, int> m_completed_reqs;
  IMemCompleteCB* m_mem_complete_cb;
  stats_t m_stats;
};

class EAHtReq : public EADevice0 {
public:  
  EAHtReq(EACoreBase* eacore);
  ~EAHtReq();
  
  bool submit(ea_mrq_t* mrq, EA_STAGE stage) override;
  bool tick(ea_mrq_t* mrq, EA_STAGE stage) override;      
  void print_stats(std::ostream& out) override;

protected:
  
  struct task_t {
    ea_mrq_t* mrq;
    EA_STAGE  stage;
    uint64_t  addr;
    int       blockid;
  };
  
  struct stats_t {
    stats_t() : invocations(0), total_cycles(0) {}
    uint64_t invocations;
    uint64_t total_cycles;
    std::map<int, uint64_t> start_rd_cycles;
    std::map<int, uint64_t> start_wr_cycles;
  };
  
  typedef std::map<int, std::list<task_t>> WaitQ;
  typedef std::map<int, uint64_t> SratchBuffer;
  
  void mem_complete_cb(int tag);
  
  EACoreBase* m_eacore;
  WaitQ m_waitQ;
  SratchBuffer m_scratchbuffer;
  ht_map::iterator m_ht_fetch_iter;    
  ht_map::iterator m_ht_update_iter; 
  std::map<int, int> m_fetched_blocks;
  std::map<int, int> m_updated_blocks;
  uint32_t m_pending_reads;
  uint32_t m_pending_writes;
  IMemCompleteCB* m_mem_complete_cb;
  stats_t m_stats;
};

class HtHashGen : public IEADevice {
public:  
  HtHashGen(EACoreBase* eacore, int cycles, int ports, int batch_size);
  virtual ~HtHashGen();
  
  bool submit(ea_mrq_t* mrq, EA_STAGE stage) override;
  
  void tick() override {
    m_device->tick();
  }
  
  void print_stats(std::ostream& out) override;
  
protected:   
  
  struct task_t {    
    ea_mrq_t* mrq;
    EA_STAGE stage;
    ea_ht_node_t* pnode;
  };
  
  struct stats_t {
    stats_t() : invocations(0), total_cycles(0) {}
    uint64_t invocations;
    uint64_t total_cycles;
    std::map<int, uint64_t> start_rd_cycles;
    std::map<int, uint64_t> start_wr_cycles;
  };
  
  typedef std::map<int, uint64_t> SratchBuffer;
  
  void onComplete(const task_t& task);
  
  EACoreBase* m_eacore;
  TClkDevice<task_t>* m_device;
  SratchBuffer m_scratchbuffer;
  std::map<int, int> m_pending_rd_blocks;
  std::map<int, int> m_pending_wr_blocks;
  std::map<int, int> m_hashed_rd_blocks;
  std::map<int, int> m_hashed_wr_blocks;
  stats_t m_stats;
  int m_batch_size; 
};

class EAHtAuth : public EADevice0 {
public:  
  EAHtAuth(EACoreBase* eacore) : m_eacore(eacore) {}
  
  bool does_memory_access() override { return true; }
  bool submit(ea_mrq_t* mrq, EA_STAGE stage) override;
  bool tick(ea_mrq_t* mrq, EA_STAGE stage) override;

protected:
  std::map<int, int> m_pending_blocks;
  std::map<int, int> m_authenticated_blocks;
  EACoreBase* m_eacore;
  ea_ht_node_t* m_pnode;
};

class EACommit : public EADevice0 {
public:  
  EACommit(EACoreBase* eacore) : m_eacore(eacore) {}
  
  bool tick(ea_mrq_t* mrq, EA_STAGE stage) override;

protected:
  EACoreBase* m_eacore;
};

class EACoreBase : public kernel::Component {
public:  
  EACoreBase(Controller* ctrl,
             const std::vector<ea_stage_t>& rd_stages,
             const std::vector<ea_stage_t>& wb_stages);
  
  virtual ~EACoreBase();
  
  ktree_t* get_hashtree() const {
    return m_hashtree;
  }
  
  IMemLayout* get_mlayout() const {
    return m_mem_layout;
  }
  
  IEADevice* get_device(EA_DEVICE dev_id) const {
    return m_devices.at(dev_id);
  }
  
  uint64_t get_cycle();
  
  virtual uint32_t get_hashtree_nodeidx(ea_mrq_t* mreq) = 0;
  
  void tick();
  
  virtual int submit_request(
      uint64_t addr, 
      bool is_read,       
      uint32_t onchip_mask,
      EA_BLOCK_TYPE dt, 
      int src_nodeid,
      ea_mrq_t* parent,
      IReqCompleteCB* cb);
  
  virtual void release(ea_mrq_t* mrq);
  
  int submit_mem_request(uint64_t addr, bool is_read, EA_BLOCK_TYPE dt, int req, IMemCompleteCB* cb);   
  
  virtual void print_stats(std::ostream& out);  
  
protected:
  
  struct stats_t {
    stats_t() : max_queue_size(0), mem_footprint(0), queue_stalls(0) {}
    uint64_t max_queue_size;
    uint64_t mem_footprint;
    uint64_t queue_stalls;
  }; 
  
  int allocate_tag();
  
  Controller* m_ctrl;
  std::map<EA_DEVICE, IEADevice*> m_devices;
  std::vector<ea_stage_t> m_rd_stages;
  std::vector<ea_stage_t> m_wb_stages;  
  std::vector<ea_mrq_t*> m_mrqs;
  int m_queue_depth;   
  int m_mrq_tags;
  bool m_ea_bus_arbitration;
  bool m_mem_bus_arbitration;
  
  ktree_t* m_hashtree;
  IMemLayout* m_mem_layout;
  stats_t m_stats; 
};

}
}

#endif // EACORE_H
