#ifndef MANIFOLD_MCPCACHE_CACHE_REQ_H
#define MANIFOLD_MCPCACHE_CACHE_REQ_H

#include "./coherence/MESI_enum.h"
#include "cache_types.h"
#include "kernel/manifold-decl.h"
#include "kernel/component-decl.h"
#include <iostream>

namespace manifold {
namespace mcp_cache_namespace {

typedef uint64_t paddr_t;

typedef enum {
   OpMemNoOp = 0,
   OpMemLd,
   OpMemSt
} mem_optype_t;



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


class cache_req {
   public:
      paddr_t addr;
      mem_optype_t op_type; 
      PreqWrapperBase* preqWrapper; //wrapper of processor request.

      cache_req();  //for deserialization
};



typedef enum
{
    INVALID_STALL = 0,
    //C_PERMISSION_STALL, // Most basic stall, waiting for permissions (initial miss or upgrade)
    //C_REPLACEMENT_STALL, // Replacement has begun for the block this request will take over
    C_LRU_BUSY_STALL, // Waiting for LRU to become non-transient before initiating eviction/issue
    //C_BEING_REPLACED_STALL, // Match on a block being evicted, wait to complete before re-acquiring
    C_TRANS_STALL, // Match on a block in a transient state without enough permissions
    C_PREV_PEND_STALL, // Previous request to the same block is in flight
    C_MSHR_STALL, // Out of MSHRs, must wait for one to clear

    //M_MEM_ACCESS_STALL, //stall for memory access to complete
    //M_CLIENT_TRANS_STALL, // Manager (or peer) req stalled from transient client state.
    //M_MANAGER_TRANS_STALL, // Manager (or peer) req stalled from transient paired lower-manager state. Only relivant for hierarchical
    //M_DOWNGRADE_STALL, //Manager req stalled while client forfiets permission (fwd or evict).  Only relivant for hierarchical
    COUNT_STALL,
} stall_type_t;





} //namespace mcp_cache_namespace
} //namespace manifold

#endif //MANIFOLD_MCPCACHE_CACHE_REQ_H
