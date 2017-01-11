#ifndef MANIFOLD_MCP_CACHE_MUX_DEMUX_H
#define MANIFOLD_MCP_CACHE_MUX_DEMUX_H
#include "kernel/component-decl.h"
#include "kernel/clock.h"
#include "uarch/networkPacket.h"
#include <list>

#include "LLP_cache.h"
#include "LLS_cache.h"

namespace manifold {
namespace mcp_cache_namespace {

//! Both an LLP and an LLS cache need a network interface. This class
//! is a Multiplexer/Demultiplexer that connects an LLP and an LLS to a
//! single network interface.

class LLP_cache;

class MuxDemux : public manifold::kernel::Component {
public:
    enum { PORT_NET=0 };

    MuxDemux(manifold::kernel::Clock&, int credit_type);
    void set_llp_lls(LLP_cache* llp, LLS_cache* lls)
    {
        m_llp = llp;
	m_lls = lls;
    }

    void tick();
    template<typename T>
    void handle_net(int, manifold::uarch::NetworkPacket* pkt);
    void send_credit_downstream();

    #ifdef FORECAST_NULL
    void do_output_to_net_prediction();
    #endif
    //void add_to_output_ticks(manifold::kernel::Ticks_t tick);

    void print_stats(ostream&);

protected:
    #ifdef FORECAST_NULL
    //overwrite base class
    void remote_input_notify(manifold::kernel::Ticks_t, void* data, int port);
    #endif

private:
    const int CREDIT_MSG_TYPE;

    LLP_cache* m_llp;
    LLS_cache* m_lls;

    manifold::kernel::Clock& m_clk;

    #ifdef FORECAST_NULL
    //std::list<manifold::kernel::Ticks_t> m_output_ticks;
    std::list<manifold::kernel::Ticks_t> m_input_msg_ticks;
    #endif

    //stats
    unsigned stats_num_llp_incoming_msg;
    unsigned stats_num_lls_incoming_msg;
    unsigned stats_num_llp_outgoing_msg;
    unsigned stats_num_lls_outgoing_msg;
    unsigned stats_num_llp_incoming_credits;
    unsigned stats_num_lls_incoming_credits;
    unsigned stats_num_outgoing_credits;
};


//! handle incoming from network; type T is the message type from memory controller
template<typename T>
void MuxDemux :: handle_net(int, manifold::uarch::NetworkPacket* pkt)
{
    int port = pkt->dst_port;

    if(port == manifold::uarch::LLP_ID) {
	if(pkt->type == CREDIT_MSG_TYPE)
	    stats_num_llp_incoming_credits++;
	else
	    stats_num_llp_incoming_msg++;

	m_llp->handle_peer_and_manager_request(0, pkt);
    }
    else if(port == manifold::uarch::LLS_ID) {
	if(pkt->type == CREDIT_MSG_TYPE)
	    stats_num_lls_incoming_credits++;
	else
	    stats_num_lls_incoming_msg++;

	m_lls->handle_incoming<T>(0, pkt);
    }
    else {
	std::cout << "@ " << m_clk.NowTicks() << " Mux received invalid req; dst port= " << port << std::endl;
	assert(0);
    }
}


} // namespace mcp_cache_namespace
} //namespace manifold
#endif
