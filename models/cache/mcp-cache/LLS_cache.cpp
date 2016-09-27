#include <assert.h>
#include "LLS_cache.h"
#include "LLP_cache.h"
#include "kernel/manifold.h"
#include "mux_demux.h"
#include "debug.h"

using namespace manifold::uarch;
using namespace manifold::kernel;


namespace manifold { 
namespace mcp_cache_namespace { 



LLS_cache :: LLS_cache (int nid, const cache_settings& parameters, const L2_cache_settings& settings) :
    L2_cache (nid, parameters, settings), m_mux(0)
{
    stats_cycles = 0;
}


LLS_cache :: ~LLS_cache (void)
{
}


void LLS_cache :: handle_llp_incoming (int port, Coh_msg* request)
{
    assert(port == PORT_LOCAL_L1);
    m_llp_incoming.push_back(request);
}


void LLS_cache :: tick()
{
    while(m_llp_incoming.size() > 0) {
        Coh_msg* msg = m_llp_incoming.front();
	m_llp_incoming.pop_front();
	process_incoming_coh(msg);
    }

    //stats
    stats_cycles++;
    unsigned tab_occupancy = my_table->get_occupancy();
    stats_table_occupancy += tab_occupancy;
    if(tab_occupancy == 0)
        stats_table_empty_cycles++;

    unsigned mshr_occupancy = mshr->get_occupancy();
    stats_mshr_occupancy += mshr_occupancy;
    if(mshr_occupancy == 0)
        stats_mshr_empty_cycles++;
}

void LLS_cache :: send_msg_to_l1(Coh_msg* msg)
{
    msg->src_id = node_id;
    msg->dst_port = manifold::uarch::LLP_ID;

    assert(l2_map);
    if (l2_map->get_page_offset_bits() > my_table->get_offset_bits())
        msg->addr = l2_map->get_global_addr(msg->addr, node_id);

    DBG_LLS_CACHE_ID(cout,  " sending msg= " << msg->msg << " to L1 node= " << msg->dst_id << " fwd= " << msg->forward_id << endl);

    if(msg->dst_id == node_id)
	Send(PORT_LOCAL_L1, msg);
    else {
	NetworkPacket* pkt = new NetworkPacket;
	pkt->type = COH_MSG;
	pkt->src = node_id;
	pkt->src_port = manifold::uarch::LLS_ID;
	pkt->dst = msg->dst_id;
	pkt->dst_port = msg->dst_port;
	*((Coh_msg*)(pkt->data)) = *msg;
	pkt->data_size = sizeof(Coh_msg);
	delete msg;

	manifold::kernel::Manifold::ScheduleClock(my_table->get_lookup_time(), *m_clk, &LLS_cache::add_to_output_buffer, this, pkt);
	#ifdef FORECAST_NULL
	m_msg_out_ticks.push_back(m_clk->NowTicks() + my_table->get_lookup_time());
	#endif
    }
}


void LLS_cache::get_from_memory (Coh_msg *request)
{
    Mem_msg req;
    req.type = Mem_msg :: MEM_REQ;

    assert(l2_map);
    if (l2_map->get_page_offset_bits() > my_table->get_offset_bits())
        req.addr = l2_map->get_global_addr(request->addr, node_id);
    else
        req.addr = request->addr;

    req.op_type = OpMemLd;
    req.src_id = node_id;
    req.src_port = manifold::uarch::LLS_ID;
    req.dst_id = mc_map->lookup(request->addr);

    DBG_LLS_CACHE_ID(cout,  " get from memory node " << req.dst_id << " for 0x" << hex << req.addr << dec << endl);

    NetworkPacket* pkt = new NetworkPacket;
    pkt->type = MEM_MSG;
    pkt->src = node_id;
    pkt->src_port = manifold::uarch::LLS_ID;
    pkt->dst = req.dst_id;
    pkt->dst_port = manifold::uarch::MEM_ID;

    *((Mem_msg*)(pkt->data)) = req;
    pkt->data_size = sizeof(Mem_msg);

    manifold::kernel::Manifold::ScheduleClock(my_table->get_lookup_time(), *m_clk, &LLS_cache::add_to_output_buffer, this, pkt);
    #ifdef FORECAST_NULL
    m_msg_out_ticks.push_back(m_clk->NowTicks() + my_table->get_lookup_time());
    #endif

    stats_read_mem++;
}


void LLS_cache::dirty_to_memory (paddr_t addr)
{

    DBG_LLS_CACHE_ID(cout, " dirty write to memory for 0x" << hex << addr << dec << endl);

    Mem_msg req;
    req.type = Mem_msg :: MEM_REQ;

    assert(l2_map);
    if (l2_map->get_page_offset_bits() > my_table->get_offset_bits())
        req.addr = l2_map->get_global_addr(addr, node_id);
    else
        req.addr = addr;

    req.op_type = OpMemSt;
    req.src_id = node_id;
    req.src_port = manifold::uarch::LLS_ID;
    req.dst_id = mc_map->lookup(addr);

    NetworkPacket* pkt = new NetworkPacket;
    pkt->type = MEM_MSG;
    pkt->src = node_id;
    pkt->src_port = manifold::uarch::LLS_ID;
    pkt->dst = req.dst_id;
    pkt->dst_port = manifold::uarch::MEM_ID;

    *((Mem_msg*)(pkt->data)) = req;
    pkt->data_size = sizeof(Coh_msg);


    manifold::kernel::Manifold::ScheduleClock(my_table->get_lookup_time(), *m_clk, &LLS_cache::add_to_output_buffer, this, pkt);
    #ifdef FORECAST_NULL
    m_msg_out_ticks.push_back(m_clk->NowTicks() + my_table->get_lookup_time());
    #endif

    stats_dirty_to_mem++;
}


void LLS_cache :: add_to_output_buffer(NetworkPacket* pkt)
{
    m_downstream_output_buffer.push_back(pkt);
}


NetworkPacket* LLS_cache :: pop_from_output_buffer()
{
    NetworkPacket* pkt = 0;
    if(!m_downstream_output_buffer.empty() && m_downstream_credits > 0) {
        pkt = m_downstream_output_buffer.front();
	m_downstream_output_buffer.pop_front();
	m_downstream_credits--;
    }
//if(pkt != 0)
//cout << "LLS " << get_node_id() << " pop pkt= " << pkt << endl;
    return pkt;
}



void LLS_cache :: try_send()
{
//do nothing
}


void LLS_cache :: send_credit_downstream()
{
    m_mux->send_credit_downstream();
}


void LLS_cache :: print_stats(ostream& out)
{
    L2_cache :: print_stats(out);
    out << "LLS_cache stats:" << endl
	<< "    read mem= " << stats_read_mem << endl
        << "    dirty to mem= " << stats_dirty_to_mem << endl;
}




} // namespace mcp_cache_namespace
} // namespace manifold

