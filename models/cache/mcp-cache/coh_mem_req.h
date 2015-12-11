#ifndef MANIFOLD_MCPCACHE_COH_MEM_REQ_H
#define MANIFOLD_MCPCACHE_COH_MEM_REQ_H

#include "cache_types.h"
#include "cache_req.h" //for paddr_t
#include <iostream>

namespace manifold {
namespace mcp_cache_namespace {


//! message sent to memory controller
struct Mem_msg {
    enum {MEM_REQ, MEM_RPLY};

    char type;
    paddr_t addr;
    mem_optype_t op_type;
    int src_id;
    int src_port;
    int dst_id;
    int dst_port;
    uint64_t get_addr() { return addr; }
    bool is_read() { return op_type == OpMemLd; }
    int get_src() { return src_id; }
    void set_src(int s) { src_id = s; }
    void set_src_port(int s) { src_port = s; }
    void set_dst(int d) { dst_id = d; }
    void set_dst_port(int d) { dst_port = d; }
    void set_mem_response() { type = MEM_RPLY; }
    //cache_msg_t msg;
    //int resolve_time;
};

//! directory-based coherence protocol message
struct Coh_msg {
    enum {COH_REQ, COH_RPLY};

    char type;
    paddr_t addr;
    int forward_id; //directory may ask client to forward
    int msg; //msg type
    int rw; // 0 for read; 1 for write
    int src_id;
    int src_port;
    int dst_id;
    int dst_port;
};


#if 0
class Coh_mem_req {
public:
    enum {MEM_REQ=0, MEM_RPLY, COH_REQ, COH_RPLY};
    int type;
    int source_id;
    int source_port;
    int dest_id;
    int dest_port;
    paddr_t addr;
    union {
	struct Mem_req mem;
	struct Coh_msg coh;
    } u;

    Coh_mem_req() {}  //for deserialization
    //Coh_mem_req (int rid, int sid, paddr_t addr, mem_optype_t op_type, cache_msg_t msg);
    ~Coh_mem_req () {}

    int get_src() { return source_id; }
    void set_src(int src) { source_id = src; }
    int get_src_port() { return source_port; }
    void set_src_port(int sport) { source_port = sport; }
    int get_dst() { return dest_id; }
    void set_dst(int dst) { dest_id = dst; }
    int get_dst_port() { return dest_port; }
    void set_dst_port(int dport) { dest_port = dport; }

    paddr_t get_addr() { return addr; }
    bool is_read()
    {
        if(type == MEM_REQ || type == MEM_RPLY)
	    return u.mem.op_type == OpMemLd;
	else
	    return u.coh.rw == 0;
    }

    //Allows memory controller to set the type.
    void set_mem_response()
    {
        type = MEM_RPLY;
    }

    int get_simulated_len()
    {
	switch(type) {
	    case MEM_REQ:
	        if(u.mem.op_type == OpMemLd)
		    return 48; //32B addr + 16B overhead
		else
		    return 80; //32B addr + 32B data + 16B overhead
		break;
	    case MEM_RPLY:
		return 80; //32B addr + 32B data + 16B overhead
		break;
	    case COH_REQ:
	    case COH_RPLY:
		//Improvements: different message can have different length.
	        return 34; //32B data + 2B overhead
	    default:
	        assert(0);
		break;
	}
    }

    //friend std::ostream& operator<<(std::ostream& stream, const mem_req& mreq);
};
#endif





} //namespace mcp_cache_namespace
} //namespace manifold




#endif //MANIFOLD_MCPCACHE_COH_MEM_REQ_H


