#ifndef UTILS_H
#define UTILS_H

#include <queue>  
#include "fifo_map.h"
#include "kernel/manifold.h"

namespace manifold {

#define DBG(level, format, ...) do { \
    DbgPrint(level, "DBG: " format, ##__VA_ARGS__); \
  } while (0)

#define __countof(a) (sizeof(a) / sizeof(a[0])) 

#define __iceildiv(a, b) \
  (isPow2(b) ? (((a) + (b)-1)  >> fastLog2(b)) : (((a) + (b)-1) / (b)))

#define __ifloordiv(a, b) \
  (isPow2(b) ? ((a) >> fastLog2(b)) : ((a) / (b)))

// Log2 template function
template <unsigned N, unsigned P = 0> struct Log2 {
  enum { value = Log2<(N >> 1), (P + 1)>::value };
};
template <unsigned P> struct Log2<0, P> {
  enum { value = P };
};
template <unsigned P> struct Log2<1, P> {
  enum { value = P };
};

inline int isPow2(int v) {
  return ((v != 0) && !(v & (v - 1)));
}

void SetDbgLevel(int level);

int getDbgLevel();

void DbgPrint(int level, const char *format, ...);

void DbgDumpCallstack(int levels);

unsigned fastLog2(unsigned value);

struct ktree_t {  
  ktree_t(uint32_t numleaves, uint32_t k) 
    : k(k) {
    height = height_from_leaves(numleaves, k);
    assert(height > 0);
    nodes  = nodes_from_height(height, k); 
    leaf_start = nodes_from_height(height-1, k);
  }
  
  uint32_t get_parent(uint32_t i) const {
    uint32_t n = (i - 1) / k;
    assert(i < nodes && n < nodes);
    return n;
  }
  
  uint32_t get_child(uint32_t i, uint32_t c) const {
    uint32_t n = k * i + 1 + c;
    assert(i < nodes && c < k && n < nodes);
    return n;
  }
  
  uint32_t node_from_leaf(uint32_t leaf_idx) const {
    uint32_t node = leaf_start + leaf_idx;
    assert(node < nodes);
    return node;
  }
  
  static uint32_t height_from_leaves(uint32_t l, uint32_t k) {
    return ceil(log(l) / log(k));
  }
  
  static uint32_t nodes_from_height(uint32_t h, uint32_t k) {
    return floor((pow(k, h+1)-1) / (k-1));
  }
  
  uint32_t k;
  uint32_t height;
  uint32_t nodes;
  uint32_t leaf_start;
};
  
class refcounted {
public:  
  void add_ref() const {
    ++m_refcount;
  }

  long release() const {
    if (--m_refcount == 0) {
      delete this;
    }
    return m_refcount;
  }
  
  long get_refcount() const {
    return m_refcount;
  }
    
protected:
  refcounted() : m_refcount(1) {}
  virtual ~refcounted() {}    
  
private:
  mutable long m_refcount;
};

template <typename Task>
class TClkDevice {
public:
  
  class ICallback : public refcounted {
  public:
    ICallback() {}
    virtual ~ICallback() {}
    virtual void onComplete(const Task& task) = 0;
  };
  
  template <typename T>
  class TCallback : public ICallback {
  private:
    typedef void (T::*Pfn)(const Task& task);
    T * m_obj;
    Pfn m_pfn;
    
  public:
    TCallback(T * obj, Pfn pfn) : m_obj(obj), m_pfn(pfn) {}
    ~TCallback() {}
    void onComplete(const Task& task) override {
      (const_cast<T*>(m_obj)->*m_pfn)(task);
    }
  };
  
  TClkDevice(ICallback* cb, int cycles = 1, int ports = 1)
    : m_cb(cb)
    , m_cycles(cycles)
    , m_ports(ports)
    , m_inbufsz(0)
    , m_ticks(0)
    , m_remaining(0) {
    cb->add_ref();
  }
  virtual ~TClkDevice() {
    m_cb->release();
  }
  
  int get_cycles() {
    return m_cycles;
  }
  
  int get_ports() {
    return m_ports;
  }
  
  bool submit(const Task& task, int batch_size = 1) {
    assert(batch_size > 0);
    if (m_inbufsz == m_ports)
      return false;    
    uint64_t expire_time = m_ticks + m_cycles + (batch_size - 1);
    m_queue.push({task, expire_time});
    this->submit(batch_size);    
    return true;
  }
  
  void tick() {
    ++m_ticks;
    m_inbufsz = 0;
    if (!m_queue.empty()) {
      const entry_t& entry = m_queue.front();
      if (entry.tick == m_ticks) {        
        m_cb->onComplete(entry.task);
        m_queue.pop();
      }      
      if (m_remaining) {
        this->submit(m_remaining);
      }
    }
  }
  
protected:
  
  struct entry_t {
    entry_t(const Task& task, uint64_t time) 
      : task(task)
      , tick(time) {}
    Task task;
    uint64_t tick;
  };
  
  void submit(int batch_size) {
    int available_size = m_ports - m_inbufsz;
    int chunk_size = (batch_size > available_size) ? available_size : batch_size;
    m_inbufsz += chunk_size;
    m_remaining = batch_size - chunk_size;
  }

  ICallback* m_cb;
  int m_cycles;
  int m_ports;
  int m_inbufsz;  
  int m_remaining;
  uint64_t m_ticks;
  std::queue<entry_t> m_queue;
};

class IMemoryController : public kernel::Component {
public:  
  class ICallback  : public refcounted {
  public:
    ICallback() {}
    virtual ~ICallback() {}
    virtual void onComplete(int tag) = 0;
  };
  
  template <typename T>
  class TCallback : public ICallback {
  private:
    typedef void (T::*Pfn)(int tag);
    T * m_obj;
    Pfn m_pfn;
    
  public:
    TCallback(T * obj, Pfn pfn) : m_obj(obj), m_pfn(pfn) {}
    ~TCallback() {}
    void onComplete(int tag) override {
      (const_cast<T*>(m_obj)->*m_pfn)(tag);
    }
  };
  
  IMemoryController() {}
  virtual ~IMemoryController() {}
  
  virtual int send_request(uint64_t addr, bool is_read) = 0;    
  virtual void print_stats(std::ostream& out) = 0;
};

}

#endif // UTILS_H
