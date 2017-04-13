#ifndef MANIFOLD_PAGEVAULT_CONTROLLER_H_
#define MANIFOLD_PAGEVAULT_CONTROLLER_H_

#include <map>
#include <list>

#include "kernel/manifold.h"
#include "kernel/component.h"
#include "uarch/networkPacket.h"
#include "uarch/DestMap.h"
#include "uarch/memMsg.h"

#include "dramsim2.h"
#include "hmcsim.h"
#include "eacore.h"

namespace manifold {
namespace pagevault {

struct config_t {  
  int mem_msg_type;
  int credit_msg_type;
  int downstream_credits; 
   
  string    type;
  string    mc_type;
  uint32_t  mem_capacity;
  uint64_t  sys_mem;
  uint32_t  mac_bits;  
  uint32_t  queue_depth;
  string    pmt_mode;
  uint32_t  pmt_size;
  bool      disable_evictions_handling;
  
  cc_cfg_t  cc;
  mc_cfg_t  mc;
  dev_cfg_t aes;
  dev_cfg_t sha1;  
  hmc_cfg_t hmc;
  dramsim2_cfg_t dramsim2;
};

config_t* get_config();

class Controller : public kernel::Component {
public:
  enum { 
    PORT0 = 0,        
  };
  
  Controller(int nid, kernel::Clock *clock);  
  ~Controller();
  
  void set_mc_map(manifold::uarch::DestMap *m);

  void handle_request(int, uarch::NetworkPacket *pkt);
  
  void tick();
  
  void print_config(std::ostream& out);  
  
  void print_stats(std::ostream& out);
  
private: 
  
  void checkDeadlock(uint64_t cycle);
  
  int submit_mem_request(uint64_t addr, bool is_read, EA_BLOCK_TYPE dt, int req, IMemCompleteCB* cb);
  
  enum {
    MEM_BLOCK_SIZE = 64,
    MEM_TAG_MAX = 0xffff,
  };
  
  struct llmemreq_t {
    llmemreq_t(uint64_t addr,
               bool is_read,
               EA_BLOCK_TYPE dt,
               int req,
               IMemCompleteCB* cb,
               int tag,
               uint64_t cycle) 
      : addr(addr)
      , is_read(is_read)
      , dt(dt)
      , req(req)
      , cb(cb)
      , tag(tag)
      , cycle(cycle) {}
    
    uint64_t addr; 
    bool is_read; 
    EA_BLOCK_TYPE dt;
    int req;
    IMemCompleteCB* cb;
    int tag;
    uint64_t cycle;    
  };
  
  struct stats_t {
     stats_t() 
       : read_latency(0)
       , write_latency(0) 
       , serviced_loads(0)
       , serviced_stores(0)
       , mc_read_latency(0)
       , mc_write_latency(0)
       , mc_serviced_loads(0)
       , mc_serviced_stores(0)
     {}
     
     std::map<int, unsigned> loads;
     std::map<int, unsigned> stores;
     uint64_t read_latency;
     uint64_t write_latency;
     uint64_t serviced_loads;
     uint64_t serviced_stores;
     std::map<EA_BLOCK_TYPE, uint64_t> mc_loads;
     std::map<EA_BLOCK_TYPE, uint64_t> mc_stores;    
     uint64_t mc_read_latency;
     uint64_t mc_write_latency; 
     uint64_t mc_serviced_loads;
     uint64_t mc_serviced_stores;
   };
  
  typedef std::map<int, llmemreq_t*> MemPQ;
  
  void schedule_request();
  
  void process_replies();
  
  void send_credit_downstream();
  
  void request_complete_cb(int tag, bool is_speculative);
  
  void mc_request_complete_cb(int tag);
  
  int m_nodeid; 
  
  manifold::uarch::DestMap* m_mc_map;
  
  IMemoryController* m_mc;
      
  EACoreBase* m_eacore;
  IReqCompleteCB* m_req_complete_cb;
  
  std::list<memreq_t*> m_incoming_reqs;
  std::map<int, std::vector<memreq_t*>> m_pending_reqs;
  std::list<memreq_t*> m_outgoing_replies;
  
  MemPQ m_mc_pending_reqs;    
  int m_downstream_credits;  
  stats_t m_stats;  
  
  friend class EACoreBase;
};

} // namespace pagevault
} // namespace manifold

#endif // MANIFOLD_PAGEVAULT_CONTROLLER_H_
