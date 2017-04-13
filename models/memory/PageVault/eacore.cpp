#include "eacore.h"
#include "utils.h"
#include "uarch/networkPacket.h"
#include "controller.h"
#include "gmt.h"
#include "bmt.h"
#include "pmt.h"
#include "sgx.h"

using namespace std;
using namespace manifold::kernel;
using namespace manifold::uarch;

namespace manifold {
namespace pagevault {

const char* toString(EA_STAGE stage) {
  switch (stage) {
  case EA_MEM_REQ: return "MEM_REQ";
  case EA_CTR_GEN: return "CTR_GEN";
  case EA_SEED_GEN: return "SEED_GEN";
  case EA_PAD_GEN: return "PAD_GEN";
  case EA_ENCRYPT: return "ENCRYPT";
  case EA_DECRYPT: return "DECRYPT";
  case EA_HASH_GEN: return "HASH_GEN";
  case EA_HT_LIST: return "HT_LIST";
  case EA_HT_RD: return "HT_RD";
  case EA_HT_WR: return "HR_RW";
  case EA_HT_HASH_RD: return "HT_HASH_RD";
  case EA_HT_HASH_WR: return "HT_HASH_WR";
  case EA_HT_AUTH: return "HT_AUTH";
  case EA_BLOCK_AUTH: return "BLOCK_AUTH";
  case EA_MAC_REQ: return "MAC_REQ";
  case EA_PMT_REQ: return "PMT_REQ";
  case EA_COMMIT0: return "COMMIT0";
  case EA_COMMIT: return "COMMIT";  
  }
  abort();
  return "";
}

const char* toString(EA_BLOCK_TYPE dt) {
  switch (dt) {
  case EA_USER_BLOCK: return "usr";
  case EA_CTR_BLOCK: return "ctr";
  case EA_MAC_BLOCK: return "mac";
  case EA_HT_BLOCK: return "ht";
  case EA_EVICT_BLOCK: return "evict";
  }
  abort();
  return "";
}

const char* toRWString(bool is_read) {
  return is_read ? "read" : "write";
}

///////////////////////////////////////////////////////////////////////////////

EADevice::EADevice(const char* name, int cycles, int ports, int batch_size) 
  : m_name(name)
  , m_batch_size(batch_size) {
  TClkDevice<task_t>::ICallback* cb = 
      new TClkDevice<task_t>::TCallback<EADevice>(this, &EADevice::onComplete);
  m_device = new TClkDevice<task_t>(cb, cycles, ports);
  cb->release();
}

EADevice::~EADevice() { 
  delete m_device; 
}

bool EADevice::submit(ea_mrq_t* mrq, EA_STAGE stage) {
  if (!m_device->submit({mrq, stage}, m_batch_size))
    return false;
  mrq->stages_pending |= stage;
  DBG(3, "*** stage %s pending for req#%d\n", toString(stage), mrq->tag);
  return true;
}

void EADevice::tick() {
  m_device->tick();
}

void EADevice::onComplete(const task_t& task) {
  task.mrq->stages_pending &= ~task.stage;
  task.mrq->stages_completed |= task.stage;
  DBG(3, "*** stage %s completed for req#%d\n", toString(task.stage), task.mrq->tag);
  m_stats.invocations += m_batch_size;
}

void EADevice::print_stats(std::ostream& out) {
  out << "  " << m_name << ": number of invocations = " << m_stats.invocations << endl;
  out << "  " << m_name << ": average latency = " << (m_device->get_cycles() + m_batch_size -1)  << " cycles" << endl;
}

///////////////////////////////////////////////////////////////////////////////

bool EADevice0::submit(ea_mrq_t* mrq, EA_STAGE stage) {
  if (m_active_mrq)
    return false;  
  m_active_mrq = mrq;
  m_active_stage = stage;
  mrq->stages_pending |= stage;
  DBG(3, "*** stage %s pending for req#%d\n", toString(stage), mrq->tag);
  return true;
}

void EADevice0::tick() {
  if (m_active_mrq) {
    if (tick(m_active_mrq, m_active_stage))
      m_active_mrq = nullptr;
  }
}

bool EADevice0::tick(ea_mrq_t* mrq, EA_STAGE stage) {
  mrq->stages_pending &= ~stage;
  mrq->stages_completed |= stage;
  DBG(3, "*** stage %s completed for req#%d\n", toString(stage), mrq->tag);
  return true;
}

///////////////////////////////////////////////////////////////////////////////

int EAHTListGen::get_blockid(ea_mrq_t* mrq, int nodeid) {
  int blockid = -1;
  if (nodeid != 0) {
    uint64_t addr = m_eacore->get_mlayout()->get_ht_mac_addr(nodeid);    
    auto iter = std::find_if(mrq->ht_blocks.begin(), mrq->ht_blocks.end(), 
        [addr](const std::map<int, uint64_t>::value_type& v)->bool { 
      return v.second == addr; 
    });
    if (iter != mrq->ht_blocks.end()) {
      blockid = iter->first;
    } else {
      blockid = mrq->ht_blocks.size();
      mrq->ht_blocks[blockid] = addr;
    }
  }
  return blockid;
}

bool EAHTListGen::tick(ea_mrq_t* mrq, EA_STAGE stage) {
  //
  // Build the MAC list of all hash tree nodes on the data block path 
  // excluding the root node (node_idx == 0).
  //
  // stage_latency = 1 cycle
  //
  uint32_t src_node_idx = m_eacore->get_hashtree_nodeidx(mrq);
  ea_ht_node_t* ppnode = nullptr;
  
  uint32_t parent_idx = src_node_idx;
  while (parent_idx != 0) {
    parent_idx = m_eacore->get_hashtree()->get_parent(parent_idx);
    
    int blockid = get_blockid(mrq, parent_idx);
    ea_ht_node_t* const pnode = new ea_ht_node_t(parent_idx, blockid, nullptr);
    mrq->ht_pnode_list.push_back(pnode);
    
    for (int i = 0; i < m_eacore->get_hashtree()->k; ++i) {
      // build the hash tree node list
      uint32_t node_idx = m_eacore->get_hashtree()->get_child(parent_idx, i);      
      ea_ht_node_t* node;
      if (ppnode 
       && ppnode->idx == node_idx) {
        node = ppnode;
        node->parent = pnode;
      } else {
        int blockid = get_blockid(mrq, node_idx);
        node = new ea_ht_node_t(node_idx, blockid, pnode);
      }      
      pnode->child_nodes.push_back(node);      
      
      // build the list of MAC blocks to fetch
      mrq->ht_fetch_list[node->blockid].push_back(node);
                        
      // add the src MAC block to the update list
      if (!mrq->is_read 
       && node_idx == src_node_idx) {        
        mrq->ht_update_list[node->blockid].push_back(node);
      }
    }
    
    // build the list of MAC blocks to update (exclude root node)
    if (!mrq->is_read 
     && parent_idx != 0) {
      mrq->ht_update_list[pnode->blockid].push_back(pnode);
    }
    
    ppnode = pnode;
  } 
  
  // task completed
  mrq->stages_pending &= ~stage;
  mrq->stages_completed |= stage;
  DBG(3, "*** stage %s completed for req#%d\n", toString(stage), mrq->tag);
  return true;
}

///////////////////////////////////////////////////////////////////////////////

EACtrGen::EACtrGen(EACoreBase* eacore, uint32_t entries, uint32_t assoc, uint32_t cycles) 
  : m_pending_reads(0)
  , m_pending_writes(0)
  , m_eacore(eacore) {
  m_cb_ctr = new TReqCompleteCB<EACtrGen>(this, &EACtrGen::ctr_request_complete_cb);
  m_cb_usr = new TReqCompleteCB<EACtrGen>(this, &EACtrGen::usr_request_complete_cb);
  CtrCache::ICallback* cb = new CtrCache::TCallback<EACtrGen>(this, &EACtrGen::cache_complete_cb);
  m_cache = new CtrCache(cb, entries * EA_BLOCK_SIZE, EA_BLOCK_SIZE, assoc, cycles);  
  cb->release();
}

EACtrGen::~EACtrGen() {
  m_cb_ctr->release();     
  m_cb_usr->release();     
  m_cache->release(); 
}

uint32_t EACtrGen::get_log2_macs_per_page(uint64_t addr) {
  uint64_t block_addr = addr >> EA_LOG2_BLOCK_SIZE;
  uint64_t ctr_addr = m_eacore->get_mlayout()->get_ctr_addr(block_addr);
  
  // ensure counters tore initialization
  this->ensure_counter_store(ctr_addr);  
  
  // return partition size   
  return m_ctrStore.at(ctr_addr).log2_psize;
}

void EACtrGen::ensure_counter_store(uint64_t ctr_addr) {
  // check if we already generated this counter, if not do it
  // this is a simple hack to avoid having to save all counters to memory at boot time
  if (0 == m_ctrStore.count(ctr_addr)) {     
    pmt::EACore* const pmt = dynamic_cast<pmt::EACore*>(m_eacore);
    if (pmt)
      m_ctrStore[ctr_addr].reset(pmt->m_log2_macs_per_page);
    else
      m_ctrStore[ctr_addr].reset(EA_LOG2_BLOCKS_PER_PAGE);
  }
}

bool EACtrGen::submit(ea_mrq_t* mrq, EA_STAGE stage) {
  if (mrq->parent) {
    //
    // ignore child blocks requests
    //
    assert(mrq->dt_type == EA_USER_BLOCK);
    // commit the stage
    mrq->stages_pending &= ~stage;      
    mrq->stages_completed |= stage;
    DBG(3, "*** stage %s completed for req#%d\n", toString(stage), mrq->tag);
    return true;
  }
    
  return EADevice0::submit(mrq, stage);
}

void EACtrGen::tick() {
  //
  // Build the seed for generating the encryption pad.
  // The seed is a concatenation of block addr and ctr value to generate a unique  
  // identifier for CTR encryption to be safe.
  //
  // To compte the seed, need to get the counter.
  // This stage looks up the counter cache, 
  // if not present, the counter if fetched from memory.
  // 
  // stage_latency = cache_latency + [memory_latency]
  //  
  
  if (!m_inputQ.empty()) {
    const ctrrq_t& crq = m_inputQ.front();
    int tag;
    
    // lookup duplicate requests
    auto iter = std::find_if(m_waitQ.begin(), m_waitQ.end(), 
        [crq](const WaitQ::value_type& v)->bool { 
      return v.second.front().addr == crq.addr; 
    });
    if (iter != m_waitQ.end()) {
      assert(crq.is_read);
      if (iter->second.front().is_read) {
        tag = iter->first; // reuse tag
        DBG(3, "*** duplicate read request for ctr block 0x%lx (tag#%d) at cycle %ld (req#%d)\n",
            crq.addr,
            tag,
            m_eacore->get_cycle(),
            crq.mrq->tag);
      } else {        
        tag = -1; // read-after-write stall
      }
    } else {
      IReqCompleteCB* const cb = (crq.dt_type == EA_CTR_BLOCK) ? m_cb_ctr : m_cb_usr;
      tag = m_eacore->submit_request(crq.addr, crq.is_read, crq.onchip_mask, crq.dt_type, -1, crq.mrq, cb);  
    }   
    if (tag != -1) {
      m_waitQ[tag].push_back(crq);
      m_inputQ.pop_front();
    }
  }
  
  if (m_active_mrq) { 
    if (tick(m_active_mrq, m_active_stage))
      m_active_mrq = nullptr;
  }
  
  // tick the internal cache
  m_cache->tick();  
}

bool EACtrGen::tick(ea_mrq_t* mrq, EA_STAGE stage) {  
  //--
  uint64_t block_addr = mrq->mem_addr >> EA_LOG2_BLOCK_SIZE;
  uint64_t ctr_addr = m_eacore->get_mlayout()->get_ctr_addr(block_addr);
  
  // do not update the counter for evicted blocks
  bool is_update = !mrq->is_read && mrq->dt_type != EA_EVICT_BLOCK;
  
  //
  // A counter block covers a full memory page
  // ideally we should enforce R/W fence at the CTR block (page) boundary
  // however, CTR block evictions cause hashtree updates which have global scope 
  // it is safer to simply use a global R/W fence to avoid out-of-order issues
  //  
  bool completed = false;
  do {
    // wait for pending writes to same ctr block to complete
    if (0 != m_pending_writes)
      break;
    // on update, wait for pending reads to same ctr block to complete
    if (is_update 
     && 0 != m_pending_reads) 
      break;
    
    // request the block from cache   
    if (m_cache->submit(ctr_addr, {mrq, stage}, is_update)) {       
      // fence updaye
      if (is_update) {
        ++m_pending_writes;
      } else {
        ++m_pending_reads;
      }
      
      // update stats
      m_stats.start_cycles[mrq->tag] = m_eacore->get_cycle();
            
      // release the device      
      completed = true; 
    }
  } while (false);

  return completed;
}

void EACtrGen::cache_complete_cb(const rqid_t& rqid, uint64_t addr, bool is_hit) {
  ea_mrq_t* const mrq  = rqid.mrq;  
  EA_STAGE stage = rqid.stage;
  ctrrq_t crq = {addr, true, 0, EA_CTR_BLOCK, mrq, stage};
  if (is_hit) {
    // increment block counter on writes       
    if (this->update_ctr({crq})) {      
      // fence update
      bool is_update = !mrq->is_read && mrq->dt_type != EA_EVICT_BLOCK;
      if (is_update) {
        assert(m_pending_writes != 0);
        --m_pending_writes;
      } else {
        assert(m_pending_reads != 0);
        --m_pending_reads;
      }     
      
      // commit the stage
      mrq->stages_pending &= ~stage;
      mrq->stages_completed |= stage;
      DBG(3, "*** stage %s completed for req#%d\n", toString(stage), mrq->tag);
      
      // update stats
      assert(0 != m_stats.start_cycles.count(mrq->tag));
      m_stats.total_cycles += (m_eacore->get_cycle() - m_stats.start_cycles.at(mrq->tag));
      m_stats.start_cycles.erase(mrq->tag);
      ++m_stats.invocations;
    }        
    // update stats
    ++m_stats.hits;
  } else {
    // ensure counters tore initialization
    this->ensure_counter_store(addr);    
      
    // submit a counter block request 
    m_inputQ.push_back(crq);       
    ++m_pending_ctr_rd_reqs[mrq->tag];
        
    // update stats
    ++m_stats.misses;
  }
}
void EACtrGen::ctr_request_complete_cb(int tag, bool is_speculative) {  
  if (is_speculative) {
    // update ctr counter
    this->update_ctr(m_waitQ.at(tag));    
    // insert ctr block into cache
    this->insert_ctr(m_waitQ.at(tag));
    this->release(tag);
  }
}

void EACtrGen::usr_request_complete_cb(int tag, bool is_speculative) {
  if (is_speculative){
    pmt::EACore* const pmt = dynamic_cast<pmt::EACore*>(m_eacore);    
    for (const ctrrq_t& crq : m_waitQ.at(tag)) {
      if (crq.is_read) {
        ea_mrq_t* const mrq = crq.mrq;  
        EA_STAGE stage = crq.stage;    
        if (pmt) {
          uint32_t log2_part_size = EA_LOG2_BLOCKS_PER_PAGE - this->get_log2_macs_per_page(crq.addr);
          uint32_t part_size  = 1 << log2_part_size; 
          uint64_t block_addr = crq.addr >> EA_LOG2_BLOCK_SIZE;
          uint64_t part_addr  = block_addr >> log2_part_size;
          for (int i = 0; i < part_size; ++i) {
            if (0 != ((crq.onchip_mask >> i) & 0x1))
              continue;          
            uint64_t _block_addr = ((part_addr << log2_part_size) + i);
            uint64_t _addr = _block_addr << EA_LOG2_BLOCK_SIZE;        
            // write the block back to memory
            DBG(3, "*** CTR overflow write request for usr block 0x%lx (req#%d)\n", _addr, mrq->tag);
            m_inputQ.push_back({_addr, false, 0, EA_USER_BLOCK, mrq, stage});
            ++m_pending_usr_wr_reqs[mrq->tag];
          }
        } else {
          // write the block back to memory
          DBG(3, "*** CTR overflow write request for usr block 0x%lx (req#%d)\n", crq.addr, mrq->tag);
          m_inputQ.push_back({crq.addr, false, 0, EA_USER_BLOCK, mrq, stage});
          ++m_pending_usr_wr_reqs[mrq->tag];
        }
      }
    }
    this->release(tag);
  }
}

bool EACtrGen::update_ctr(const std::vector<ctrrq_t>& crqs) {
  bool overflow = false;
  
  for (const ctrrq_t& crq : crqs) {    
    if (crq.is_read) {             
      ea_mrq_t* const mrq  = crq.mrq; 
      bool is_update = !mrq->is_read && mrq->dt_type != EA_EVICT_BLOCK;
      if (is_update) {
        // increment the block minor counter for write requests
        // check for max-value overflow to reset the page
        uint64_t block_addr = mrq->mem_addr >> EA_LOG2_BLOCK_SIZE;
        uint64_t block_pid = block_addr & EA_BLOCKS_PER_PAGE_MASK;      
        ctr_t& ctr = m_ctrStore.at(crq.addr);
        if (!ctr.incr_minor(block_pid)) {
          overflow = true;
        }
      }
    }
  }
  
  if (overflow) {    
    const ctrrq_t& crq = crqs.front();
    ea_mrq_t* const mrq  = crq.mrq;
    EA_STAGE stage = crq.stage;
    
    // increment major counter    
    ctr_t& ctr = m_ctrStore.at(crq.addr);
    ctr.incr_major();
    
    // apply dynamic partitioning
    if (get_config()->pmt_mode == "dynamic") {
      if (ctr.log2_psize > 0)
        --ctr.log2_psize;
    }
    
    // re-encrypt all the blocks in the associated page
    // do this by simply fetching all off-chip blocks and writing them back to memory
    
    uint64_t block_addr = mrq->mem_addr >> EA_LOG2_BLOCK_SIZE;
    uint64_t page_addr = block_addr >> EA_LOG2_BLOCKS_PER_PAGE;
    
    pmt::EACore* const pmt = dynamic_cast<pmt::EACore*>(m_eacore);
    if (pmt) {
      uint64_t page_onchip_mask = pmt->get_page_onchip_mask(mrq->mem_addr, mrq->src_nodeid);
      uint32_t log2_parts_per_page = ctr.log2_psize;
      uint32_t log2_part_size = EA_LOG2_BLOCKS_PER_PAGE - log2_parts_per_page;
      uint32_t parts_per_page = 1 << log2_parts_per_page;
      uint32_t part_size = 1 << log2_part_size;
      uint32_t block_pid = block_addr & (part_size - 1);
      uint32_t curr_part_idx = (block_addr >> log2_part_size) & (parts_per_page - 1);
      
      for (uint32_t i = 0; i < parts_per_page; ++i) {
        uint32_t onchip_mask = (page_onchip_mask >> (i * part_size)) & ((1 << part_size) - 1);        
        // mark cuurent block as onchip
        if (i == curr_part_idx)
          onchip_mask |= 1 << block_pid;
        
        for (uint32_t j = 0; j < part_size; ++j) {         
          // skip on chip blocks
          if (0 != ((onchip_mask >> j) & 0x1))
            continue;
          
          uint32_t _addr = ((page_addr << EA_LOG2_BLOCKS_PER_PAGE) + (j + i * part_size)) << EA_LOG2_BLOCK_SIZE;
          
          DBG(3, "*** CTR overflow read request for usr block 0x%lx (req#%d)\n", _addr, mrq->tag);
          m_inputQ.push_back({_addr, true, onchip_mask, EA_USER_BLOCK, mrq, stage});
          ++m_pending_usr_rd_reqs[mrq->tag];
          
          // submit a single request per partition
          break; 
        }
      }
    } else {          
      for (uint32_t i = 0; i < EA_BLOCKS_PER_PAGE; ++i) {
        uint32_t _addr = ((page_addr << EA_LOG2_BLOCKS_PER_PAGE) + i) << EA_LOG2_BLOCK_SIZE;
        if (_addr != mrq->mem_addr) {
          DBG(3, "*** CTR overflow read request for usr block 0x%lx (req#%d)\n", _addr, mrq->tag);
          m_inputQ.push_back({_addr, true, 0, EA_USER_BLOCK, mrq, stage});
          ++m_pending_usr_rd_reqs[mrq->tag];
        }
      }
    }
    
    // update stats
    ++m_stats.overflows;   
    
    // return false if pending child requests 
    return (0 == m_pending_usr_rd_reqs.count(mrq->tag));
  }
  
  return true;
}

bool EACtrGen::insert_ctr(const std::vector<ctrrq_t>& crqs) {
  // a single block insert for all the counters in the list
  const ctrrq_t& crq = crqs.front();
  if (crq.is_read) {
    ea_mrq_t* const mrq  = crq.mrq;  
    EA_STAGE stage = crq.stage;
    
    uint64_t victim_addr;
    bool is_update = !mrq->is_read && mrq->dt_type == EA_USER_BLOCK;
    if (!m_cache->insert(crq.addr, &victim_addr, is_update)) {    
      // send the victim block to memory (push request at the front)
      // we don't have to wait for the eviction to complete
      DBG(3, "*** evicting CTR block 0x%lx (req#%d)\n", victim_addr, mrq->tag);
      m_inputQ.push_front({victim_addr, false, 0, EA_CTR_BLOCK, mrq, stage}); // 
      ++m_pending_ctr_wr_reqs[mrq->tag];
      
      // udpate stats
      ++m_stats.evictions;
      return false;
    }
  }
  return true;
}

void EACtrGen::release(int tag) {
  for (const ctrrq_t& crq : m_waitQ.at(tag)) {
    ea_mrq_t* const mrq  = crq.mrq;
    EA_STAGE stage = crq.stage;
    
    switch (crq.dt_type) {
    case EA_CTR_BLOCK:
      if (crq.is_read) {
        assert(m_pending_ctr_rd_reqs[mrq->tag] > 0);
        if (0 == --m_pending_ctr_rd_reqs[mrq->tag])
          m_pending_ctr_rd_reqs.erase(mrq->tag);            
      } else {
        assert(m_pending_ctr_wr_reqs[mrq->tag] > 0);
        if (0 == --m_pending_ctr_wr_reqs[mrq->tag])
          m_pending_ctr_wr_reqs.erase(mrq->tag);    
      }
      break;
    case EA_USER_BLOCK:
      if (crq.is_read) {
        assert(m_pending_usr_rd_reqs[mrq->tag] > 0);
        if (0 == --m_pending_usr_rd_reqs[mrq->tag])
          m_pending_usr_rd_reqs.erase(mrq->tag);    
      } else {
        assert(m_pending_usr_wr_reqs[mrq->tag] > 0);
        if (0 == --m_pending_usr_wr_reqs[mrq->tag])
          m_pending_usr_wr_reqs.erase(mrq->tag);    
      }
    }
    
    if (0 == m_pending_ctr_rd_reqs.count(mrq->tag)
     && 0 == m_pending_ctr_wr_reqs.count(mrq->tag)
     && 0 == m_pending_usr_rd_reqs.count(mrq->tag)
     && 0 == m_pending_usr_wr_reqs.count(mrq->tag)) {
      
      // fence update
      bool is_update = !mrq->is_read && mrq->dt_type == EA_USER_BLOCK;
      if (is_update) {
        assert(m_pending_writes != 0);
        --m_pending_writes;
      } else {      
        assert(m_pending_reads != 0);
        --m_pending_reads;
      }
      
      // update stats
      assert(0 != m_stats.start_cycles.count(mrq->tag));
      m_stats.total_cycles += (m_eacore->get_cycle() - m_stats.start_cycles.at(mrq->tag));
      m_stats.start_cycles.erase(mrq->tag);
      ++m_stats.invocations;
      
      // commit the stage
      mrq->stages_pending &= ~stage;
      mrq->stages_completed |= stage;     
      DBG(3, "*** stage %s completed for req#%d\n", toString(stage), mrq->tag);
    }
  } 
  
  m_waitQ.erase(tag);  
}

void EACtrGen::print_stats(std::ostream& out) {
  uint64_t num_accesses = m_stats.hits + m_stats.misses;
  out << "  EACtrGen: number of invocations = " << num_accesses << endl;
  out << "  EACtrGen: hit rate = " << (m_stats.hits * 100) / num_accesses << "%" << endl;
  out << "  EACtrGen: eviction rate = " << (m_stats.evictions * 100) / num_accesses  << "%" << endl;
  out << "  EACtrGen: overflow rate = " << (m_stats.overflows * 100) / num_accesses  << "%" << endl;
  out << "  EACtrGen: average latency = " << ((double)m_stats.total_cycles) / m_stats.invocations  << " cycles" << endl;
}

///////////////////////////////////////////////////////////////////////////////

EAMacReq::EAMacReq(EACoreBase* eacore, uint32_t entries, uint32_t assoc, uint32_t cycles) 
  : m_eacore(eacore) {
  MCache::ICallback* const cb = new MCache::TCallback<EAMacReq>(this, &EAMacReq::cache_complete_cb);
  m_cache = new MCache(cb, entries * EA_BLOCK_SIZE, EA_BLOCK_SIZE, assoc, cycles);
  cb->release();  
  m_mem_complete_cb = new TMemCompleteCB<EAMacReq>(this, &EAMacReq::mem_complete_cb);  
}

EAMacReq::~EAMacReq() {
  m_cache->release();
  m_mem_complete_cb->release();
}

void EAMacReq::tick() {
  // process pending requests 
  if (!m_inputQ.empty()) {
    const mac_req_t& mac_req = m_inputQ.front();
    int tag;
    // lookup duplicate requests
    auto iter = std::find_if(m_waitQ.begin(), m_waitQ.end(), 
        [mac_req](const WaitQ::value_type& v)->bool { 
      return v.second.front().addr == mac_req.addr; 
    });
    if (iter != m_waitQ.end()) {    
      assert(mac_req.is_read);
      if (iter->second.front().is_read) {
        tag = iter->first; // reuse tag
        DBG(3, "*** duplicate read request for mac block 0x%lx (tag#%d) at cycle %ld (req#%d)\n",
            mac_req.addr,
            tag,
            m_eacore->get_cycle(),
            mac_req.mrq->tag);
      } else {        
        tag = -1; // read-after-write stall
      }
    } else {
      tag = m_eacore->submit_mem_request(
            mac_req.addr, mac_req.is_read, EA_MAC_BLOCK, mac_req.mrq->tag, m_mem_complete_cb);
    }
    if (tag != -1) {
      m_waitQ[tag].push_back(mac_req);      
      m_inputQ.pop_front();
    }
  }
  
  if (m_active_mrq) { 
    if (tick(m_active_mrq, m_active_stage))
      m_active_mrq = nullptr;
  }
  
  // tick the internal cache
  m_cache->tick();
}

bool EAMacReq::tick(ea_mrq_t* mrq, EA_STAGE stage) {
  //
  // A MAC block is shared by multiple requests
  // We should enforce R/W fence at the block boundary
  //    
  uint64_t block_addr = mrq->mem_addr >> EA_LOG2_BLOCK_SIZE;
  uint64_t mac_addr = m_eacore->get_mlayout()->get_user_mac_addr(block_addr);
  
  bool completed = false;  
  do {
    // wait for pending writes to same ctr block to complete
    if (0 != m_active_wr_blocks.count(mac_addr))
      break;
    // on update, wait for pending reads to same ctr block to complete
    if (!mrq->is_read 
     && 0 != m_active_rd_blocks.count(mac_addr)) 
      break;
    
    // request the block from cache   
    if (m_cache->submit(mac_addr, {mrq, stage}, !mrq->is_read)) {
      // fence update
      if (mrq->is_read) {
        ++m_active_rd_blocks[mac_addr];
      } else {
        ++m_active_wr_blocks[mac_addr];
      }
      
      // update stats
      m_stats.start_cycles[mrq->tag] = m_eacore->get_cycle();
      
      // release the device
      completed = true; 
    }
  } while (false);
  
  return completed; 
}

void EAMacReq::cache_complete_cb(const rqid_t& rqid, uint64_t addr, bool is_hit) {
  ea_mrq_t* const mrq  = rqid.mrq;  
  EA_STAGE stage = rqid.stage;
  if (is_hit) { 
    // fence update
    if (mrq->is_read) {
      assert(m_active_rd_blocks[addr] > 0);
      if (0 == --m_active_rd_blocks[addr])
        m_active_rd_blocks.erase(addr);
    } else {
      assert(m_active_wr_blocks[addr] > 0);
      if (0 == --m_active_wr_blocks[addr])
        m_active_wr_blocks.erase(addr);
    }
    // commit the stage    
    mrq->stages_pending &= ~stage;
    mrq->stages_completed |= stage;    
    DBG(3, "*** stage %s completed for req#%d\n", toString(stage), mrq->tag);
    
    // update stats
    ++m_stats.hits;
    
    // update stats
    assert(0 != m_stats.start_cycles.count(mrq->tag));
    m_stats.total_cycles += (m_eacore->get_cycle() - m_stats.start_cycles.at(mrq->tag));
    m_stats.start_cycles.erase(mrq->tag);
    ++m_stats.invocations;
  } else {
    // submit MAC fetch request
    uint64_t block_addr = mrq->mem_addr >> EA_LOG2_BLOCK_SIZE;
    uint64_t mac_addr = m_eacore->get_mlayout()->get_user_mac_addr(block_addr);
    m_inputQ.push_back({mac_addr, true, mrq, stage});    
    
    // update stats
    ++m_stats.misses;
  }
}

void EAMacReq::mem_complete_cb(int tag) {    
  // insert the block into the cache on read access
  const mac_req_t& mac_req = m_waitQ.at(tag).front();
  if (mac_req.is_read) { 
    {
      uint64_t victim_addr;
      ea_mrq_t* const mrq  = mac_req.mrq;
      EA_STAGE stage = mac_req.stage;  
      if (!m_cache->insert(mac_req.addr, &victim_addr, !mrq->is_read)) {    
        // send the victim block to memory (push request at the front)
        // we don't have to wait for the eviction to complete
        DBG(3, "*** evicting MAC block 0x%lx (req#%d)\n", victim_addr, mrq->tag);
        m_inputQ.push_front({victim_addr, false, mrq, stage});
        // udpate stats
        ++m_stats.evictions;
      }
    }
    
    // commit the state
    for (const mac_req_t& mac_req : m_waitQ.at(tag)) {
      ea_mrq_t* const mrq  = mac_req.mrq;
      EA_STAGE stage = mac_req.stage;  
      
      // fence update
      uint64_t block_addr = mrq->mem_addr >> EA_LOG2_BLOCK_SIZE;
      uint64_t mac_addr = m_eacore->get_mlayout()->get_user_mac_addr(block_addr);
      if (mrq->is_read) { 
        assert(m_active_rd_blocks[mac_addr] > 0);
        if (0 == --m_active_rd_blocks[mac_addr])
          m_active_rd_blocks.erase(mac_addr);
      } else {      
        assert(m_active_wr_blocks[mac_addr] > 0);
        if (0 == --m_active_wr_blocks[mac_addr])
          m_active_wr_blocks.erase(mac_addr);
      }
      
      // update stats
      assert(0 != m_stats.start_cycles.count(mrq->tag));
      m_stats.total_cycles += (m_eacore->get_cycle() - m_stats.start_cycles.at(mrq->tag));
      m_stats.start_cycles.erase(mrq->tag);
      ++m_stats.invocations;
      
      // commit the stage      
      mrq->stages_pending &= ~stage;
      mrq->stages_completed |= stage;      
      DBG(3, "*** stage %s completed for req#%d\n", toString(stage), mrq->tag);
    }
  }
  
  // free the queue
  m_waitQ.erase(tag);
}

void EAMacReq::print_stats(std::ostream& out) {
  uint64_t num_accesses = m_stats.hits + m_stats.misses;
  out << "  EAMacCache: number of invocations = " << num_accesses << endl;
  out << "  EAMacCache: hit rate = " << (m_stats.hits * 100) / num_accesses << "%" << endl;
  out << "  EAMacCache: eviction rate = " << (m_stats.evictions * 100) / num_accesses  << "%" << endl;
  out << "  EAMacCache: average latency = " << ((double)m_stats.total_cycles) / m_stats.invocations  << " cycles" << endl;
}

///////////////////////////////////////////////////////////////////////////////

EAMemReq::EAMemReq(EACoreBase* eacore) 
  : m_eacore(eacore) {
  m_mem_complete_cb = new TMemCompleteCB<EAMemReq>(this, &EAMemReq::mem_complete_cb);
}

EAMemReq::~EAMemReq() {
  m_mem_complete_cb->release();
}

bool EAMemReq::tick(ea_mrq_t* mrq, EA_STAGE stage) {
  bool completed = false;
  assert(stage == EA_MEM_REQ);
  do {
    if (mrq->part_list.empty()) {
      uint64_t addr = mrq->mem_addr;
      int tag = m_eacore->submit_mem_request(
            addr, mrq->is_read, mrq->dt_type, mrq->tag, m_mem_complete_cb);
      if (tag == -1)
        break;      
      m_waitQ[tag].push_back(mrq);
      completed = true; // release the device
      
      // update stats
      m_stats.start_cycles[mrq->tag] = m_eacore->get_cycle();
    } else {
      int index = m_pending_reqs[mrq->tag];
      uint64_t addr = mrq->part_list.at(index);
      int tag = m_eacore->submit_mem_request(
            addr, mrq->is_read, mrq->dt_type, mrq->tag, m_mem_complete_cb);
      if (tag == -1)
        break;      
      m_waitQ[tag].push_back(mrq);
      if (++m_pending_reqs[mrq->tag] == mrq->part_list.size()) {
        m_pending_reqs.erase(mrq->tag);
        completed = true; // release the device
        
        // update stats
        m_stats.start_cycles[mrq->tag] = m_eacore->get_cycle();
      }
    }
  } while (false);
  
  return completed;
}

void EAMemReq::mem_complete_cb(int tag) {
  //--
  EA_STAGE stage = EA_MEM_REQ;
  for (ea_mrq_t* mrq : m_waitQ.at(tag)) {
    if (mrq->part_list.empty() 
     || ++m_completed_reqs[mrq->tag] == mrq->part_list.size()) {
      m_completed_reqs.erase(mrq->tag);
      // commit the stage
      mrq->stages_pending &= ~stage;
      mrq->stages_completed |= stage;
      DBG(3, "*** stage %s completed for req#%d\n", toString(stage), mrq->tag);
      
      // update stats
      assert(0 != m_stats.start_cycles.count(mrq->tag));
      m_stats.total_cycles += (m_eacore->get_cycle() - m_stats.start_cycles.at(mrq->tag));
      m_stats.start_cycles.erase(mrq->tag);
      ++m_stats.invocations;
    }
  }
  
  // free the queue
  m_waitQ.erase(tag);
}

void EAMemReq::print_stats(std::ostream& out) {
  out << "  EAMemReq: number of invocations = " << m_stats.invocations << endl;
  out << "  EAMemReq: average latency = " << ((double)m_stats.total_cycles) / m_stats.invocations  << " cycles" << endl;
}

///////////////////////////////////////////////////////////////////////////////

EAHtReq::EAHtReq(EACoreBase* eacore) 
  : m_eacore(eacore)
  , m_pending_reads(0)
  , m_pending_writes(0) {
  m_mem_complete_cb = new TMemCompleteCB<EAHtReq>(this, &EAHtReq::mem_complete_cb);
}

EAHtReq::~EAHtReq() {
  m_mem_complete_cb->release();
}

bool EAHtReq::submit(ea_mrq_t* mrq, EA_STAGE stage) {
  if (!EADevice0::submit(mrq, stage))
    return false;  
  
  switch (stage) {
  case EA_HT_RD:
    m_ht_fetch_iter = mrq->ht_fetch_list.begin();
    break;
  case EA_HT_WR:
    m_ht_update_iter = mrq->ht_update_list.begin();    
    break;
  } 
  
  return true;
}

bool EAHtReq::tick(ea_mrq_t* mrq, EA_STAGE stage) {
  bool completed = false;
  
  switch (stage) {
  case EA_HT_RD: {    
    // wait for pending writes to complete
    if (0 != m_pending_writes)
      break;
    // get the current hashtree block addr
    int blockid = m_ht_fetch_iter->first;
    uint64_t addr = mrq->ht_blocks.at(blockid);
    
    // look up the schatch buffer
    if (0 != m_scratchbuffer.count(blockid) 
     && addr == m_scratchbuffer.at(blockid)) {
      DBG(3, "*** scratchbuffer hit for ht block 0x%lx at cycle %ld (req#%d)\n", 
          addr, m_eacore->get_cycle(), mrq->tag);
      ++m_fetched_blocks[mrq->tag];
      for (ea_ht_node_t* node : mrq->ht_fetch_list.at(blockid)) {
        node->stages_completed |= stage;        
        ++node->parent->fetched_childnodes; 
      }      
    } else {
      int tag;
      
      // lookup duplicate requests
      auto iter = std::find_if(m_waitQ.begin(), m_waitQ.end(), 
          [addr](const WaitQ::value_type& v)->bool { 
        return v.second.front().addr == addr; 
      });
      if (iter != m_waitQ.end()) {  
        tag = iter->first;
        DBG(3, "*** duplicate read request for ht block 0x%lx (tag#%d) at cycle %ld (req#%d)\n",
            addr,
            tag,
            m_eacore->get_cycle(),
            mrq->tag);
      } else {
        tag = m_eacore->submit_mem_request(
              addr, true, EA_HT_BLOCK, mrq->tag, m_mem_complete_cb);
        if (tag == -1)
          break;  
      }
      m_waitQ[tag].push_back({mrq, stage, addr, blockid});
    }
    
    if (++m_ht_fetch_iter == mrq->ht_fetch_list.end()) {      
      if (m_fetched_blocks[mrq->tag] == mrq->ht_fetch_list.size()) {
        m_fetched_blocks.erase(mrq->tag);
        // commit the stage
        mrq->stages_pending &= ~stage;
        mrq->stages_completed |= stage;
        DBG(3, "*** stage %s completed for req#%d\n", toString(stage), mrq->tag);
        
        // update stats
        m_stats.total_cycles += 1;
        ++m_stats.invocations;
      } else {
        ++m_pending_reads;
        
        // update stats
        m_stats.start_rd_cycles[mrq->tag] = m_eacore->get_cycle();
      }
      completed = true; // release the device
    }    
  } break;
  case EA_HT_WR: {    
    // wait for pending reads and writes to complete
    if (0 != m_pending_writes 
     || 0 != m_pending_reads)
      break;
    
    if (m_ht_update_iter == mrq->ht_update_list.begin()) {
      // ensure the main hash block has been fetched
      ea_ht_node_t* pnode = m_ht_update_iter->second.front();
      if (0 == (pnode->stages_completed & EA_HT_RD))
        break;
    } else {
      // ensure child nodes in current block have been rehashed
      bool rehashed = true;
      for (ea_ht_node_t* pnode : m_ht_update_iter->second) {
        if (0 == (pnode->stages_completed & EA_HT_HASH_WR)) {
          rehashed = false;
          break;
        }
      }
      if (!rehashed)
        break;
    }
    int blockid = m_ht_update_iter->first;
    uint64_t addr = mrq->ht_blocks.at(blockid);
    int tag = m_eacore->submit_mem_request(
          addr, false, EA_HT_BLOCK, mrq->tag, m_mem_complete_cb);
    if (tag == -1)
      break;
    m_waitQ[tag].push_back({mrq, stage, addr, blockid});
    // check end of list
    if (++m_ht_update_iter == mrq->ht_update_list.end()) {
      m_scratchbuffer.clear(); // clear the scratch buffer
      ++m_pending_writes;
      completed = true; // release the device   
      
      // update stats
      m_stats.start_wr_cycles[mrq->tag] = m_eacore->get_cycle();
      break;
    }
  } break;
  default:
    abort();
  }
  
  return completed;
}

void EAHtReq::mem_complete_cb(int tag) {  
  //--
  for (const task_t& task : m_waitQ.at(tag)) {
    ea_mrq_t* mrq    = task.mrq;  
    EA_STAGE stage   = task.stage;
    uint64_t blockid = task.blockid; 
    
    switch (stage) {
    case EA_HT_RD: {
      // commit all nodes included in the memory block
      for (ea_ht_node_t* node : mrq->ht_fetch_list.at(blockid)) {
        node->stages_completed |= stage;        
        ++node->parent->fetched_childnodes; 
      }
      
      // update scratch buffer
      uint64_t addr = mrq->ht_blocks.at(blockid);
      m_scratchbuffer[blockid] = addr;
          
      // terminate the task after all hash blocks are fetched. 
      if (++m_fetched_blocks[mrq->tag] == mrq->ht_fetch_list.size()) {
        m_fetched_blocks.erase(mrq->tag);
        assert(m_pending_reads != 0);
        --m_pending_reads;
        // commit the stage
        mrq->stages_pending &= ~stage;
        mrq->stages_completed |= stage;
        DBG(3, "*** stage %s completed for req#%d\n", toString(stage), mrq->tag);
        
        // update stats
        assert(0 != m_stats.start_rd_cycles.count(mrq->tag));
        m_stats.total_cycles += (m_eacore->get_cycle() - m_stats.start_rd_cycles.at(mrq->tag));
        m_stats.start_rd_cycles.erase(mrq->tag);
        ++m_stats.invocations;
      }
    } break;    
    case EA_HT_WR: {
      // commit all nodes included in the memory block
      for (ea_ht_node_t* node : mrq->ht_update_list.at(blockid)) {
        node->stages_completed |= stage;      
      }
      // terminate the task after all hash blocks are writen.
      if (++m_updated_blocks[mrq->tag] == mrq->ht_update_list.size()) {        
        m_updated_blocks.erase(mrq->tag);
        assert(m_pending_writes != 0);
        --m_pending_writes;
        // commit the stage
        mrq->stages_pending &= ~stage;
        mrq->stages_completed |= stage;
        DBG(3, "*** stage %s completed for req#%d\n", toString(stage), mrq->tag);
        
        // update stats
        assert(0 != m_stats.start_wr_cycles.count(mrq->tag));
        m_stats.total_cycles += (m_eacore->get_cycle() - m_stats.start_wr_cycles.at(mrq->tag));
        m_stats.start_wr_cycles.erase(mrq->tag);
        ++m_stats.invocations;
      }
    } break;
    default:
      abort();
    }
  }
  
  // free the queue
  m_waitQ.erase(tag);
}

void EAHtReq::print_stats(std::ostream& out) {
  out << "  EAHtReq: number of invocations = " << m_stats.invocations << endl;
  out << "  EAHtReq: average latency = " << ((double)m_stats.total_cycles) / m_stats.invocations  << " cycles" << endl;
}

///////////////////////////////////////////////////////////////////////////////

HtHashGen::HtHashGen(EACoreBase* eacore, int cycles, int ports, int batch_size) 
  : m_eacore(eacore)
  , m_batch_size(batch_size) {
  TClkDevice<task_t>::ICallback* cb = 
      new TClkDevice<task_t>::TCallback<HtHashGen>(this, &HtHashGen::onComplete);
  m_device = new TClkDevice<task_t>(cb, cycles, ports);
  cb->release();
}

HtHashGen::~HtHashGen() { 
  delete m_device; 
}

bool HtHashGen::submit(ea_mrq_t* mrq, EA_STAGE stage) {
  //--
  bool device_full = false;
  
  switch (stage) {
  case  EA_HT_HASH_RD:
    //
    // Iterate thru all parent nodes and compute their MAC.
    // Computing a parent node MAC requires hashing all child nodes.
    // The hashing is done is order from bottom level to top level.
    //
    for (ea_ht_node_t* pnode : mrq->ht_pnode_list) {      
      // ensure all child nodes are avialable
      if (pnode->fetched_childnodes != pnode->child_nodes.size())
        continue;          
      if (0 == (pnode->stages_pending & stage)
       && 0 == (pnode->stages_completed & stage)) { 
        // schedule all child nodes for hashing
        // check if any child node is dirty
        bool need_hashing = false;
        for (auto cn : pnode->child_nodes) {
          // look up the schatch buffer
          uint64_t addr = mrq->ht_blocks.at(cn->blockid);         
          if (0 == m_scratchbuffer.count(cn->blockid) 
           || addr != m_scratchbuffer.at(cn->blockid)) {
            need_hashing = true;
            break;
          }
        }
        if (need_hashing) {
          if (m_device->submit({mrq, stage, pnode}, pnode->child_nodes.size() * m_batch_size)) {
            pnode->stages_pending |= stage;            
          } else {
            device_full = true;
            break;
          }
        } else {
          pnode->stages_completed |= stage;
          ++m_hashed_rd_blocks[mrq->tag];          
        }
        
        if (++m_pending_rd_blocks[mrq->tag] == mrq->ht_pnode_list.size()) {            
          m_pending_rd_blocks.erase(mrq->tag);          
          if (m_hashed_rd_blocks[mrq->tag] == mrq->ht_pnode_list.size()) {
            m_hashed_rd_blocks.erase(mrq->tag);            
            mrq->stages_completed |= stage;             
            DBG(3, "*** stage %s completed for req#%d\n", toString(stage), mrq->tag);  
            
            // update stats
            m_stats.total_cycles += 1;
            ++m_stats.invocations;
          } else {
            mrq->stages_pending |= stage;
            DBG(3, "*** stage %s pending for req#%d\n", toString(stage), mrq->tag);
            
            // update stats
            m_stats.start_rd_cycles[mrq->tag] = m_eacore->get_cycle();
          }                    
        }
      }
    }
    break;
  case EA_HT_HASH_WR:
    //
    // Iterate thru all parent nodes and compute their MAC.
    // Computing a parent node MAC requires hashing all child nodes.
    // The hashing is done is order from bottom level to top level.
    // Ensure that the hash tree verification is complete
    // and the original block has been hashed
    //
    for (ea_ht_node_t* pnode : mrq->ht_pnode_list) {      
      // ensure the parent node has been authenticated
      if (0 == (pnode->stages_completed & EA_HT_AUTH))
        continue;          
      if (0 == (pnode->stages_pending & stage)
       && 0 == (pnode->stages_completed & stage)) {       
        // schedule all child nodes for hashing
        if (m_device->submit({mrq, stage, pnode}, pnode->child_nodes.size() * m_batch_size)) {
          pnode->stages_pending |= stage;
          if (++m_pending_wr_blocks[mrq->tag] == mrq->ht_pnode_list.size()) {
            m_pending_wr_blocks.erase(mrq->tag);
            mrq->stages_pending |= stage;
            DBG(3, "*** stage %s pending for req#%d\n", toString(stage), mrq->tag);
            
            // update stats
            m_stats.start_wr_cycles[mrq->tag] = m_eacore->get_cycle();
          }
        } else {
          device_full = true;
          break;
        }
      }
    }
    break;
  default:
    abort();
  }
  
  return !device_full;
}

void HtHashGen::onComplete(const task_t& task) {
  ea_mrq_t* const mrq  = task.mrq;  
  EA_STAGE stage = task.stage; 
  ea_ht_node_t* pnode = task.pnode;
  
  switch (stage) {
  case EA_HT_HASH_RD:
    pnode->stages_pending &= ~stage;
    pnode->stages_completed |= stage;
    // udpate scratch buffer
    for (auto cn : pnode->child_nodes) {
      uint64_t addr = mrq->ht_blocks.at(cn->blockid);
      m_scratchbuffer[cn->blockid] = addr;
    }
    if (++m_hashed_rd_blocks[mrq->tag] == mrq->ht_pnode_list.size()) {
     m_hashed_rd_blocks.erase(mrq->tag);
      mrq->stages_pending &= ~stage;
      mrq->stages_completed |= stage;
      DBG(3, "*** stage %s completed for req#%d\n", toString(stage), mrq->tag);
      
      // update stats
      assert(0 != m_stats.start_rd_cycles.count(mrq->tag));
      m_stats.total_cycles += (m_eacore->get_cycle() - m_stats.start_rd_cycles.at(mrq->tag));
      m_stats.start_rd_cycles.erase(mrq->tag);
      ++m_stats.invocations;
    }
    break;
  case EA_HT_HASH_WR:
    pnode->stages_pending &= ~stage;
    pnode->stages_completed |= stage;
    // udpate scratch buffer
    for (auto cn : pnode->child_nodes) {
      m_scratchbuffer.erase(cn->blockid);
    }
    if (++m_hashed_wr_blocks[mrq->tag] == mrq->ht_pnode_list.size()) {
      m_hashed_wr_blocks.erase(mrq->tag);
      mrq->stages_pending &= ~stage;
      mrq->stages_completed |= stage;
      DBG(3, "*** stage %s completed for req#%d\n", toString(stage), mrq->tag);
      
      // update stats
      assert(0 != m_stats.start_wr_cycles.count(mrq->tag));
      m_stats.total_cycles += (m_eacore->get_cycle() - m_stats.start_wr_cycles.at(mrq->tag));
      m_stats.start_wr_cycles.erase(mrq->tag);
      ++m_stats.invocations;
    }
    break;
  default:
    abort();
  }  
}

void HtHashGen::print_stats(std::ostream& out) {  
  out << "  HtHashGen: number of invocations = " << (m_stats.invocations * m_batch_size) << endl;
  out << "  HtHashGen: average latency = " << ((double)m_stats.total_cycles) / m_stats.invocations  << " cycles" << endl;
}

///////////////////////////////////////////////////////////////////////////////

bool EAHtAuth::submit(ea_mrq_t* mrq, EA_STAGE stage) {
  if (m_active_mrq)
    return false;
  
  switch (stage) {  
  case EA_BLOCK_AUTH: {
    //
    // Authenticate data block
    //   1) ensure block's reference MAC from memory is available 
    //   2) ensure block's generated MAC from data is complete 
    //   3) compare MACs
    //
    
    // check if the authentication is hashtree based
    if (!mrq->ht_fetch_list.empty()) {    
      // ensure the source hash block has been fetched
      ea_ht_node_t* pnode = mrq->ht_fetch_list.begin()->second.front();
      if (0 == (pnode->stages_completed & EA_HT_RD))
        return false;
    }
    
    m_active_mrq = mrq;
    m_active_stage = stage;
    mrq->stages_pending |= stage;    
    DBG(3, "*** stage %s pending for req#%d\n", toString(stage), mrq->tag);
  } break;
  case  EA_HT_AUTH: {
    //
    // Authenticate hash tree (for each parent node)
    //   1) ensure parent's old MAC from memory is avialable (except root node)
    //   2) ensure parent's generated MAC from child nodes is complete 
    //   3) compare MACs
    //
    for (ea_ht_node_t* pnode : mrq->ht_pnode_list) {
      // all parent nodes except the parent should have been fetched and 
      // all parent nodes should have been hashed
      if ((pnode->idx != 0 
        && 0 == (pnode->stages_completed & EA_HT_RD)) 
       || 0 == (pnode->stages_completed & EA_HT_HASH_RD))
        continue;      
      if (0 == (pnode->stages_pending & stage)
       && 0 == (pnode->stages_completed & stage)) {
        m_active_mrq   = mrq;
        m_active_stage = stage;
        m_pnode = pnode;
        pnode->stages_pending |= stage;        
        if (++m_pending_blocks[mrq->tag] == mrq->ht_pnode_list.size()) {
          m_pending_blocks.erase(mrq->tag);
          mrq->stages_pending |= stage;
          DBG(3, "*** stage %s pending for req#%d\n", toString(stage), mrq->tag);
        }
        break;
      }
    }
  } break;
  default:
    abort();
  }
  
  return (m_active_mrq != nullptr);
}

bool EAHtAuth::tick(ea_mrq_t* mrq, EA_STAGE stage) {  
  switch (stage) {
  case EA_BLOCK_AUTH: {
    mrq->stages_pending &= ~stage;
    mrq->stages_completed |= stage;
    DBG(3, "*** stage %s completed for req#%d\n", toString(stage), mrq->tag);
  } break;
  case EA_HT_AUTH: {    
    m_pnode->stages_pending &= ~stage;
    m_pnode->stages_completed |= stage;    
    if (++m_authenticated_blocks[mrq->tag] == mrq->ht_pnode_list.size()) {
      m_authenticated_blocks.erase(mrq->tag);
      mrq->stages_pending &= ~stage;
      mrq->stages_completed |= stage;  
      DBG(3, "*** stage %s completed for req#%d\n", toString(stage), mrq->tag);
    }          
  } break;
  default:
    abort();
  }
  
  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool EACommit::tick(ea_mrq_t* mrq, EA_STAGE stage) {
  switch (stage) {
  case EA_COMMIT0: {
    mrq->cb->onComplete(mrq->tag, true);
    mrq->stages_pending &= ~stage;
    mrq->stages_completed |= stage;
    DBG(3, "*** stage %s completed for req#%d\n", toString(stage), mrq->tag);    
  } break;
  case EA_COMMIT: {    
    // commit the request
    mrq->cb->onComplete(mrq->tag, false);     
    mrq->stages_pending &= ~stage;
    mrq->stages_completed |= stage;
    DBG(3, "*** stage %s completed for req#%d\n", toString(stage), mrq->tag);    
    
    // free hash tree nodes
    for (ea_ht_node_t* p : mrq->ht_pnode_list) {
      for (ea_ht_node_t* c : p->child_nodes) {
        delete c;
      }
    }   
    
    // free request
    m_eacore->release(mrq);   
  } break;
  default:
    abort();
  }
  
  return true;
}

///////////////////////////////////////////////////////////////////////////////

EACoreBase::EACoreBase(
    Controller* ctrl,
   const std::vector<ea_stage_t>& rd_stages,
   const std::vector<ea_stage_t>& wb_stages) 
  : m_ctrl(ctrl)
  , m_rd_stages(rd_stages)
  , m_wb_stages(wb_stages)  
  , m_queue_depth(get_config()->queue_depth)  
  , m_mrq_tags(0)
  , m_ea_bus_arbitration(false)
  , m_mem_bus_arbitration(false)
  , m_hashtree(nullptr)
  , m_mem_layout(nullptr)
{}

EACoreBase::~EACoreBase() {
  delete m_hashtree;
  delete m_mem_layout;
  for (auto& device : m_devices) {
    delete device.second;
  }
}

uint64_t EACoreBase::get_cycle() {
  return m_ctrl->get_clock()->NowTicks();
}

void EACoreBase::tick() {  
  // clear bus arbiters
  m_ea_bus_arbitration = false;
  m_mem_bus_arbitration = false;  
  
  // iterate thru all devices  
  for (auto dev : m_devices) {
   
    // iterate thru all requests in order
    bool move_next_device = false;
    for (ea_mrq_t* mrq : m_mrqs) {
      if (move_next_device)
        break;
      
      // iterate thru all stages in order
      const std::vector<ea_stage_t>& stages = 
          mrq->is_read ? m_rd_stages : m_wb_stages;
      for (ea_stage_t stage : stages) {   
        // check stage device
        if (stage.device != dev.first)         
          continue;
          
        // check stage datatype
        if (0 == (stage.dt_types & mrq->dt_type))
          continue;
        
        // check stage completed status
        if (mrq->stages_completed & stage.id
         || mrq->stages_pending & stage.id)
          continue;
        
        // check stage dependencies
        if ((mrq->stages_completed & stage.dependencies) != stage.dependencies) {
          if (dev.second->does_memory_access()) {
            move_next_device = true; // enforce request ordering
            break;
          }
          continue;
        }
             
        // submit the request to the device
        if (!dev.second->submit(mrq, stage.id)) {
          move_next_device = true; // the device is full
          break;
        }        
      }
    }
  }
  
  // advance all devices clock tick
  for (auto& device : m_devices) {
    device.second->tick();
  } 
}

int EACoreBase::allocate_tag() {
  int tag = m_mrq_tags++;
  // handle special values
  switch (tag) {
  case -1:
    tag = m_mrq_tags++;
    break;
  }
  return tag;
}

int EACoreBase::submit_mem_request(
    uint64_t addr, 
    bool is_read, 
    EA_BLOCK_TYPE dt, 
    int req, 
    IMemCompleteCB* cb) {
  // ensure a single access per cycle
  if (m_mem_bus_arbitration)
    return -1;
  m_mem_bus_arbitration = true;    
  
  // submit the request
  return m_ctrl->submit_mem_request(addr, is_read, dt, req, cb);
}

int EACoreBase::submit_request(
    uint64_t addr, 
    bool is_read, 
    uint32_t onchip_mask,
    EA_BLOCK_TYPE dt,
    int src_nodeid,
    ea_mrq_t* parent,
    IReqCompleteCB* cb) {
  
  // ensure a single access per cycle
  if (m_ea_bus_arbitration)
    return -1;
  m_ea_bus_arbitration = true;
    
  // ensure that the request queue is not full
  if (m_mrqs.size() >= m_queue_depth) {
    ++m_stats.queue_stalls;
    return -1;
  }
      
  // create a new request tag
  int tag = this->allocate_tag();
  
  // create a new reauest
  if (parent)
    DBG(2, "EA: %s request for %s block 0x%lx from parent#%d (req#%d)\n", toRWString(is_read), toString(dt), addr, parent->tag, tag);
  else
    DBG(2, "EA: %s request for %s block 0x%lx (req#%d)\n", toRWString(is_read), toString(dt), addr, tag);
  ea_mrq_t* const mrq = new ea_mrq_t(addr, tag, dt, is_read, onchip_mask, src_nodeid, cb, parent);
  if (parent) { 
    // insert the request right before its parent
    auto iter = std::find(m_mrqs.begin(), m_mrqs.end(), parent);
    m_mrqs.insert(iter, mrq);
  } else {
    m_mrqs.push_back(mrq);
  }
  
  // udpate stats
  if (m_mrqs.size() > m_stats.max_queue_size) {
    m_stats.max_queue_size = m_mrqs.size();
  }
  
  return tag;
}

void EACoreBase::release(ea_mrq_t* mrq) {
  DBG(2, "EA: committed request req#%d\n", mrq->tag);
  auto iter = std::find(m_mrqs.begin(), m_mrqs.end(), mrq);
  assert(iter != m_mrqs.end());
  m_mrqs.erase(iter);
  mrq->release();
}

void EACoreBase::print_stats(std::ostream& out) {
  config_t* config = get_config();
  out << "*****  " << config->type <<  " EACore  *****" << endl;
  out << "  Total Mem Footprint = " << m_stats.mem_footprint << " bytes" << endl;
  out << "  Total Mem Overhead = " << (((m_stats.mem_footprint - config->sys_mem) * 100.0) / config->sys_mem)  << "%" << endl;
  out << "  Max Request Queue Size = " << m_stats.max_queue_size << endl;
  out << "  Request Queue Stalls = " << m_stats.queue_stalls << endl;
  for (auto& device : m_devices) {
    device.second->print_stats(out);
  } 
}

}
}
