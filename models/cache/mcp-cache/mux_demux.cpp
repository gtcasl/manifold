#include "mux_demux.h"
#include "LLP_cache.h"
#include "LLS_cache.h"
#include "kernel/component.h"
#include "kernel/manifold.h"

using namespace manifold::kernel;
using namespace manifold::uarch;


namespace manifold {
namespace mcp_cache_namespace {

MuxDemux :: MuxDemux(Clock& clk, int credit_type) : CREDIT_MSG_TYPE(credit_type), m_clk(clk)
{
    //stats
    stats_num_llp_incoming_msg = 0;
    stats_num_lls_incoming_msg = 0;
    stats_num_llp_outgoing_msg = 0;
    stats_num_lls_outgoing_msg = 0;
    stats_num_llp_incoming_credits = 0;
    stats_num_lls_incoming_credits = 0;
    stats_num_outgoing_credits = 0;
}

void MuxDemux :: tick()
{
#if 0
    cout << "llp next msg= " << ((m_llp->m_msg_out_ticks.size() >0) ? m_llp->m_msg_out_ticks.front() : -1)
	<< " llp next credit= " << ((m_llp->m_credit_out_ticks.size() >0) ? m_llp->m_credit_out_ticks.front() : -1)
	<< " lls next msg= " << ((m_lls->m_msg_out_ticks.size() >0) ? m_lls->m_msg_out_ticks.front() : -1)
	<< " lls next credit= " << ((m_lls->m_credit_out_ticks.size() >0) ? m_lls->m_credit_out_ticks.front() : -1) <<"\n";
#endif

//cout << "Mux " << m_llp->get_node_id() << " tick(), @ " << m_clk.NowTicks() << endl;
    NetworkPacket* lls_pkt = m_lls->pop_from_output_buffer();
    if(lls_pkt) {
	Send(PORT_NET, lls_pkt);
	stats_num_lls_outgoing_msg++;
//cout << "OOOOOOOOOOOOOOOOOOOOOOOOOO, @ " << m_clk.NowTicks() << " mux send lls\n";
//cout.flush();
    }
    else {
	NetworkPacket* llp_pkt = m_llp->pop_from_output_buffer();
	if(llp_pkt) {
	    Send(PORT_NET, llp_pkt);
	    stats_num_llp_outgoing_msg++;
//cout << "OOOOOOOOOOOOOOOOOOOOOOOOOO, @ " << m_clk.NowTicks() << " mux send llp\n";
//cout.flush();
	}
    }
}



void MuxDemux :: send_credit_downstream()
{
    NetworkPacket* pkt = new NetworkPacket;
    pkt->type = CREDIT_MSG_TYPE;
    Send(PORT_NET, pkt);
//cout << "OOOOOOOOOOOOOOOOOOOOOOOOOO, @ " << m_clk.NowTicks() << " mux send credit\n";
//cout.flush();
    stats_num_outgoing_credits++;
}


void MuxDemux :: print_stats(ostream& out)
{
    out << "Mux stats:" << endl
	<< "  incoming msg: " << stats_num_llp_incoming_msg << " (LLP)  " 
	                      << stats_num_lls_incoming_msg << " (LLS)  "
			      << stats_num_llp_incoming_msg + stats_num_lls_incoming_msg << " (total)" << endl
	<< "  outgoing msg: " << stats_num_llp_outgoing_msg << " (LLP)  " 
	                      << stats_num_lls_outgoing_msg << " (LLS)  "
			      << stats_num_llp_outgoing_msg + stats_num_lls_outgoing_msg << " (total)" << endl
	<< "  incoming credits: " << stats_num_llp_incoming_credits << " (LLP)  " 
	                          << stats_num_lls_incoming_credits << " (LLS)  "
			          << stats_num_llp_incoming_credits + stats_num_lls_incoming_credits << " (total)" << endl
        << "  outgoing credits: " << stats_num_outgoing_credits << endl;
}


#ifdef FORECAST_NULL
void MuxDemux :: do_output_to_net_prediction()
{

    Ticks_t now = m_clk.NowTicks();

    manifold::kernel::BorderPort* bp = this->border_ports[PORT_NET];
    assert(bp);

    if(!m_llp->m_downstream_output_buffer.empty() || !m_lls->m_downstream_output_buffer.empty()) { //if output buffer has msg
	bp->update_output_tick(now);
        return;
    }

    //remove all old values
    while(m_llp->m_msg_out_ticks.size() > 0)
        if(m_llp->m_msg_out_ticks.front() < now)
	    m_llp->m_msg_out_ticks.pop_front();
	else
	    break;
    while(m_llp->m_credit_out_ticks.size() > 0)
        if(m_llp->m_credit_out_ticks.front() < now)
	    m_llp->m_credit_out_ticks.pop_front();
	else
	    break;

    while(m_lls->m_msg_out_ticks.size() > 0)
        if(m_lls->m_msg_out_ticks.front() < now)
	    m_lls->m_msg_out_ticks.pop_front();
	else
	    break;
    while(m_lls->m_credit_out_ticks.size() > 0)
        if(m_lls->m_credit_out_ticks.front() < now)
	    m_lls->m_credit_out_ticks.pop_front();
	else
	    break;

    //The earliest tick for possible outgoing msg/credit is the minimum of the following
    //3 values:
    //(1) earliest scheduled msg or credit
    //(2) min lookup time; note, if (1) exists, it must be smaller than (2)
    //(3) credit for unprocessed incoming msg

    //Find the earliest output msg or credit
    Ticks_t out_ticks[4];
    int count = 0;
    if(m_llp->m_msg_out_ticks.size() > 0)
        out_ticks[count++] =  m_llp->m_msg_out_ticks.front();
    if(m_llp->m_credit_out_ticks.size() > 0)
        out_ticks[count++] =  m_llp->m_credit_out_ticks.front();
    if(m_lls->m_msg_out_ticks.size() > 0)
        out_ticks[count++] =  m_lls->m_msg_out_ticks.front();
    if(m_lls->m_credit_out_ticks.size() > 0)
        out_ticks[count++] =  m_lls->m_credit_out_ticks.front();

    //find the minimum
    Ticks_t min = 0;
    if(count > 0) {
        min = out_ticks[0];
	for(int i=1; i<count; i++)
	    if(out_ticks[i] < min)
	        min = out_ticks[i];
    }

    if(min == now) {
	bp->update_output_tick(now);
        return; //short circuit; no need to continue
    }


    //at this point, min==0 (count==0) OR min > now
    assert(bp->get_clk() == &m_clk);

    if(count == 0) {
        min = now + ((m_llp->my_table->get_lookup_time() < m_lls->my_table->get_lookup_time()) ?
	             m_llp->my_table->get_lookup_time() : m_lls->my_table->get_lookup_time());
    }

    //at this point min is earliest outgoing msg/credit without considering unprocessed incoming msgs.

    //check if any incoming msg scheduled for current or future cycle; credit is sent for each incoming msg.
    while(m_input_msg_ticks.size() > 0)
        if(m_input_msg_ticks.front() < now) //if earlier than now, it must have been processed already, and
	    m_input_msg_ticks.pop_front();  //if any output, than it must be in m_msg_out_ticks or m_credit_out_ticks.
	else
	    break;
    if(m_input_msg_ticks.size() > 0) {
        if(m_input_msg_ticks.front() == now) {
	    m_input_msg_ticks.pop_front();
	    bp->update_output_tick(now + 1); //asumption: credit is sent one tick after msg; the earliest to send credit is next tick
            return; //can't be smaller than now+1
        }
        else {//event scheduled for future
	    Ticks_t t = m_input_msg_ticks.front() + 1; //assumption: credit is sent one tick after msg
            min = (min < t) ? min : t; //get the smaller
        }
    }

    bp->update_output_tick(min);
    //cout << "@ " << m_clk.NowTicks() << " mux preddd " << min << "\n";
//cout.flush();

}


void MuxDemux :: remote_input_notify(Ticks_t when, void* data, int port)
{
    NetworkPacket* pkt = (NetworkPacket*)data;
    if(pkt->get_type() == CREDIT_MSG_TYPE) {
        //ignore
    }
    else {
        if(m_input_msg_ticks.size() > 0) {
            assert(m_input_msg_ticks.back() <= when);
        }
        m_input_msg_ticks.push_back(when);
    }
}

#endif //ifdef FORECAST_NULL





} // namespace mcp_cache_namespace
} //namespace manifold
