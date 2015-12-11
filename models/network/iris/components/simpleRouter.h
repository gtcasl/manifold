/*
 * =====================================================================================
 *
 *       Filename:  simpleRouter.h
 *
 *    Description:  This file contains the description for a simple 5 stage
 *    router component
 *    It is modelled as a component within manifold
 *
 *    BW->RC->VCA->SA->ST->LT
 *    Buffer write(BW)
 *    Route Computation (RC)
 *    Virtual Channel Allocation (VCA)
 *    Switch Allocation (SA)
 *    Switch Traversal (ST)
 *    Link Traversal (LT)
 *
 *    Model Description in cycles:
 *    ---------------------------
 *    BW and RC stages happen in the same cycle ( BW pushes the flits into the
 *    input buffer and the RC unit. )
 *    VCA: Head flits request for an output virtual channel and max(pxv) requesting
 *    input ports/vcs. On winning the grant the packet requests for SA. The
 *    winner is cleared when the tail exits.
 *    SA: Pick one output port from n requesting input ports (0<n<p) for the pxp crossbar 
 *    ST: Move the flits across the crossbar and push it out on the link
 *    LT: This is not modelled within the router and is part of the link component.
 *
 *
 *        Version:  2.0
 *        Created:  02/08/2011 10:50:32 AM
 *       Revision:  none
 *       Compiler:  g++/mpicxx
 *
 *         Author:  Mitchelle Rasquinha
 *        Company:  Georgia Institute of Technology
 *
 *    Modified by:  Zhenjiang Dong
 *         school:  Georgia Institute of Technology
 * =====================================================================================
 */


#ifndef  MANIFOLD_IRIS_SIMPLEROUTER_H
#define  MANIFOLD_IRIS_SIMPLEROUTER_H

//#include	"../interfaces/genericHeader.h"
#include	"../data_types/linkData.h"
#include	"genericBuffer.h"
#include	"genericSwitchArbiter.h"
#include	"genericRC.h"
#include	"genericVcAllocator.h"


namespace manifold {
namespace iris {

enum RouterPipeStage { 
    PS_INVALID = 0, 
    EMPTY,
    //IB,
    FULL,
    //ROUTED,
    VCA_REQUESTED, 
    SWA_REQUESTED, 
    //SW_ALLOCATED, 
    SW_TRAVERSAL, 
    //REQ_OUTVC_ARB, 
    VCA_COMPLETE 
};

static const char* RouterPipeStageStr[] = {
    "PS_INVALID", 
    "EMPTY",
    //"IB",
    "FULL",
    //"ROUTED",
    "VCA_REQUESTED", 
    "SWA_REQUESTED", 
    //"SW_ALLOCATED", 
    "SW_TRAVERSAL", 
    //"REQ_OUTVC_ARB", 
    "VCA_COMPLETE" 
};
#ifdef IRIS_DBG
#endif


//! Record each input VC's state
class InputBufferState
{
    public:
        uint input_port;
        uint input_channel;
        uint output_port;
        uint output_channel;
        uint64_t pkt_arrival_time;
        RouterPipeStage pipe_stage;
        bool sa_head_done; //Head flit has gone through Switch Allocation
        std::vector<uint> possible_oports;
        std::vector<uint> possible_ovcs;

        std::string toString();
#ifdef IRIS_DBG
#endif

#ifdef _DEBUG2
        uint fid;
#endif
};


struct router_init_params {
    uint no_nodes;
    uint grid_size; 
    uint no_ports;
    uint no_vcs;
    uint credits;
    ROUTING_SCHEME rc_method;
};


//! This router can be used in a ring, mesh, or torus.
class SimpleRouter : public manifold::kernel::Component
{
    public:
unsigned npred;
unsigned tpred;
        enum { PORT_NI=0, PORT_WEST, PORT_EAST, PORT_NORTH, PORT_SOUTH };


        /* ====================  LIFECYCLE     ======================================= */
        SimpleRouter(uint id, router_init_params* i_p); 
        ~SimpleRouter (); 

        /* ====================  Event handlers     ======================================= */
        void handle_link_arrival( int inputid, LinkData* data); 

        /* ====================  Clocked funtions  ======================================= */
        void tick (void);
        void tock (void);

        /* ================ Functions Local to this class ====================== */
        void print_stats(std::ostream& out) const;

        const std::vector<InputBufferState>& get_input_buffer_state() { return input_buffer_state; }

	//! Check if an output VC has max credits.
	bool has_max_credits(unsigned port, unsigned vc) const { return downstream_credits[port][vc] == CREDITS; }

	// Determine the earliest time when a head flit arrives at the NI.
	manifold::kernel::Ticks_t earliest_to_NI();

	#ifdef FORECAST_NULL
	void do_output_to_router_prediction();
	#endif

	void set_port_cross_lp (int port);
        int get_router_id () {return node_id;};
        bool get_cross_lp_flag () { return cross_lp_flag;};

#ifdef IRIS_TEST
    public:
#else
    private:
#endif
        const unsigned node_id;

        void do_switch_traversal();
        void do_switch_allocation();
        void do_vc_allocation();
	void do_route_computing();
        void do_input_buffering(HeadFlit*, uint, uint);
	void dump_input_vc_state();
	#ifdef IRIS_DBG
	#endif


        std::vector <InputBufferState> input_buffer_state;

        std::vector <GenericBuffer*> in_buffers;
        std::vector <GenericRC*> decoders; //route computation
        GenericVcAllocator* vca;
        GenericSwitchArbiter* swa;

        const unsigned ports;
        const unsigned vcs;
        const unsigned CREDITS;
        const ROUTING_SCHEME rc_method;
        uint no_nodes;
        uint grid_size;
        std::vector< std::vector<uint> > downstream_credits;


        // stats
        uint64_t stat_last_packet_out_cycle;
        
        uint64_t stat_packets_in;
        uint64_t stat_packets_out;
        uint64_t stat_flits_in;
        uint64_t stat_flits_out;

        uint64_t avg_router_latency;
        uint64_t stat_last_flit_out_cycle;
        std::vector< std::vector<uint64_t> > stat_pp_packets_out; //per-port stats
        std::vector< std::vector<uint64_t> > stat_pp_pkt_out_cy;
        std::vector< std::vector<uint64_t> > stat_pp_avg_lat;

        // energy counters
        uint64_t ib_cycles;
        uint64_t vca_cycles;
        uint64_t sa_cycles;
        uint64_t st_cycles;

	//data structures for event forecast between Router
	vector<bool> port_cross_lp;
	bool cross_lp_flag;
	
};


} // namespace iris
} // namespace manifold

#endif // MANIFOLD_IRIS_SIMPLEROUTER_H
