#ifndef GMTCORE_H
#define GMTCORE_H

#include "eacore.h"

namespace manifold {
namespace pagevault {
namespace gmt {

class EACore : public EACoreBase {
public: 
  
  EACore(Controller* ctrl);
  ~EACore();
  
  uint32_t get_hashtree_nodeidx(ea_mrq_t* mrq) override;
  
protected:
  
  enum {    
    LOG2_CTR_PER_BLOCK = 6, // 64 counters per block
    CTR_PER_BLOCK      = (1 << LOG2_CTR_PER_BLOCK),
    CTR_PER_BLOCK_MASK = CTR_PER_BLOCK - 1,    
    MTREE_ARY          = 4,
  };
  
  class mem_layout_t : public IMemLayout {
  public:
    mem_layout_t(uint64_t ctr_offset, 
                 uint64_t ctr_size,
                 uint64_t ht_offset, 
                 uint64_t ht_size,
                 uint8_t  log2_macs_in_block) 
      : m_ctr_offset(ctr_offset)
      , m_ctr_size(ctr_size)
      , m_ht_offset(ht_offset)   
      , m_ht_size(ht_size)
      , m_log2_macs_in_block(log2_macs_in_block) 
    {}
    
    // return the block address of specified user mac
    uint64_t get_user_mac_addr(uint32_t mac_idx) {
      return get_ht_mac_addr(mac_idx);
    }
    
    // return the block address of specified counter
    uint64_t get_ctr_addr(uint32_t ctr_idx) {
      assert(ctr_idx < m_ctr_size);
      return (m_ctr_offset + (ctr_idx >> LOG2_CTR_PER_BLOCK)) << EA_LOG2_BLOCK_SIZE;
    }
    
    // return the block address of specified hash tree node
    uint64_t get_ht_mac_addr(uint32_t node_idx) {
      assert(node_idx < m_ht_size);
      assert(node_idx > 0); // the root node is stored on-chip
      // remove root node when calculating block count      
      uint64_t mac_block = (node_idx - 1) >> m_log2_macs_in_block;
      return (m_ht_offset + mac_block) << EA_LOG2_BLOCK_SIZE;
    }
    
  private:
    uint64_t  m_ctr_offset;
    uint64_t  m_ctr_size;
    uint64_t  m_ht_offset;
    uint64_t  m_ht_size;
    uint8_t   m_log2_macs_in_block;    
  };
};

}
}
}

#endif // GMTCORE_H
