#include	"genericBuffer.h"

namespace manifold {
namespace iris {


//! @param \c vcs  No. of virtual channels; internally vcs buffers are created.
//! @param \c size  Max. size of each buffer.
GenericBuffer::GenericBuffer (unsigned vcs, unsigned size) : buffers(vcs), buffer_size(size)
{
}		/* -----  end of function GenericBuffer::GenericBuffer  ----- */

GenericBuffer::~GenericBuffer ()
{
}

void
GenericBuffer::push (unsigned ch, Flit* f )
{
    buffers[ch].push_back(f);
    assert(buffers[ch].size() > 0 && buffers[ch].size() <= buffer_size);
    return;
}		/* -----  end of function GenericBuffer::push  ----- */

Flit*
GenericBuffer::pull (unsigned ch)
{
    assert(ch < buffers.size() );
    assert(buffers[ch].size() != 0);

    Flit* f = buffers[ch].front();
    buffers[ch].pop_front();
    return f;
}		/* -----  end of function GenericBuffer::pull  ----- */

/*!
 * ===  FUNCTION  ======================================================================
 *         Name:  peek
 *  Description:  Uses the pull channel. Make sure to set it before peek is
 *  called. Normal usage is to get a pointer to the head of the buffer without
 *  actually pulling it out from the buffer
 * =====================================================================================
 */
Flit*
GenericBuffer::peek (unsigned ch)
{
    assert(ch < buffers.size());
    assert(buffers[ch].size() <= buffer_size);

    Flit* f = 0;
    if(buffers[ch].size() > 0)
        f = buffers[ch].front();
    return f;
}		/* -----  end of function GenericBuffer::pull  ----- */

uint
GenericBuffer::get_occupancy ( uint ch ) const
{
    return buffers[ch].size();
}		/* -----  end of function GenericBuffer::get_occupancy  ----- */




bool
GenericBuffer::is_channel_full ( uint ch ) const
{
    /* this is the buffer size that the router is configured for the implementation allow for a bigger buffer[i].size which is the simulation artifact and not the buffer size in the physical router */
    return buffers[ch].size() >= buffer_size;  
}		/* -----  end of function GenericBuffer::full  ----- */

bool
GenericBuffer::is_empty (uint ch ) const
{
    return buffers[ch].empty();
}		/* -----  end of function GenericBuffer::empty  ----- */

std::string
GenericBuffer::toString () const
{
    std::stringstream str;
    str << "GenericBuffer"
        << "\t buffer_size: " << buffer_size
        << "\t No of buffers: " << buffers.size() << "\n";
    for( uint i=0; i<buffers.size() && !buffers[i].empty(); i++)
        str << buffers[i].front()->toString();
    return str.str();
}		/* -----  end of function GenericBuffer::toString  ----- */



} // namespace iris
} // namespace manifold
