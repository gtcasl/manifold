#include "MESI_LLS_cache.h"
#include "coherence/MESI_manager.h"
#include "coh_mem_req.h"

#include <iostream>

namespace manifold {
namespace mcp_cache_namespace {

//! Subclassing MESI_manager to implement message sending.
class MESI_LLS_cache_manager : public MESI_manager {
public:
    MESI_LLS_cache_manager(int id, MESI_LLS_cache* cache) : MESI_manager(id), m_l2_cache(cache)
    {
        stats_coh_msg_to_network = 0;
	stats_coh_data_msg_to_network = 0;
    }

    bool process_lower_client_request(void* req, bool first);
    void process_lower_client_reply(void* req);

    bool is_invalidation_request(void* req);

    //stats
    unsigned get_stats_coh_msg_to_network() { return stats_coh_msg_to_network; }
    unsigned get_stats_coh_data_msg_to_network() { return stats_coh_data_msg_to_network; }

protected:
    virtual void sendmsg(bool req, MESI_messages_t msg, int dest_id, int fwd_id);
    virtual void client_writeback();
    virtual void invalidate();
    virtual void ignore();

private:
    MESI_LLS_cache* m_l2_cache;

    //stats
    unsigned stats_coh_msg_to_network; //coherence related messages sent to network.
    unsigned stats_coh_data_msg_to_network; //coherence message that also carries data
};





bool MESI_LLS_cache_manager :: process_lower_client_request(void* request, bool first)
{
    Coh_msg* req = (Coh_msg*)request;
    assert(req->type == Coh_msg :: COH_REQ);
    if(first)
	process(req->msg, req->src_id);
    return (req_pending() == false);
}

void MESI_LLS_cache_manager :: process_lower_client_reply(void* rply)
{
    Coh_msg* reply = (Coh_msg*)rply;
    assert(reply->type == Coh_msg :: COH_RPLY);
    process(reply->msg, reply->src_id);
}

bool MESI_LLS_cache_manager :: is_invalidation_request(void* request)
{
    Coh_msg* req = (Coh_msg*)request;

    return (req->msg == MESI_CM_E_to_I || req->msg == MESI_CM_M_to_I);
}




//! @param \c req  Whether it's an initial request or a reply.
void MESI_LLS_cache_manager :: sendmsg(bool req, MESI_messages_t msg, int dest_id, int fwd_id)
{
//std::cout << "MESI_L2_cache_manager sendmsg(), msg= " << msg << " dest= " << dest_id << " fwd= " << fwd_id << std::endl;
    Coh_msg* message = new Coh_msg();
    if(req)
	message->type = Coh_msg :: COH_REQ;
    else
	message->type = Coh_msg :: COH_RPLY;
    message->addr = m_l2_cache->get_hash_entry_by_idx(id)->get_line_addr();
    message->msg = msg;
    message->dst_id = dest_id;
    message->forward_id = fwd_id;

    //stats - coherence related messages
    if(dest_id != m_l2_cache->get_node_id()) {
	switch(msg) {
	    case MESI_MC_GRANT_I:
	    case MESI_MC_FWD_E:
	    case MESI_MC_FWD_S:
	    case MESI_MC_DEMAND_I:
	        stats_coh_msg_to_network++;
		break;
            case MESI_MC_GRANT_E_DATA:
            case MESI_MC_GRANT_S_DATA:
	        stats_coh_data_msg_to_network++;
		break;
	    default:
		break;
	}//switch
    }

    m_l2_cache->send_msg_to_l1(message);
}


void MESI_LLS_cache_manager :: client_writeback()
{
    m_l2_cache->client_writeback(this);
}

void MESI_LLS_cache_manager :: invalidate()
{
    m_l2_cache->invalidate(this);
}

void MESI_LLS_cache_manager :: ignore()
{
    m_l2_cache->ignore(this);
}



MESI_LLS_cache :: MESI_LLS_cache (int nid, const cache_settings& parameters, const L2_cache_settings& settings) :
    LLS_cache (nid, parameters, settings)
{
    managers.resize(my_table->get_num_entries());
    hash_entries.resize(my_table->get_num_entries());

    for(unsigned i=0; i<managers.size(); i++) {
        managers[i] = new MESI_LLS_cache_manager(i, this);
    }

    vector<hash_set*> all_sets;
    my_table->get_sets(all_sets);

    for(unsigned i=0; i<all_sets.size(); i++) {
        vector<hash_entry*> entries;
	all_sets[i]->get_entries(entries);
	for(unsigned e=0; e<entries.size(); e++) {
	    int idx = entries[e]->get_idx();
	    assert(idx >= 0 && idx < managers.size());

	    hash_entries[idx] = entries[e];
	}
    }

    for(unsigned i=0; i<managers.size(); i++) {
        assert(hash_entries[i]->get_idx() == i);
	assert(managers[i]->getManagerID() == i);
    }

}



/*
bool MESI_L2_cache :: start_eviction (hash_entry* entry)
{
#ifdef DBG_MCP_CACHE_L2_CACHE
cout << "MESI_L2_cache start eviction line " <<hex<< entry->get_line_addr() <<dec<< "\n";
#endif
    //managers[entry->get_idx()]->process(GET_EVICT, node_id);
    managers[entry->get_idx()]->Evict();
    return false;
}
*/


void MESI_LLS_cache :: print_stats(ostream& out)
{
    LLS_cache :: print_stats(out);

    unsigned total_coh_msg = 0;
    unsigned total_coh_data_msg = 0;

    for(unsigned i=0; i<managers.size(); i++) {
        MESI_LLS_cache_manager* m = dynamic_cast<MESI_LLS_cache_manager*>(managers[i]);
	total_coh_msg += m->get_stats_coh_msg_to_network();
	total_coh_data_msg += m->get_stats_coh_data_msg_to_network();
    }
    
    out << " MESI_LLS stats:" << endl
        << "   messages to network: " << total_coh_msg << " (coh) " 
				     << total_coh_data_msg << " (coh+data)" << endl;

}


} //namespace mcp_cache
} //namespace manifold


