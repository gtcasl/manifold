#include "cache_req.h"


using namespace std;

namespace manifold {
namespace simple_cache {


Cache_req::Cache_req()
{
    preqWrapper = 0;
}



Cache_req::Cache_req (int src, paddr_t ad, Mem_optype_t op)
{
    source_id = src;
    addr = ad;
    op_type = op;
    preqWrapper = 0;
}



Mem_req::Mem_req (int src, paddr_t addr, Mem_optype_t op_type)
{
   this->originator_id = src;
   this->source_id = src;
   this->addr = addr;
   this->op_type = op_type;
}

Mem_req::~Mem_req (void)
{
}



ostream& operator<<(ostream& stream, const Cache_req& creq)
{
    const char* pstr;
    switch (creq.op_type) {
	case OpMemLd:
	    pstr = "LD";
	    break;
	case OpMemSt:
	    pstr = "ST";
	    break;
	default:
	    assert(0);
	    break;
    }

    stream << "cache_req: " << " src_id= " << creq.source_id << " addr= 0x" <<hex<< creq.addr <<dec
           << " " << pstr;
}

ostream& operator<<(ostream& stream, const Mem_req& mreq)
{
    const char* pstr = 0;
    switch (mreq.op_type) {
	case OpMemLd:
	    pstr = "LD";
	    break;
	case OpMemSt:
	    pstr = "ST";
	    break;
	default:
	    assert(0);
	    break;
    }

    stream << "mem_req: " << " org_id= " << mreq.originator_id << " src_id= " << mreq.source_id << " src_port= " << mreq.source_port
           << " dst_id= " << mreq.dest_id << " dst_port= " << mreq.dest_port<< " addr= 0x" <<hex<< mreq.addr <<dec
           << " " << pstr;
}


} //namespace simple_cache
} //namespace manifold

