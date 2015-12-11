#include	"genericSwitchArbiter.h"

namespace manifold {
namespace iris {

//####################################################################
// GenericSwitchArbiter
//####################################################################

//! @param \c  p  No. of ports.
//! @param \c  v  No. of virtual channels per port.
#ifdef IRIS_DBG
GenericSwitchArbiter::GenericSwitchArbiter(unsigned p, unsigned v, unsigned id) :
#else
GenericSwitchArbiter::GenericSwitchArbiter(unsigned p, unsigned v) :
#endif
    ports(p),
    vcs(v),
    requested(p),
    last_winner(p)
{
#ifdef IRIS_DBG
    router_id = id;
#endif
    name = "swa";

    for ( uint i=0; i<ports; i++) {
        requested[i].resize(ports*vcs);
        for ( uint j=0; j<(ports*vcs); j++) {
            requested[i][j]=false;
        }
    }

}


GenericSwitchArbiter::~GenericSwitchArbiter()
{
}



void
GenericSwitchArbiter::request(uint oport, uint ovc, uint inport, uint ivc )
{
#ifdef IRIS_DBG
std::cout << "SWA request: op= " << oport << " ov= " << ovc << " ip= " <<inport << " iv= " <<ivc <<std::endl;
#endif

    requested[oport][inport*vcs+ivc] = true;
    //requesting_inputs[oport][inport*vcs+ivc].port = inport;
    //requesting_inputs[oport][inport*vcs+ivc].ch=ivc;
    //requesting_inputs[oport][inport*vcs+ivc].in_time = manifold::kernel::Manifold::NowTicks();
}





//! Called when a VC no longer requests Switch Allocation, because a tail flit
//! has passed (i.e., a whole packet has been routed), or it has gone to
//! VCA_COMPLETE due to lack of input flits or credits.
void
GenericSwitchArbiter::clear_requestor( uint oport, uint inport, uint ich)
{
    requested[oport][inport*vcs+ich] = false;
}



std::string
GenericSwitchArbiter::toString() const
{
    std::stringstream str;
    str << "GenericSwitchArbiter: matrix size "
        << "\t requested_qu row_size: " << requested.size();
    if( requested.size())
        str << " col_size: " << requested[0].size()
            ;
    return str.str();
}



//####################################################################
// RRSwitchArbiter
//####################################################################

//! @param \c  p  No. of ports.
//! @param \c  v  No. of virtual channels per port.
#ifdef IRIS_DBG
RRSwitchArbiter :: RRSwitchArbiter(unsigned p, unsigned v, unsigned id) :
    GenericSwitchArbiter(p, v, id),
#else
RRSwitchArbiter :: RRSwitchArbiter(unsigned p, unsigned v) :
    GenericSwitchArbiter(p, v),
#endif
    last_port_winner(p)
{

    for ( uint i=0; i<ports; i++) {
	last_port_winner[i] = -1; //initializ to -1, since we always start from last winner plus 1.
    }
}





//! @return  Pointer to the winner's info; 0 if no requester.
const SA_unit*
RRSwitchArbiter :: pick_winner( uint oport)
{
    //if( last_winner[oport].win_cycle >= manifold::kernel::Manifold::NowTicks())
     //   return last_winner[oport];

#ifdef IRIS_DBG
std::cout << "Router " << router_id << " Switch Allocation, oport= " << oport << " requested port-vc: ";
for(int i=0; i<ports*vcs; i++) {
if(requested[oport][i])
std::cout << " " << i/vcs << "-" << i%vcs;
}
std::cout << std::endl;
#endif

    //Go through all virtual channels and pick the 1st requestor, starting from the
    //winner of last time.
    const unsigned TOTAL_VCS = ports*vcs;
    unsigned starting = last_port_winner[oport] + 1;

    for( unsigned i=0; i<TOTAL_VCS; i++) {
        unsigned ivc = (starting + i) % TOTAL_VCS;
        if(requested[oport][ivc])
        {
            last_port_winner[oport] = ivc;
            last_winner[oport].port = ivc / vcs;
            last_winner[oport].ch = ivc % vcs;
            //last_winner[oport].win_cycle= manifold::kernel::Manifold::NowTicks();
            return &last_winner[oport];
        }
    }

    return 0;
}


//####################################################################
// FCFSSwitchArbiter
//####################################################################

//! @param \c  p  No. of ports.
//! @param \c  v  No. of virtual channels per port.
#ifdef IRIS_DBG
FCFSSwitchArbiter :: FCFSSwitchArbiter(unsigned p, unsigned v, unsigned id) :
    GenericSwitchArbiter(p, v, id),
#else
FCFSSwitchArbiter :: FCFSSwitchArbiter(unsigned p, unsigned v) :
    GenericSwitchArbiter(p, v),
#endif
    m_requesters(p)
{
}



void
FCFSSwitchArbiter::request(uint oport, uint ovc, uint inport, uint ivc )
{
#ifdef IRIS_DBG
std::cout << "SWA request: op= " << oport << " ov= " << ovc << " ip= " <<inport << " iv= " <<ivc <<std::endl;
#endif

    requested[oport][inport*vcs+ivc] = true;
    m_requesters[oport].push_back(inport*vcs+ivc);
}





//! @return  Pointer to the winner's info; 0 if no requester.
const SA_unit*
FCFSSwitchArbiter :: pick_winner( uint oport)
{
#ifdef IRIS_DBG
std::cout << "Router " << router_id << " Switch Allocation, oport= " << oport << " requested port-vc: ";
for(int i=0; i<ports*vcs; i++) {
if(requested[oport][i])
std::cout << " " << i/vcs << "-" << i%vcs;
}
std::cout << std::endl;
#endif

    if(m_requesters[oport].size() > 0) {
        unsigned vcid = m_requesters[oport].front();
	m_requesters[oport].pop_front();
        assert(requested[oport][vcid]);

	last_winner[oport].port = vcid / vcs;
	last_winner[oport].ch = vcid % vcs;
	return &last_winner[oport];
    }

    return 0;
}


} // namespace iris
} // namespace manifold


