#include "genericVcAllocator.h"
#include "simpleRouter.h"

namespace manifold {
namespace iris {


//####################################################################
// GenericVcAllocator
//####################################################################

//! @param \c p  No. of ports.
//! @param \c v  No. of virtual channels per port.
GenericVcAllocator::GenericVcAllocator(const SimpleRouter* r, unsigned p, unsigned v) :
    router(r), PORTS(p), VCS(v),
    requested(p),
    ovc_taken(p)
{
    name = "genericVcAllocator";

    for ( uint i=0; i<PORTS; i++ ) {
        requested[i].resize(PORTS*VCS);
        for ( uint j=0; j<PORTS*VCS; j++ ) {
            requested[i][j].is_valid = false;
        }
    }

    for(unsigned i=0; i<PORTS; i++) {
        ovc_taken[i].resize(VCS);
	for(unsigned j=0; j<VCS; j++) {
	    ovc_taken[i][j] = false;
	}
    }

}

GenericVcAllocator::~GenericVcAllocator()
{
}



void
GenericVcAllocator::request( uint op, uint ovc, uint ip, uint invc )
{ 
//    requested[op][ip*VCS + invc] = true;
    VCA_unit& tmp = requested[op][ip*VCS+invc];
    if ( tmp.is_valid == false )
    {
        tmp.is_valid = true;
        tmp.in_port = ip;
        tmp.in_vc = invc;
        tmp.out_port = op;
        tmp.out_vc = ovc;
    }

}



//! Tail flit has passed through; the output VC can be released.
void
GenericVcAllocator::release_output_vc(unsigned port, unsigned ovc)
{
    ovc_taken[port][ovc] = false;
}





//####################################################################
// RRVcAllocator
//####################################################################

//! @param \c p  No. of ports.
//! @param \c v  No. of virtual channels per port.
RRVcAllocator :: RRVcAllocator(const SimpleRouter* r, unsigned p, unsigned v) :
    GenericVcAllocator(r, p, v),
    last_winner(p)
{
    name = "RRVcAllocator";

    for(unsigned i=0; i<PORTS; i++) {
        last_winner[i].resize(VCS);
	for(unsigned j=0; j<VCS; j++) {
	    last_winner[i][j] = -1; //initialize to -1 because every time in pick winner we start from last winner plus 1.
	}
    }
}


//should return winners of current tick; 
std::vector<VCA_unit>&
RRVcAllocator::pick_winner()
{
//clear winners array from last tick.
    current_winners.clear();
    for ( unsigned port=0; port<PORTS; port++) { //for each output port
	for ( unsigned vc=0; vc<VCS; vc++) { //for each vc
	    if (!ovc_taken[port][vc] && router->has_max_credits(port, vc)) { // this port/vc pair not taken and has max credits; only allocate VC when the VC has max credits; otherwise, flit cannot make progress after VC is allocated.
		unsigned st_loc = last_winner[port][vc] + 1; //starting point; this implements round-robin priority on a per output vc basis

		const unsigned TOTAL_VCS = PORTS*VCS;
		for ( unsigned jj = 0; jj<TOTAL_VCS; jj++) {
		    unsigned ivc = (st_loc + jj) % TOTAL_VCS; //input vc's index
		    if ( requested[port][ivc].is_valid && requested[port][ivc].out_vc == vc ) {//if the input vc requests this output port
			VCA_unit tmp;
			tmp.out_port = port;
			tmp.out_vc = vc;
			tmp.in_port = ivc / VCS;
			tmp.in_vc= ivc % VCS;
			current_winners.push_back(tmp);
			last_winner[port][vc] = ivc;
			ovc_taken[port][vc] = true; //make this output vc as taken; will only be released after the
			                            //tail flit has gone through.
			requested[port][ivc].is_valid = false; //reset request indicator
			break;
		    }//if
                }
	//	if(!ovc_taken[port][vc]) //if an available output vc is not taken, that means there're no
	//	    break;               //requests for this output port, so move on to the next port.
            }//if
	}//for each vc of a port
    }
    return current_winners;
}




//####################################################################
// FCFSVcAllocator: first come first serve.
//####################################################################

//! @param \c p  No. of ports.
//! @param \c v  No. of virtual channels per port.
FCFSVcAllocator :: FCFSVcAllocator(const SimpleRouter* r, unsigned p, unsigned v) :
    GenericVcAllocator(r, p, v),
    m_requesters(p*v)
{
    name = "FCFSVcAllocator";
}


void FCFSVcAllocator :: request( uint op, uint ovc, uint ip, uint invc )
{ 
    GenericVcAllocator :: request(op, ovc, ip, invc);
    m_requesters[op*VCS + ovc].push_back(ip * VCS + invc);
}



//! Allocate output VCs for requesting input VCs. When a VC requesting an output VC, it is in the
//! state SVA_REQUESTED. After it is allocated an output VC, it changes to SVA_COMPLETE. Then it
//! will request the switch. However the first time it goes from SVA_COMPLETE to SWA_REQUESTED
//! (when sa_head_done is false), it must have the max credits. This is required so that it will
//! not be blocked by the previous packet that uses the same output VC. See pp. 341 of Dally and
//! Towels for more.
//! For the FCFS VC allocator, this means we should only allocate an output VC when it is free AND
//! it has max credits. Otherwise, a VC may obtain an output VC and then find itself stuck in
//! SVA_COMPLETE because it doesn't have max credits to go to SWA_REQUESTED. Then, it may be
//! overtaken by a later VC.
//!
#if 0
std::vector<VCA_unit>&
FCFSVcAllocator :: pick_winner()
{
    //clear winners array from last tick.
    current_winners.clear();
    for ( unsigned port=0; port<PORTS; port++) { //for each output port
        if(m_requesters[port].size() > 0) { //if there are requesters for the port
	    for ( unsigned vc=0; vc<VCS; vc++) { //for each vc
		//IMPORTANT! Only allocate VC when it has max credits. This ensures the requester
		//can go immediately to SVA_COMPLETE and then SWA_REQUESTED.
		if (!ovc_taken[port][vc] && requested[port][m_requesters[port].front()].out_vc == vc && router->has_max_credits(port, vc)) { // this port/vc pair not taken and has max credits
		    unsigned ivc = m_requesters[port].front();
		    m_requesters[port].pop_front();

		    VCA_unit tmp;
		    tmp.out_port = port;
		    tmp.out_vc = vc;
		    tmp.in_port = ivc / VCS;
		    tmp.in_vc= ivc % VCS;
		    current_winners.push_back(tmp);
		    ovc_taken[port][vc] = true; //make this output vc as taken; will only be released after the
						//tail flit has gone through.
		    requested[port][ivc].is_valid = false; //reset request indicator
		    break;
                }
            }//for each vc of a port
	}//if
    }//for

    return current_winners;
}
#endif
std::vector<VCA_unit>&
FCFSVcAllocator :: pick_winner()
{
    //clear winners array from last tick.
    current_winners.clear();
    for ( unsigned port=0; port<PORTS; port++) { //for each output port
	for ( unsigned vc=0; vc<VCS; vc++) { //for each vc
	    if(m_requesters.at(port*VCS + vc).size() > 0) { //if there are requesters for the vc
		//IMPORTANT! Only allocate VC when it has max credits. This ensures the requester
		//can go immediately to SVA_COMPLETE and then SWA_REQUESTED.
		if (!ovc_taken[port][vc] && router->has_max_credits(port, vc)) { // this port/vc pair not taken and has max credits
		    unsigned ivc = m_requesters.at(port*VCS + vc).front();
		    assert(requested[port][ivc].out_vc == vc);
		    m_requesters.at(port*VCS + vc).pop_front();

		    VCA_unit tmp;
		    tmp.out_port = port;
		    tmp.out_vc = vc;
		    tmp.in_port = ivc / VCS;
		    tmp.in_vc= ivc % VCS;
		    current_winners.push_back(tmp);
		    ovc_taken[port][vc] = true; //make this output vc as taken; will only be released after the
						//tail flit has gone through.
		    requested[port][ivc].is_valid = false; //reset request indicator
		    break;
                }
            }//for each vc of a port
	}//if
    }//for

    return current_winners;
}



} // namespace iris
} // namespace manifold

