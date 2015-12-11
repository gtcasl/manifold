
#include "simpleArbiter.h"

namespace manifold {
namespace iris {


//####################################################################
// SimpleArbiter
//####################################################################
SimpleArbiter::SimpleArbiter(unsigned nch) :
    no_channels(nch),
    requested(nch)
{
    for ( unsigned i=0; i<no_channels; i++)
        requested[i] = false;
}

SimpleArbiter::~SimpleArbiter()
{
}


//! @param \c ch  The index of virtual channel
//! @return  The request status for a given virtual channel
bool
SimpleArbiter::is_requested ( unsigned ch )
{
    assert( ch < requested.size() );
    return requested[ch];
}

/*
//! @return  True if no channel is requesting.
bool
SimpleArbiter::is_empty ( void )
{
    for ( uint i=0 ; i<no_channels; i++ )
        if ( requested[i] )
            return false;

    return true;
}
*/


//! Make a request for the given channel.
//! @param \c ch  The index of virtual channel
void
SimpleArbiter::request ( unsigned ch )
{
    assert(ch < no_channels);
    requested[ch] = true;
}	


//! Clear request for the given channel.
//! @param \c ch  The index of virtual channel.
void
SimpleArbiter :: clear_request ( unsigned ch )
{
    assert(ch < no_channels);
    requested[ch] = false;
}	


//####################################################################
// RRSimpleArbiter
//####################################################################

RRSimpleArbiter :: RRSimpleArbiter(unsigned nch) :
    SimpleArbiter(nch),
    last_winner(-1)
{
}


//! Pick a winner using round-robin priority, i.e., the last winner
//! would have the lowest priority.
//! @return  The winning VC, or no_channels if there's no requester.
unsigned
RRSimpleArbiter::pick_winner ()
{
    for ( uint i=last_winner+1; i<requested.size() ; i++ )
        if ( requested[i] )
        {
            last_winner = i;
            return last_winner;
        }

    for ( uint i=0; i<=last_winner ; i++ )
        if ( requested[i] )
        {
            last_winner = i;
            return last_winner;
        }

    return no_channels;
}


//####################################################################
// FCFSSimpleArbiter
//####################################################################

FCFSSimpleArbiter :: FCFSSimpleArbiter(unsigned nch) :
    SimpleArbiter(nch)
{
}


void
FCFSSimpleArbiter::request( unsigned ch )
{
    assert(ch < no_channels && requested[ch] == false);
    requested[ch] = true;
    m_requesters.push_back(ch);
}	


//! Pick a winner using first-come-first-serve priority.
//! Return the winner, which is the earliest of the current requesters; or no_channels if
//! there are no requesters.
unsigned
FCFSSimpleArbiter::pick_winner ()
{
    unsigned winner = no_channels;
    if(m_requesters.size() > 0) {
        winner = m_requesters.front();
	m_requesters.pop_front();
    }

    return winner;
}






} //iris
} //manifold

