#include "kernel/component.h"
#include "kernel/manifold.h"
#include "dram_sim.h"
//#include "Transaction.h"

#include <iostream>

using namespace std;
using namespace manifold::kernel;

namespace manifold {
namespace dramsim {

int Dram_sim:: MEM_MSG_TYPE = -1;
int Dram_sim :: CREDIT_MSG_TYPE = -1;
bool Dram_sim :: Msg_type_set = false;


Dram_sim::Dram_sim (int nid, const Dram_sim_settings& dram_settings, Clock& clk) :
    m_nid(nid)
{
    assert(Msg_type_set);
    assert(MEM_MSG_TYPE != CREDIT_MSG_TYPE);

    m_send_st_response = dram_settings.send_st_resp;
    downstream_credits = dram_settings.downstream_credits;

    /* instantiate the DRAMSim module */
    mem = new MultiChannelMemorySystem(dram_settings.dev_filename, dram_settings.mem_sys_filename, ".", "res", dram_settings.size);

    mc_map = NULL;

    /* create and register DRAMSim callback functions */
    read_cb = new Callback<Dram_sim, void, unsigned, uint64_t, uint64_t>(this, &Dram_sim::read_complete);
    write_cb = new Callback<Dram_sim, void, unsigned, uint64_t, uint64_t>(this, &Dram_sim::write_complete);
    mem->RegisterCallbacks(read_cb, write_cb, NULL);

    //register with clock
    Clock :: Register(clk, this, &Dram_sim::tick, (void(Dram_sim::*)(void)) 0 );

    //stats
    stats_n_reads = 0;
    stats_n_writes = 0;
    stats_n_reads_sent = 0;
    stats_totalMemLat = 0;

#ifdef DRAMSIM_UTEST
    completed_writes = 0;
#endif
}


void Dram_sim::read_complete(unsigned id, uint64_t address, uint64_t done_cycle)
{
    //cout << "@ " << m_clk->NowTicks() << " (local) " << Manifold::NowTicks() << " (default), read complete\n";
    map<uint64_t, list<Request> >::iterator it = m_pending_reqs.find(address);
    assert (it != m_pending_reqs.end());

    Request req = m_pending_reqs[address].front();
    m_pending_reqs[address].pop_front();
    if (it->second.size() == 0)
    m_pending_reqs.erase(it);

    assert(req.read);
    assert(req.addr == address);

    m_completed_reqs.push_back(req);
}


void Dram_sim::write_complete(unsigned id, uint64_t address, uint64_t done_cycle)
{
    map<uint64_t, list<Request> >::iterator it = m_pending_reqs.find(address);
    assert (it != m_pending_reqs.end());

    Request req = m_pending_reqs[address].front();
    m_pending_reqs[address].pop_front();
    if (it->second.size() == 0)
    m_pending_reqs.erase(it);

    assert(req.read == false);
    assert(req.addr == address);

    //move from pending buffer to completed buffer
    if (m_send_st_response) {
    m_completed_reqs.push_back(req);
    }

#ifdef DRAMSIM_UTEST
    completed_writes++;
#endif
}


void Dram_sim :: try_send_reply()
{
    if ( !m_completed_reqs.empty() && downstream_credits > 0) {
        //stats
    stats_n_reads_sent++;
    stats_totalMemLat += (m_clk->NowTicks() - m_completed_reqs.front().r_cycle);

    Request req = m_completed_reqs.front();
    m_completed_reqs.pop_front();

    uarch::Mem_msg mem_msg(req.gaddr, req.read);

    manifold::uarch::NetworkPacket * pkt = (manifold::uarch::NetworkPacket*)(req.extra);
    pkt->type = MEM_MSG_TYPE;
    *((uarch::Mem_msg*)(pkt->data)) = mem_msg;
    pkt->data_size = sizeof(uarch::Mem_msg);

#ifdef DBG_DRAMSIM
    cout << "@ " << m_clk->NowTicks() << " MC " << m_nid << " sending reply: addr= " << hex << req.gaddr << dec << " destination= " << pkt->get_dst() << endl;
#endif
    Send(PORT0, pkt);
    downstream_credits--;
    }
}


void Dram_sim :: send_credit()
{
    manifold::uarch::NetworkPacket *credit_pkt = new manifold::uarch::NetworkPacket();
    credit_pkt->type = CREDIT_MSG_TYPE;
    Send(PORT0, credit_pkt);
}


bool Dram_sim::limitExceeds()
{
    //TODO this is the request reply dependency
    // PendingRequest has to be limited. Cannot exceed indefinitely
    return (m_completed_reqs.size() > 8); // some low threshold
}


void Dram_sim::tick()
{
    //cout << "Dram sim tick(), t= " << m_clk->NowTicks() << endl;
    //start new transaction if there is any and the memory can accept
    if (!m_incoming_reqs.empty() && mem->willAcceptTransaction() && !limitExceeds()) {
    // if limit exceeds, stop sending credits. interface will stop eventually
    Request req = m_incoming_reqs.front();
    m_incoming_reqs.pop_front();

    mem->addTransaction(!req.read, req.addr);
#ifdef DBG_DRAMSIM
    cout << "@ " << m_clk->NowTicks() << " MC " << m_nid << ": transaction of address " << hex << req.gaddr << dec << " is pushed to memory" << endl;
#endif
    //move from input buffer to pending buffer
        m_pending_reqs[req.addr].push_back(req);
        send_credit();
    }

    mem->update();
    try_send_reply();

}

void Dram_sim :: set_mc_map(manifold::uarch::DestMap *m)
{
    this->mc_map = m;
}

void Dram_sim :: print_stats(ostream& out)
{
    out << "***** DRAMSim2 " << m_nid << " *****" << endl;
    out << "  Total Reads Received= " << stats_n_reads << endl;
    out << "  Total Writes Received= " << stats_n_writes << endl;
    out << "  Total Reads Sent Back= " << stats_n_reads_sent << endl;
    out << "  Avg Memory Latency= " << (double)stats_totalMemLat / stats_n_reads_sent << endl;
    out << "  Reads per source:" << endl;
    for(map<int, unsigned>::iterator it=stats_n_reads_per_source.begin(); it != stats_n_reads_per_source.end();
           ++it) {
        out << "    " << it->first << ": " << it->second << endl;
    }
    out << "  Writes per source:" << endl;
    for(map<int, unsigned>::iterator it=stats_n_writes_per_source.begin(); it != stats_n_writes_per_source.end();
           ++it) {
        out << "    " << it->first << ": " << it->second << endl;
    }

    mem->printStats(true);
}



} // namespace dramsim
} //namespace manifold
