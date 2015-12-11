#include "MESI_L1_cache.h"
#include "coherence/MESI_client.h"
#include "coh_mem_req.h"

#include <iostream>

using namespace std;

namespace manifold {
namespace mcp_cache_namespace {

//! Subclassing MESI_client to implement message sending.
class MESI_L1_cache_client : public MESI_client {
public:
    MESI_L1_cache_client(int id, MESI_L1_cache* cache) : MESI_client(id), m_l1_cache(cache)
    {
        stats_coh_msg = 0;
    }

    unsigned get_stats_coh_msg() { return stats_coh_msg; }

protected:
    virtual void sendmsg(bool req, MESI_messages_t msg, int destId=-1);
    virtual void invalidate();

private:
    MESI_L1_cache* m_l1_cache;

    //stats
    unsigned stats_coh_msg; //coherence related messages.
};



//! @param \c req  Whether it's an initial request or a reply.
void MESI_L1_cache_client :: sendmsg(bool req, MESI_messages_t msg, int destId)
{
    Coh_msg* message = new Coh_msg();
    if(req)
	message->type = Coh_msg :: COH_REQ;
    else
	message->type = Coh_msg :: COH_RPLY;
    message->addr = m_l1_cache->get_hash_entry_by_idx(id)->get_line_addr();
    message->msg = msg;
    message->dst_id = destId;
//std::cout << "MESI_L1_cache_client sendmsg(), msg= " << msg << " dest= " << destId << std::endl;

    //stats - coherence related messages
    switch(msg) {
	case MESI_CM_I_to_E:
	    if(this->get_state() == MESI_C_I) {
		break;
	    }
	    else {
		assert(this->get_state() == MESI_C_S);
	    }
	case MESI_CM_E_to_I:
	case MESI_CM_M_to_I:
	case MESI_CM_UNBLOCK_I:
	case MESI_CM_UNBLOCK_I_DIRTY:
	case MESI_CM_UNBLOCK_E:
	case MESI_CM_UNBLOCK_S:
	case MESI_CM_CLEAN:
	case MESI_CM_WRITEBACK:
	    stats_coh_msg++;
	    break;
	default:
	    //do nothing
	    break;
    }//switch

    m_l1_cache->send_msg_to_peer_or_l2(message);
}


//! called when the client enters the I state.
void MESI_L1_cache_client :: invalidate()
{
    m_l1_cache->invalidate(this);
}



MESI_L1_cache :: MESI_L1_cache (int nid, cache_settings my_settings, const L1_cache_settings& settings) :
    L1_cache (nid, my_settings, settings)
{
    clients.resize(my_table->get_num_entries());
    hash_entries.resize(my_table->get_num_entries());

    for(unsigned i=0; i<clients.size(); i++) {
        clients[i] = new MESI_L1_cache_client(i, this);
    }

    //?????????????????????????? populating hash_entries can be done in L1_Cache ??????????????????
    vector<hash_set*> all_sets;
    my_table->get_sets(all_sets);

    for(unsigned i=0; i<all_sets.size(); i++) {
        vector<hash_entry*> entries;
	all_sets[i]->get_entries(entries);
	for(unsigned e=0; e<entries.size(); e++) {
	    int idx = entries[e]->get_idx();
	    assert(idx >= 0 && idx < clients.size());

	    hash_entries[idx] = entries[e];
	}
    }

    for(unsigned i=0; i<clients.size(); i++) {
        assert(hash_entries[i]->get_idx() == i);
	assert(clients[i]->getClientID() == i);
    }

}


void MESI_L1_cache :: invalidate (ClientInterface* client)
{
    hash_entries[client->getClientID()]->invalidate();
}


void MESI_L1_cache :: respond_with_default(Coh_msg* request)
{
    assert(request->msg == MESI_MC_DEMAND_I);

    //reuse request
    request->type = Coh_msg :: COH_RPLY;
    request->msg = MESI_CM_UNBLOCK_I;
    request->dst_id = request->src_id;
    request->src_id = node_id;
    send_msg_to_peer_or_l2(request);
}


void MESI_L1_cache :: print_stats(ostream& out)
{
    L1_cache :: print_stats(out);

    unsigned total_coh_msg = 0;

    for(unsigned i=0; i<clients.size(); i++) {
        MESI_L1_cache_client* c = dynamic_cast<MESI_L1_cache_client*>(clients[i]);
	total_coh_msg += c->get_stats_coh_msg();
    }
    
    out << " MESI_L1_cache coherence messages: " << total_coh_msg << endl;

}




} //namespace mcp_cache
} //namespace manifold


