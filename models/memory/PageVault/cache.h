#ifndef CACHE_H
#define CACHE_H

#include "kernel/manifold.h"
#include "kernel/component.h"
#include "utils.h"

namespace manifold {

template <typename RID>
class TCache : public refcounted {
public:  
  class ICallback : public refcounted {
  public:
    ICallback() {}
    virtual ~ICallback() {}
    virtual void onComplete(const RID& id, uint64_t addr, bool is_hit) = 0;
  };
  
  template <typename T>
  class TCallback : public ICallback {
  private:
    typedef void (T::*Pfn)(const RID& id, uint64_t addr, bool is_hit);
    T * m_obj;
    Pfn m_pfn;
    
  public:
    TCallback(T * obj, Pfn pfn) : m_obj(obj), m_pfn(pfn) {}
    ~TCallback() {}
    void onComplete(const RID& id, uint64_t addr, bool is_hit) override {
      (const_cast<T*>(m_obj)->*m_pfn)(id, addr, is_hit);
    }   
  };
  
  TCache(ICallback* cb, 
        uint64_t capacity, 
        uint32_t block_size,
        uint32_t assoc,
        uint32_t cycles = 1, 
        uint32_t ports = 1) 
    : m_cb(cb) {
    //--
    cb->add_ref();
    typename TClkDevice<request_t>::ICallback* dev_cb = new DeviceCB(this, &TCache::onComplete);
    m_device = new TClkDevice<request_t>(dev_cb, cycles, ports);
    dev_cb->release();
    
    //--
    m_block_bits = fastLog2(block_size);
    uint32_t num_blocks = capacity >> m_block_bits;
    uint32_t num_sets = num_blocks / assoc;
    m_sets_bits  = fastLog2(num_sets);
    m_assoc_bits = fastLog2(assoc);
    assert(num_blocks == 1 << (m_sets_bits + m_assoc_bits));
    
    //--
    assert(m_assoc_bits <= MAX_ASSOC_BITS);
    m_sets = new set_t[num_sets];
    for (uint32_t i = 0; i < num_sets; ++i) {
      set_t& set = m_sets[i];
      set.tags = new tag_t[assoc];      
      for (uint32_t j = 0; j < assoc; ++j) {
        set.tags[j].valid = 0;
        set.tags[j].dirty = 0;
        set.tags[j].pos   = j;
        set.tags[j].freq  = 0;
      }    
    }
    m_available_space = num_blocks;
  }
  
  ~TCache() {
    for (uint32_t i = 0, n = 1 << m_sets_bits; i < n; ++i) {
      set_t& set = m_sets[i];
      for (int j = 0, m = m_assoc_bits; j < m; ++j) {
        delete [] set.tags;
      }
    }
    delete [] m_sets;  
    delete m_device;
    m_cb->release();
  }
  
  uint32_t get_available_space() {
    return m_available_space;
  }
  
  bool submit(uint64_t addr, const RID& id, bool write) {
    return m_device->submit({addr, id, write});
  }
  
  bool insert(uint64_t addr, uint64_t* victim_addr, bool dirty) {
    assert(victim_addr != nullptr);
    uint32_t set_idx = this->get_set(addr);
    uint32_t tag = this->get_tag(addr);
    
    int dest = -1;
    int victim = -1;
    set_t& set = m_sets[set_idx];
    for (uint32_t i = 0, n = 1 << m_assoc_bits, lru_pos = n - 1; i < n; ++i) {
      if (set.tags[i].valid) {      
        if (set.tags[i].pos == lru_pos) {
          dest = i;          
          victim = i;
          break;
        }
      } else {
        dest = i;
        break;
      }
    }     
    
    if (victim != -1) {
      ++m_available_space;
      // return victim on writeback only
      if (set.tags[victim].dirty) 
        *victim_addr = ((set.tags[victim].tag << m_sets_bits) | set_idx) << m_block_bits;
      else
        victim = -1;
    }
    
    // update cache
    assert(dest != -1);
    set.tags[dest].tag   = tag;
    set.tags[dest].valid = 1;
    set.tags[dest].dirty = dirty;
    set.update(dest, 1 << m_assoc_bits); 
    
    --m_available_space;
    assert(m_available_space >= 0);
    
    return (victim == -1);
  }
  
  bool insert(uint64_t addr, uint64_t freq, uint64_t* victim_addr, bool dirty) {
    assert(victim_addr != nullptr);
    uint32_t set_idx = this->get_set(addr);
    uint32_t tag = this->get_tag(addr);
    
    int dest = -1;
    int victim = -1;
    set_t& set = m_sets[set_idx];
    uint64_t victim_freq = ~0ull;
    for (uint32_t i = 0, n = 1 << m_assoc_bits, lru_pos = n - 1; i < n; ++i) {
      if (set.tags[i].valid) {      
        if (set.tags[i].freq < victim_freq) {
          dest = i;          
          victim = i;
          victim_freq = set.tags[i].freq;
        }
      } else {
        dest = i;
        break;
      }
    }     
    
    if (victim != -1) {
      ++m_available_space;
      // return victim on writeback only
      if (set.tags[victim].dirty) 
        *victim_addr = ((set.tags[victim].tag << m_sets_bits) | set_idx) << m_block_bits;
      else
        victim = -1;
    }
    
    // update cache
    assert(dest != -1);
    set.tags[dest].tag   = tag;
    set.tags[dest].valid = 1;
    set.tags[dest].dirty = dirty;
    set.tags[dest].freq  = freq;
    set.update(dest, 1 << m_assoc_bits); 
    
    --m_available_space;
    assert(m_available_space >= 0);
    
    return (victim == -1);
  }
  
  void invalidate(uint64_t addr) {
    uint32_t set_idx = this->get_set(addr);
    uint32_t tag = this->get_tag(addr);
    set_t& set = m_sets[set_idx];
    for (uint32_t i = 0, n = 1 << m_assoc_bits; i < n; ++i) {
      if (set.tags[i].valid && set.tags[i].tag == tag) {
        set.tags[i].valid = 0;
        ++m_available_space;
        return;
      }
    }
  }
  
  void tick() {
    m_device->tick();
  }
  
protected:
  
  enum {
    MAX_ASSOC_BITS = 30, 
  };
  
  struct request_t {
    uint64_t addr;
    RID id;
    bool write;
  };
  
  struct tag_t {    
    uint64_t tag;
    uint32_t freq;
    uint32_t pos   : MAX_ASSOC_BITS;    
    uint32_t valid : 1;
    uint32_t dirty : 1;
  };
  
  typedef typename TClkDevice<request_t>:: template TCallback<TCache> DeviceCB;
  
  struct set_t {
    tag_t* tags;

    // move selected entry to MRU position    
    void update(uint32_t way, uint32_t num_ways) {      
      uint32_t curr_pos = tags[way].pos;
      for (uint32_t i = 0; i < num_ways; ++i) {
        if (tags[i].pos < curr_pos)
          ++tags[i].pos;
      }
      tags[way].pos = 0;      
    }
  };
  
  void onComplete(const request_t& rqst) {
    bool is_hit = this->get_impl(rqst.addr, rqst.write);
    m_cb->onComplete(rqst.id, rqst.addr, is_hit);
  }
  
  bool get_impl(uint64_t addr, bool write) {
    uint32_t set_idx  = this->get_set(addr);
    uint32_t tag = this->get_tag(addr);
    
    set_t& set = m_sets[set_idx];
    for (uint32_t i = 0, n = 1 << m_assoc_bits; i < n; ++i) {
      if (set.tags[i].valid && set.tags[i].tag == tag) {
        set.tags[i].dirty |= write ? 1 : 0;
        set.update(i, 1 << m_assoc_bits);
        return true;
      }
    }
    return false;
  }
  
  uint64_t get_tag(uint64_t addr) const {
    return addr >> (m_block_bits + m_sets_bits);
  }
  
  uint32_t get_set(uint64_t addr) const {
    return (addr >> m_block_bits) & ((1 << m_sets_bits)-1);
  } 
  
  uint32_t   m_block_bits; 
  uint32_t   m_sets_bits;
  uint32_t   m_assoc_bits;  
  set_t*     m_sets;    
  ICallback* m_cb;
  int        m_available_space;
  TClkDevice<request_t>* m_device;
};

}

#endif // CACHE_H
