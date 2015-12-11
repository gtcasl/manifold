#include	"simpleRouter.h"

#include "kernel/component.h"

using namespace std;
using namespace manifold::kernel;

namespace manifold {
namespace iris {


SimpleRouter::SimpleRouter(uint id, router_init_params* i_p) : in_buffers(i_p->no_ports), decoders(i_p->no_ports),
	node_id(id),
	ports(i_p->no_ports), vcs(i_p->no_vcs), CREDITS(i_p->credits), rc_method(i_p->rc_method)
{
    assert(vcs == 4);
    port_cross_lp.resize(ports);
    for (int i = 0; i < port_cross_lp.size(); i++){
	port_cross_lp[i] = false;
    }
    cross_lp_flag = false;

    //read the parameters in i_p
    no_nodes = i_p->no_nodes;
    grid_size = i_p->grid_size;

    vca = new FCFSVcAllocator(this, i_p->no_ports, i_p->no_vcs);

    #ifdef IRIS_DBG
    swa = new FCFSSwitchArbiter(i_p->no_ports, i_p->no_vcs, id);
    #else
    swa = new FCFSSwitchArbiter(i_p->no_ports, i_p->no_vcs);
    #endif

    for(unsigned i=0; i<ports; i++)
        in_buffers[i] = new GenericBuffer(vcs, CREDITS);

    GenericRCSettings rcSetting;
    rcSetting.grid_size = grid_size;
    rcSetting.node_id = node_id;
    rcSetting.rc_method = rc_method;
    rcSetting.no_nodes = no_nodes;
    for(unsigned i=0; i<ports; i++) //one RC per port
        decoders[i] = new ReqReplyRC(vcs, rcSetting);

    // useful when debugging the swa and vca... but not needed for operation
    //swa.node_ip = node_id; 

    downstream_credits.resize(ports);

    for(uint i=0; i<ports; i++)
    {
        downstream_credits[i].resize(vcs);
    }

    //init all input buffer state
    input_buffer_state.resize(ports*vcs);

    for(uint i=0; i<ports; i++) {
        for(uint j=0; j<vcs; j++)
        {
            downstream_credits[i][j] = CREDITS;
            input_buffer_state[i*vcs+j].pipe_stage = PS_INVALID;
            input_buffer_state[i*vcs+j].input_port = -1;
            input_buffer_state[i*vcs+j].input_channel = -1;
            input_buffer_state[i*vcs+j].output_port = -1;
            input_buffer_state[i*vcs+j].output_channel = -1;
        }
    }


    // stats init
    stat_pp_packets_out.resize(ports);
    stat_pp_pkt_out_cy.resize(ports);
    stat_pp_avg_lat.resize(ports);

    for(uint i=0; i<ports; i++)
    {
        stat_pp_packets_out[i].resize(vcs);
        stat_pp_pkt_out_cy[i].resize(vcs);
        stat_pp_avg_lat[i].resize(vcs);
    }

    stat_packets_out = 0;
    stat_packets_in = 0;
    stat_flits_out = 0;
    stat_flits_in = 0;
    avg_router_latency = 0;
    stat_last_flit_out_cycle= 0;

    ib_cycles = 0;
    vca_cycles = 0;
    sa_cycles = 0;
    st_cycles = 0;
    for(uint i=0; i<ports; i++) {
        for(uint j=0; j<vcs; j++)
        {
            stat_pp_packets_out[i][j]=0;
            stat_pp_pkt_out_cy[i][j]=0;
            stat_pp_avg_lat[i][j]=0;
        }
    }
}

SimpleRouter::~SimpleRouter ()
{
    delete vca;

    for(unsigned i=0; i<ports; i++) {
        delete in_buffers[i];
        delete decoders[i];
    }
}






void
SimpleRouter::handle_link_arrival( int port, LinkData* data )
{   
    //assert(port<ports);
    switch ( data->type ) {
        case FLIT:	
            {
                /* Stats update */
                stat_flits_in++;
                ib_cycles++;

                if ( data->f->type == TAIL ) stat_packets_in++;

                uint inport = port%ports;
                //push the flit into the buffer. Init for buffer state done
                //inside do_input_buffering
                in_buffers[inport]->push(data->vc, data->f);

#ifdef IRIS_DBG
std::cout << " @ " << manifold::kernel::Manifold::NowTicks() << " Router " << node_id << " got flit from port-vc " << port << "-" << data->vc << " vcid= " << inport*vcs+data->vc << ": " << data->f->toString() << std::endl;
#endif
                if ( data->f->type == HEAD )
                {
#ifdef IRIS_DBG
std::cout << "Router " << node_id << " got flit HEAD.\n";
#endif
                    if(data->f->pkt_length == 1)
		        stat_packets_in++;

                    do_input_buffering(static_cast<HeadFlit*>(data->f), inport, data->vc);
                }
                else
                    decoders[inport]->push(data->f, data->vc);

                break;
            }
        case CREDIT:	
            {
#ifdef IRIS_DBG
std::cout << "Router " << node_id << " received credit from port " << port << std::endl;
#endif
                /*  Update credit information for downstream buffer
                 *  corresponding to port and vc */
                uint inport = port%ports;
                downstream_credits[inport][data->vc]++;

                break;
            }

        default:	
            assert(0);
            break;
    }				/* -----  end switch  ----- */

    delete data;
}


void
SimpleRouter::do_input_buffering(HeadFlit* hf, uint inport, uint invc)
{
#ifdef IRIS_DBG
std::cout << "Router " << node_id << " received head flit: port-vc " << inport << "-" << invc << " state set to FULL" << std::endl;
#endif

    //Head flit can only go from interface to router or router to another router when the VC
    //is clear. Therefore, we won't have flits from 2 different packets in the input_buffer.
    assert(input_buffer_state[inport*vcs+invc].pipe_stage == EMPTY || 
	   input_buffer_state[inport*vcs+invc].pipe_stage == PS_INVALID);
    input_buffer_state[inport*vcs+invc].input_port = inport;
    input_buffer_state[inport*vcs+invc].input_channel = invc;
    input_buffer_state[inport*vcs+invc].pipe_stage = FULL;
    input_buffer_state[inport*vcs+invc].pkt_arrival_time = manifold::kernel::Manifold::NowTicks();

    input_buffer_state[inport*vcs+invc].sa_head_done = false;

#ifdef _DEBUG2
    input_buffer_state[inport*vcs+invc].fid= hf->flit_id;
#endif

    //Route computation
    decoders[inport]->push(hf,invc);

#if 0
    input_buffer_state[inport*vcs+invc].possible_oports.clear();
    uint rc_port = decoders[inport]->get_output_port(invc);
    input_buffer_state[inport*vcs+invc].possible_oports.push_back(rc_port);

    input_buffer_state[inport*vcs+invc].possible_ovcs.clear();
    uint rc_vc = decoders[inport]->get_virtual_channel(invc);
    input_buffer_state[inport*vcs+invc].possible_ovcs.push_back(rc_vc);

    assert ( input_buffer_state[inport*vcs+invc].possible_oports.size() != 0);
    assert ( input_buffer_state[inport*vcs+invc].possible_ovcs.size() != 0);
#endif

}


//The actual route computing is done in do_input_buffering(). Here we only change the input
//VC's state from FULL to VCA_REQUESTED and make a vc allocation request.
void SimpleRouter :: do_route_computing()
{
    unsigned idx = 0;
    for(unsigned p=0; p<ports; p++) {
	for(unsigned v=0; v<vcs; v++, idx++) {
	    // idx == p*vcs + v

	    // Check new head flits; enter them into VCA_REQUESTED stage.
	    if(input_buffer_state[idx].pipe_stage == FULL) {

		input_buffer_state[idx].possible_oports.clear();
		unsigned rc_port = decoders[p]->get_output_port(v);
		input_buffer_state[idx].possible_oports.push_back(rc_port);

		input_buffer_state[idx].possible_ovcs.clear();
		unsigned rc_vc = decoders[p]->get_virtual_channel(v);
		input_buffer_state[idx].possible_ovcs.push_back(rc_vc);

		assert ( input_buffer_state[idx].possible_oports.size() != 0);
		assert ( input_buffer_state[idx].possible_ovcs.size() != 0);

		assert(p == input_buffer_state[idx].input_port);
		assert(v == input_buffer_state[idx].input_channel);

		unsigned op = input_buffer_state[idx].possible_oports[0];
		unsigned oc = input_buffer_state[idx].possible_ovcs[0];
		input_buffer_state[idx].pipe_stage = VCA_REQUESTED;
		vca->request(op,oc,p,v);
#ifdef IRIS_DBG
std::cout << "Router " << node_id << " vc " << p << "-"<< v <<"-" << op <<"-"<< oc << " FULL->VCA_REQUESTED.\n";
#endif
	    }
	}
    }
}


//! Do 3 things in the following order:
//! 1. Allocate output VC for all input VCs in the state VCA_REQUESTED. This arbitration task is designated to
//! the VC allocator. Winners of the arbitration move to the state VCA_COMPLETE.
//! 2. Check all input VCs in the state VCA_COMPLETE, if they can use the switch, move them to SWA_REQUESTED.
//! 3. Check all input VCs in the state FULL. These are newly arrived packets. Move them to VCA_REQUESTED, and
//! for each one make a request to the VC allocator.
void SimpleRouter::do_vc_allocation()
{
    vca_cycles++;

    std::vector<VCA_unit>& vca_current_winners = vca->pick_winner();

    for ( unsigned i=0; i<vca_current_winners.size(); i++) {
	VCA_unit winner = vca_current_winners[i];

	uint ivc = winner.in_port*vcs + winner.in_vc;

	assert( input_buffer_state[ivc].pipe_stage == VCA_REQUESTED );

	input_buffer_state[ivc].output_port = winner.out_port;
	input_buffer_state[ivc].output_channel= winner.out_vc;
	input_buffer_state[ivc].pipe_stage = VCA_COMPLETE; //in the following we check if VCs in VCA_COMPLETE
	                                                   //can move on to SWA_REQUESTED

#ifdef IRIS_DBG
std::cout << "Router " << node_id << " vc " << winner.in_port << "-"<<winner.in_vc <<"-" <<winner.out_port <<"-"<<winner.out_vc << " VCA_REQUESTED->VCA_COMPLETE.\n";
#endif
	// if requesting multiple outports make sure to cancel them as
	// pkt will no longer be VCA_REQUESTED
    }



    //check all input VCs and change their state if applicable.
    for( uint i=0; i<(ports*vcs); i++) {
	//a VC can go from  SW_TRAVERSAL to VCA_COMPLETE; this is
	//a stalled state in which a VC has allocated an output VC but could not go to
	//SWA_REQUESTED becaues input is empty or no credits to send. Check and see if
	//the situation has changed.
        if( input_buffer_state[i].pipe_stage == VCA_COMPLETE) {
            uint ip = input_buffer_state[i].input_port;
            uint ic = input_buffer_state[i].input_channel;
            uint op = input_buffer_state[i].output_port;
            uint oc = input_buffer_state[i].output_channel;

	    //For a VC to go from VCA_COMPLETE to SWA_REQUESTED, there must be input flits and credits.
	    //If the head flit hasn't passed yet, we wait until we have full credits; otherwise, we
	    //require credits > 0.
            if(in_buffers[ip]->get_occupancy(ic)) { //input buffer has flit(s)
	        if( (input_buffer_state[i].sa_head_done && downstream_credits[op][oc] > 0) ||
		   (!input_buffer_state[i].sa_head_done && downstream_credits[op][oc] == CREDITS)) { //has credits
		   //If head flit hasn't been through SWA_REQUESTED yet, we require the output VC has max
		   //credits, meaning, the previous packet using the same output channel has completely left
		   //the downstream router. See pp. 314 of Dally and Towels.
		    //assert(swa->is_requested(op, ip, ic) == false);
		    swa->request(op, oc, ip, ic);
		    input_buffer_state[i].pipe_stage = SWA_REQUESTED;
#ifdef IRIS_DBG
std::cout << "Router " << node_id << " vc " << ip << "-"<<ic <<"-" <<op <<"-"<<oc << " VCA_COMPLETE->SWA_REQUESTED.\n";
#endif
		}
            }
        }
	#if 0
	//moved to do_route_computing()
	else if( input_buffer_state[i].pipe_stage == FULL ) { //new head flit
	    // Check new head flits; enter them into VCA_REQUESTED stage.
            uint ip = input_buffer_state[i].input_port;
            uint ic = input_buffer_state[i].input_channel;
            uint op = input_buffer_state[i].possible_oports[0]; //routing algorithm already determined output port
            uint oc = input_buffer_state[i].possible_ovcs[0];
	    input_buffer_state[i].pipe_stage = VCA_REQUESTED;
	    vca->request(op,oc,ip,ic);
#ifdef IRIS_DBG
std::cout << "Router " << node_id << " vc " << ip << "-"<<ic <<"-" <<op <<"-"<<oc << " FULL->VCA_REQUESTED.\n";
#endif
        }
	#endif
    }
}





//! Move flits out of the router.
//! For each input VC in the SW_TRAVERSAL state, check if a flit can be sent out. If not, move to the
//! VCA_COMPLETE state. If a flit can be sent, send it out and send a credit upstream. If the flit is
//! a tail flit, ask the VC allocator to release the output VC, and the Switch Allocator to clear the
//! request. If not a tail flit, check if more flit can be sent, and if so, move to SWA_REQUESTED,
//! otherwise, move to VCA_COMPLETE and clear the request for SWA.
void 
SimpleRouter::do_switch_traversal()
{
    for( uint i=0; i<ports*vcs; i++) { //for each vc
        if( input_buffer_state[i].pipe_stage == SW_TRAVERSAL) {
            uint op = input_buffer_state[i].output_port;
            uint oc = input_buffer_state[i].output_channel;
            uint ip = input_buffer_state[i].input_port;
            uint ic= input_buffer_state[i].input_channel;

            if( in_buffers[ip]->get_occupancy(ic)> 0 && downstream_credits[op][oc]>0 ) {
                Flit* f = in_buffers[ip]->pull(ic);
                f->virtual_channel = oc;

                if( f->type == TAIL || f->pkt_length == 1) {
                    // Update packet stats
                    uint lat = manifold::kernel::Manifold::NowTicks() - input_buffer_state[i].pkt_arrival_time;
                    avg_router_latency += lat;
                    stat_packets_out++;
                    stat_pp_packets_out[op][oc]++;
                    stat_pp_pkt_out_cy[op][oc] = manifold::kernel::Manifold::NowTicks();
                    stat_pp_avg_lat[op][oc] += lat;

                    input_buffer_state[i].pipe_stage = EMPTY;
                    input_buffer_state[i].input_port = -1;
                    input_buffer_state[i].input_channel = -1;
                    input_buffer_state[i].output_port = -1;
                    input_buffer_state[i].output_channel = -1;
                    input_buffer_state[i].possible_oports.clear();
                    input_buffer_state[i].possible_ovcs.clear();

		    vca->release_output_vc(op, oc); //VC allocator can release the output VC
                    swa->clear_requestor(op, ip, ic); //Switch Allocation no longer needed
#ifdef IRIS_DBG
std::cout << "Router " << node_id << " vc " << ip << "-"<<ic <<"-" <<op <<"-"<<oc << " SWA_TRAVERSAL->EMPTY.\n";
#endif
                }
		else {
		    //See if we can send more flits; if so, go to SWA_REQUESTED; otherwise,
		    //go to VCA_COMPLETE.
		    if( in_buffers[ip]->get_occupancy(ic)> 0 && downstream_credits[op][oc]>1) {
		        //note credits have not been decremented yet; so compare againt 1
			input_buffer_state[i].pipe_stage = SWA_REQUESTED;
#ifdef IRIS_DBG
std::cout << "Router " << node_id << " switch traversal for " << ip << "-" << ic << ", SW_TRAVERSAL->SWA_REQUESTED" << std::endl;
#endif
			swa->request(op, oc, ip, ic);
		    }
		    else {
#ifdef IRIS_DBG
std::cout << "Router " << node_id << " switch traversal for " << ip << "-" << ic << ", SW_TRAVERSAL->VCA_COMPLETE" << std::endl;
#endif
			input_buffer_state[i].pipe_stage = VCA_COMPLETE;
			swa->clear_requestor(op, ip, ic);
		    }

		}

                stat_flits_out++;
                st_cycles++;

                LinkData* ld = new LinkData();
                ld->type = FLIT;
                ld->src = this->node_id;
                f->virtual_channel = oc;
                ld->f = f;
                ld->vc = oc;

#ifdef IRIS_DBG
cout << "@ " << manifold::kernel::Manifold::NowTicks() << " Router " << node_id << " sending flit to port " << op << ": " << f->toString() << endl;
#endif
                Send(op, ld);    //schedule cannot be used here as the component is not on the same LP
                downstream_credits[op][oc]--;

                LinkData* ldc = new LinkData();
                ldc->type = CREDIT;
                ldc->src = this->node_id;
                ldc->vc = ic;

#ifdef IRIS_DBG
cout << "@ " << manifold::kernel::Manifold::NowTicks() << " router " << node_id << "  credit to port " << ip << endl;
#endif
                Send(ip, ldc);
                stat_last_flit_out_cycle= manifold::kernel::Manifold::NowTicks();
            }
            else { //either no input flits or no credits, or both
	    //We may never reach here!
#ifdef IRIS_DBG
std::cout << "Router " << node_id << " switch traversal for " << ip << "-" << ic << ", SW_TRAVERSAL->VCA_COMPLETE due to lack of input or credit" << std::endl;
#endif
                input_buffer_state[i].pipe_stage = VCA_COMPLETE;
                swa->clear_requestor(op, ip, ic);
            }
        }//in SW_TRAVERSAL
    }//for
}



//! Allocate output ports to input VCs. For each output port, ask the SWA to pick a winner.
//! Since for each tick, each output port can send out only 1 flit, only one winner for each
//! port can be picked.
void 
SimpleRouter::do_switch_allocation()
{
    sa_cycles++;    // stat

    for(unsigned p=0; p<ports; p++) { //for each output port
        const SA_unit* sap = swa->pick_winner(p);
	if(sap == 0) //no winner for this port; must be no requestors
	    continue;

        SA_unit sa_winner = *sap;

	unsigned winner_ivc = sa_winner.port*vcs + sa_winner.ch;

	assert(input_buffer_state[winner_ivc].pipe_stage == SWA_REQUESTED);

	if(input_buffer_state[winner_ivc].sa_head_done == false)
	    input_buffer_state[winner_ivc].sa_head_done = true;

	input_buffer_state[winner_ivc].pipe_stage = SW_TRAVERSAL;
#ifdef IRIS_DBG
std::cout << "Router " << node_id << " switch alloc for port " << p << " winner: " << sa_winner.port <<"-"<<sa_winner.ch << " SWA_REQUESTED->SW_TRAVERSAL" << std::endl;
#endif
	//We don't turn off request for switch allocation here. After switch traversal, if we can send
	//more flit, we will go back to VCA_COMPLETE, and at the point the request for SWA will be turned off.
    }

}


void
SimpleRouter::tock ( void )
{
    return ;
}

extern void dump_state_at_deadlock(void);

//! Move the pipeline.
void
SimpleRouter::tick ( void )
{
#ifdef IRIS_DBG
std::cout << "Router " << node_id << " ################################################################ tick " << manifold::kernel::Manifold::NowTicks() << std::endl;
#endif

    /* 
     * This can be used to check if a message does not move for a large
     * interval. ( eg. interval of arrival_time + 100 )
     for ( uint i=0; i<input_buffer_state.size(); i++)
     if (input_buffer_state[i].pipe_stage != EMPTY
     && input_buffer_state[i].pipe_stage != PS_INVALID
     && input_buffer_state[i].pkt_arrival_time+100 < manifold::kernel::Manifold::NowTicks())
     {
     fprintf(stderr,"\n\nDeadlock at Router %d node %d Msg id %d Fid%d", GetComponentId(), node_id,
     i,input_buffer_state[i].fid);
     dump_state_at_deadlock();
     }
     * */
#ifdef IRIS_DBG
std::cout << "Router " << node_id << " ============================================== sw traversal\n";
#endif
    do_switch_traversal();

#ifdef IRIS_DBG
std::cout << "Router " << node_id << " ============================================== sw alloc\n";
#endif
    do_switch_allocation();

#ifdef IRIS_DBG
std::cout << "Router " << node_id << " ============================================== vc alloc\n";
#endif
    do_vc_allocation();

#ifdef IRIS_DBG
std::cout << "Router " << node_id << " ============================================== route computing\n";
#endif
    do_route_computing();

#ifdef IRIS_DBG
dump_input_vc_state();
#endif
}


//! Determine the earliest time when a head flit arrives at the NI.
manifold::kernel::Ticks_t SimpleRouter :: earliest_to_NI()
{
    for(unsigned i=0; i<ports; i++) {
#if 1
        if(i == PORT_NI)
	    continue;
        for(unsigned j=0; j<vcs; j++) {
	    if(input_buffer_state[i*vcs+j].output_port == PORT_NI)
	        return 0;
        }
#endif 
    }

    //There are no flits for the NI.
    return 4; //takes at least 4 cycles for a head flt to arrive at NI
}



void SimpleRouter::print_stats (ostream& out ) const
{
    out 
        << "\n SimpleRouter[" << node_id << "] packets_in: " << stat_packets_in
        << "\n SimpleRouter[" << node_id << "] packets_out: " << stat_packets_out
        << "\n SimpleRouter[" << node_id << "] flits_in: " << stat_flits_in
        << "\n SimpleRouter[" << node_id << "] flits_out: " << stat_flits_out
        << "\n SimpleRouter[" << node_id << "] avg_router_latency: " << (avg_router_latency+0.0)/stat_packets_out
        << "\n SimpleRouter[" << node_id << "] last_pkt_out_cy: " << stat_last_flit_out_cycle
        << "\n SimpleRouter[" << node_id << "] ib_cycles: " << ib_cycles
        << "\n SimpleRouter[" << node_id << "] vca_cycles: " << vca_cycles
        << "\n SimpleRouter[" << node_id << "] sa_cycles: " << sa_cycles
        << "\n SimpleRouter[" << node_id << "] st_cycles: " << st_cycles
        ;
    out << "\n SimpleRouter[" << node_id << "] per_port_pkts_out: ";
    for ( uint i=0; i<ports; i++) {
        out << "(";
        for ( uint j=0; j<vcs; j++) {
            out << stat_pp_packets_out[i][j];
	    if(j != vcs-1)
	        out << " ";
	}
	out << ") ";
    }

    out << "\n SimpleRouter[" << node_id << "] per_port_avg_lat: ";
    for ( uint i=0; i<ports; i++) {
        out << "(";
        for ( uint j=0; j<vcs; j++) {
            if ( stat_pp_packets_out[i][j] ) 
                out  << (stat_pp_avg_lat[i][j]+0.0)/stat_pp_packets_out[i][j];
            else
                out  << "0";
	    if(j != vcs-1)
	        out << " ";
	}
	out << ") ";
    }

    out << "\n SimpleRouter[" << node_id << "] per_port_last_pkt_out_cy: ";
    for ( uint i=0; i<ports; i++) {
        out << "(";
        for ( uint j=0; j<vcs; j++) {
            out << stat_pp_pkt_out_cy[i][j];
	    if(j != vcs-1)
	        out << " ";
	}
	out << ") ";
    }

    if ( ( stat_packets_in - stat_packets_out ) > ports*vcs )
        out << "\n ERROR pkts_in-pkts_out > buffering available";
    out << "\n";
}



std::string InputBufferState :: toString()
{
    stringstream str;
    str << "port= " << (int)input_port << " vc= " << (int)input_channel << " out port= " << (int)output_port
		      << " out vc= " << (int)output_channel << " stage= " << RouterPipeStageStr[pipe_stage];
    return str.str();
}

void SimpleRouter :: dump_input_vc_state()
{
    for(unsigned i=0; i<ports; i++) {
        for(unsigned j=0; j<vcs; j++) {
            unsigned idx = i*vcs + j;
            unsigned op = input_buffer_state[idx].output_port;
            unsigned ovc = input_buffer_state[idx].output_channel;
            std::cout << "Router " << node_id << "  " << idx << ": " << input_buffer_state[idx].toString() << " buffered flits= " << in_buffers[i]->get_occupancy(j);

            unsigned ip = input_buffer_state[idx].input_port;
            unsigned ivc = input_buffer_state[idx].input_channel;
            if(ip != -1) {
                Flit* f = in_buffers[ip]->peek(ivc);
                if(f != 0 && f->type == HEAD) {
                    std::cout << " dst= " << ((HeadFlit*)f)->dst_id << " len= " << f->pkt_length;
                }
            }
            if(input_buffer_state[idx].possible_oports.size() > 0) {
                std::cout << " RC_port= " << input_buffer_state[idx].possible_oports[0]
                          << " RC_vc= " << input_buffer_state[idx].possible_ovcs[0];
            }
	    cout << " credits= ";
            if(op < ports && ovc < vcs)
                std::cout << downstream_credits[op][ovc] << std::endl;
            else
                std::cout << " NA\n";
        }
    }
    for(unsigned i=0; i<ports; i++) {
        for(unsigned j=0; j<vcs; j++) {
            cout << "Router " << node_id << " port " << i << " vc " << j << " credits= " << downstream_credits[i][j] << endl;
        }
    }

}



//record the ports that connected to another router that at different lp
void SimpleRouter :: set_port_cross_lp (int port)
{
    port_cross_lp[port] = true;
    if (!cross_lp_flag)
	cross_lp_flag = true;
    //cout<<"router: "<<node_id<<" port: "<<port<<endl;
}





#ifdef FORECAST_NULL
//! Predict, conservatively, the earliest tick when the router sends out message to other LPs.
//! This is used as part of the forecast null message algorithm.
void SimpleRouter :: do_output_to_router_prediction()
{
    const int PIPE_DEPTH = 4;

    for (unsigned p = 0; p < ports; p++) { //for all ports
	if (!port_cross_lp[p])
	    continue;
	    
	BorderPort* bp = this->border_ports[p];
 	assert(bp != 0);

	//check each input vc
	bool inited = false;
	Ticks_t pred = 0; //forecast for port p

	for(unsigned q=0; q<ports; q++) {
	    if(q == p) {//when flit from p leaves the router, credit is sent out of p
		for(unsigned v=0; v<vcs; v++) { //for each vc
		    Ticks_t vc_pred = 0;
		    switch(input_buffer_state[q*vcs + v].pipe_stage) {
			case PS_INVALID:
			case EMPTY:
			case FULL:
			    vc_pred = PIPE_DEPTH;
			    break;
			case VCA_REQUESTED:
			    vc_pred = 2;
			    break;
			case VCA_COMPLETE:
			    vc_pred = 2;
			    break;
			case SWA_REQUESTED:
			    vc_pred = 1;
			    break;
			case SW_TRAVERSAL:
			    vc_pred = 0;
			    break;
		    }//switch
		    if(inited == false) {
			pred = vc_pred;
			inited = true;
		    }
		    else {
			if(vc_pred < pred)
			    pred = vc_pred;
		    }
		    if(pred == 0)
		        break; //if credit could be sent right away, this port is done.
		}//for each vc
            }//if q==p
	    else { //q != p
		for(unsigned v=0; v<vcs; v++) { //for each vc
		    Ticks_t vc_pred = 0;
		    int oport = input_buffer_state[q*vcs+v].output_port;
		    switch(input_buffer_state[q*vcs + v].pipe_stage) {
			case PS_INVALID:
			case EMPTY:
			case FULL:
			    vc_pred = PIPE_DEPTH - 1; //why DEPTH-1? Because the current tick has not been processed.
			                              //assume state is EMPTY, and there's a head flit arriving this tick.
						      //When the event is processed, do_input_buffering() puts the state
						      //to FULL, and after tick() at t, it changes to VCA_REQUESTED, after
						      //tick() at t+1 it becomes SWA_REQUESTED, after tick() at t+2 SW_TRAVERSAL,
						      //at t+3 it goes out. So at t we can only predict t+3, which is 3 ticks
						      //from now.
			    break;
			case VCA_REQUESTED:
                            if(input_buffer_state[q*vcs+v].possible_oports[0] == p)
			        vc_pred = 2;
			    else
				vc_pred = PIPE_DEPTH;
			    break;
			case VCA_COMPLETE:
			    if(oport == p)
				vc_pred = 2;
			    else
				vc_pred = PIPE_DEPTH;
			    break;
			case SWA_REQUESTED:
			    if(oport == p)
				vc_pred = 1;
			    else
				vc_pred = PIPE_DEPTH;
			    break;
			case SW_TRAVERSAL:
			    if(oport == p)
				vc_pred = 0;
			    else
				vc_pred = PIPE_DEPTH;
			    break;
		    }//switch

		    if(inited == false) {
			pred = vc_pred;
			inited = true;
		    }
		    else {
			if(vc_pred < pred)
			    pred = vc_pred;
		    }

		    if(pred == 0)
			break; //no need to continue b/c one flit could be sent right away
		}//for each vc
	    }//q != p

	    if(pred == 0)
		break; //no need to continue b/c a flit or credit could be sent right away
	}//for each port q

#if 0
cout << "@ " << Manifold::NowTicks() << " pppppppppp router" << node_id << " port " << p << " pred= " << pred << endl;
#endif
	pred += manifold::kernel::Manifold::NowTicks();
	bp->update_output_tick(pred);
    }//for each port p

}

#endif //#ifdef FORECAST_NULL




} // namespace iris
} // namespace manifold

