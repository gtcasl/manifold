#include "mcp-iris.h"

#include "mcp-cache/coh_mem_req.h"
#include "mcp-cache/coherence/MESI_enum.h"

using namespace manifold::uarch;
using namespace manifold::mcp_cache_namespace;

MCPSimLen :: MCPSimLen(int cacheLineSize, int coh_msg_type, int mem_msg_type, int credit_msg_type, std::vector<int>& mc_nodes) :
          m_cacheLineSize(cacheLineSize), m_COH(coh_msg_type), m_MEM(mem_msg_type), m_CREDIT(credit_msg_type)
{
    for(vector<int>::iterator it = mc_nodes.begin(); it != mc_nodes.end(); ++it)
        m_mc_nodes.insert(*it);
}


int MCPSimLen :: get_simulated_len(NetworkPacket* pkt)
{
    if(pkt->type == m_COH) { //coherence msg between MCP caches
	Coh_msg& msg = *((Coh_msg*)pkt->data);
	const int SZ_NO_DATA = 34; //32B addr + 2B overhead.
	const int SZ_WITH_DATA = 34 + m_cacheLineSize; //32B addr + 2B overhead + line

		switch(msg.msg) {
		    case MESI_CM_I_to_E:
		    case MESI_CM_I_to_S:
		    case MESI_CM_E_to_I:
		    case MESI_CM_M_to_I:
		    case MESI_CM_UNBLOCK_I:
			return SZ_NO_DATA;

		    case MESI_CM_UNBLOCK_I_DIRTY:
			return SZ_WITH_DATA;

		    case MESI_CM_UNBLOCK_E:
		    case MESI_CM_UNBLOCK_S:
		    case MESI_CM_CLEAN:
			return SZ_NO_DATA;

		    case MESI_CM_WRITEBACK:
		    case MESI_MC_GRANT_E_DATA:
		    case MESI_MC_GRANT_S_DATA:
			return SZ_WITH_DATA;

		    case MESI_MC_GRANT_I:
		    case MESI_MC_FWD_E:
		    case MESI_MC_FWD_S:
		    case MESI_MC_DEMAND_I:
			return SZ_NO_DATA;

		    case MESI_CC_E_DATA:
		    case MESI_CC_M_DATA:
		    case MESI_CC_S_DATA:
			return SZ_WITH_DATA;
		    default:
			assert(0);
			break;
		}
    }
    else if(pkt->type == m_MEM) { //memory msg between cache and MC
		const int SZ_NO_DATA = 36; //32B addr + 4B overhead.
		const int SZ_WITH_DATA = 36 + m_cacheLineSize; //32B addr + 4B overhead + line


		if(m_mc_nodes.find(pkt->get_dst()) != m_mc_nodes.end()) { //destination is mc; this is a request
		    manifold::mcp_cache_namespace::Mem_msg& msg = *((manifold::mcp_cache_namespace::Mem_msg*)pkt->data);

		    if(msg.is_read()) //LOAD request
			return SZ_NO_DATA;
		    else
			return SZ_WITH_DATA;
		}
		else {
		    return SZ_WITH_DATA;
		}
    }
    else {
	assert(0);
    }

}


MCPVnet :: MCPVnet(int coh_msg_type, int mem_msg_type, int credit_msg_type, std::vector<int>& mc_nodes) :
      m_COH(coh_msg_type), m_MEM(mem_msg_type), m_CREDIT(credit_msg_type)
{
    for(vector<int>::iterator it = mc_nodes.begin(); it != mc_nodes.end(); ++it)
        m_mc_nodes.insert(*it);
}



//Requests are assigned to virtual net 0; replies virtual net 1.
int MCPVnet :: get_virtual_net(NetworkPacket* pkt)
{
    if(pkt->type == m_COH) { //coherence msg between MCP caches
	Coh_msg& msg = *((Coh_msg*)pkt->data);
        if(msg.type == Coh_msg :: COH_REQ)
            return 0;
        else if(msg.type == Coh_msg :: COH_RPLY)
            return 1;
        else {
            assert(0);
        }
    }
    else if(pkt->type == m_MEM) { //memory msg between cache and MC
        //for memory messages, we determine whether it's a request or reply based on whether the
	//destination if a memory controller
	if(m_mc_nodes.find(pkt->get_dst()) != m_mc_nodes.end()) { //destination is mc; this is a request
            return 0;
	}
	else {
            return 1;
	}
    }
    else {
	assert(0);
    }
}
