#ifndef MANIFOLD_SIMPLE_CACHE_CACHE_REQ_H
#define MANIFOLD_SIMPLE_CACHE_CACHE_REQ_H

#include <iostream>
#include <stdint.h>

#include "kernel/component.h"

namespace manifold {
namespace simple_cache {

typedef uint64_t paddr_t;

typedef enum {
   OpMemLd,
   OpMemSt
} Mem_optype_t;


class PreqWrapperBase {
public:
    virtual void SendTick(manifold::kernel::Component* comp, int port, manifold::kernel::Ticks_t ticks) = 0;
};

template<typename T>
class GenericPreqWrapper : public PreqWrapperBase {
public:
    GenericPreqWrapper(T* r) : preq(r) {}
    void SendTick(manifold::kernel::Component* comp, int port, manifold::kernel::Ticks_t ticks)
    {
        comp->SendTick(port, preq, ticks);
    }

    T* preq;
};


class Cache_req {
public:
    int source_id;
    paddr_t addr;
    Mem_optype_t op_type; 
    PreqWrapperBase* preqWrapper; //wrapper of processor request.


    Cache_req();  //for deserialization
    Cache_req (int src, paddr_t addr, Mem_optype_t op_type);

    //paddr_t get_addr() { return addr; }
    //bool is_read() { return op_type == OpMemLd; }

    friend std::ostream& operator<<(std::ostream& stream, const Cache_req& creq);
};

class Mem_req {
public:
    int originator_id;
    int source_id;
    int source_port;
    int dest_id;
    int dest_port;
    paddr_t addr;
    Mem_optype_t op_type;

    Mem_req() {}  //for deserialization
    Mem_req (int src, paddr_t addr, Mem_optype_t op_type);
    ~Mem_req (void);

    int get_src() { return source_id; }
    int get_src_port() { return source_port; }
    int get_dst() { return dest_id; }
    int get_dst_port() { return dest_port; }

    paddr_t get_addr() { return addr; }
    bool is_read() { return op_type == OpMemLd; }

    friend std::ostream& operator<<(std::ostream& stream, const Mem_req& mreq);
};


} //namespace simple_cache
} //namespace manifold

#endif // MANIFOLD_SIMPLE_CACHE_CACHE_REQ_H
