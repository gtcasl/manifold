#include "LLP_cache.h"
#include "mux_demux.h"
#include "kernel/manifold.h"
#include "debug.h"


#include <assert.h>
#include <iostream>


using namespace std;
using namespace manifold::uarch;
using namespace manifold::kernel;


namespace manifold { 
namespace mcp_cache_namespace {




LLP_cache :: LLP_cache (int nid, const cache_settings& parameters, const L1_cache_settings& settings) :
      L1_cache (nid, parameters, settings), m_mux(0)
{
}


LLP_cache :: ~LLP_cache (void)
{
}


void LLP_cache :: handle_local_LLS_request(int port, Coh_msg *request)
{
    assert(port == PORT_LOCAL_L2);
    //may be more than one requests at the same tick
    //assert(m_lls_requests.size() == 0);
    m_lls_requests.push_back(request);
    //process_peer_and_manager_request (request);
}


//Since LLP_cache has multiple input ports, to ensure determinism, tick() functions is used
//such that inputs from different ports are handled in a fixed order.
void LLP_cache :: tick()
{
    while(m_lls_requests.size() > 0) {
        Coh_msg* req = m_lls_requests.front();
	m_lls_requests.pop_front();
	process_peer_and_manager_request(req);
    }

    L1_cache :: tick();

}



void LLP_cache :: send_msg_to_peer_or_l2(Coh_msg* msg)
{
    msg->src_id = node_id;
    msg->src_port = LLP_ID;
    if(msg->dst_id == -1) { //destination is L2
	msg->dst_id = l2_map->lookup(msg->addr);
	msg->dst_port = LLS_ID;
    }
    else {
	msg->dst_port = LLP_ID;
    }

    DBG_LLP_CACHE_ID(cout,  " sending msg= " << msg->msg << " to node " << msg->dst_id << " port= " << msg->dst_port << " addr= " <<hex<< msg->addr <<dec<< endl);

#ifdef DBG_MCP_CACHE_LLP_CACHE
//cout << "LLP_cache node " << node_id << " sending msg= " << msg->msg << " to node " << msg->dst_id << " port= " << msg->dst_port << " addr= " <<hex<< msg->addr <<dec<< endl;
#endif
    if(msg->dst_id == get_node_id()) {
        assert(msg->dst_port == LLS_ID);
	Send(PORT_LOCAL_L2, msg); //send to local L2
    }
    else {
	NetworkPacket* pkt = new NetworkPacket;
	pkt->type = COH_MSG;
	pkt->src = node_id;
	pkt->src_port = msg->src_port;
	pkt->dst = msg->dst_id;
	pkt->dst_port = msg->dst_port;
	*((Coh_msg*)(pkt->data)) = *msg;
	pkt->data_size = sizeof(Coh_msg);
	delete msg;

	assert(my_table->get_lookup_time() > 0);
	manifold::kernel::Manifold::ScheduleClock(my_table->get_lookup_time(), *m_clk, &LLP_cache::add_to_output_buffer, this, pkt);
	#ifdef FORECAST_NULL
	m_msg_out_ticks.push_back(m_clk->NowTicks() + my_table->get_lookup_time());
	#endif
    }
}



void LLP_cache :: add_to_output_buffer(NetworkPacket* pkt)
{
    m_downstream_output_buffer.push_back(pkt);
}


//try_send() is called in L1_cache::internal_handle_peer_and_manager_request(), so must overwrite
void LLP_cache :: try_send()
{
//do nothing
}

void LLP_cache :: send_credit_downstream()
{
    DBG_LLP_CACHE_ID(cout,  " sending credit.\n");

#ifdef DBG_MCP_CACHE_LLP_CACHE
//cout << "LLP_cache node " << node_id << " sending credit.\n";
#endif
    m_mux->send_credit_downstream();
}


NetworkPacket* LLP_cache :: pop_from_output_buffer()
{
    NetworkPacket* pkt = 0;
    if(!m_downstream_output_buffer.empty() && m_downstream_credits > 0) {
        pkt = m_downstream_output_buffer.front();
	m_downstream_output_buffer.pop_front();
	m_downstream_credits--;
    }
    return pkt;
}


void LLP_cache :: print_stats(ostream& out)
{
    L1_cache :: print_stats(out);
}










} // namespace mcp_cache_namespace
} // namespace manifold

