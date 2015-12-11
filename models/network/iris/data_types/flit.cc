#include	"flit.h"
#include <assert.h>

using namespace std;

namespace manifold {
namespace iris {


// Phit class function implementations
Phit::Phit()
{
    ft = UNK;
}

Phit::~Phit()
{
}





#ifdef IRIS_DBG
unsigned Flit :: NextId = 0;
#endif

// Flit class implementation
Flit::Flit()
{
    type = UNK;
    virtual_channel = -1;
    pkt_length = 0;
#ifdef IRIS_DBG
    flit_id = NextId;
    NextId++;
#endif
}

Flit::~Flit()
{
}

string
Flit::toString ( ) const
{
    stringstream str;
    #if 0
    switch(type) {
        case HEAD:
	    { const HeadFlit* hf = static_cast<const HeadFlit*>(this);
	      str << " HEAD " << hf->toString();
	    }
	    break;
        case BODY:
	    {
	      str << " BODY ";
	    }
	    break;
        case TAIL:
	    { const TailFlit* tf = static_cast<const TailFlit*>(this);
	      str << " TAIL ";
	    }
	    break;
	default:
	    break;
    }
    #endif

    str << " vc: " << virtual_channel
        << " type: " << type
        << " pkt_length: " << pkt_length;
#ifdef IRIS_DBG
    str << " flit_id: " << flit_id;
#endif
    return str.str();
}





// HeadFlit class implementation
HeadFlit::HeadFlit()
{
    type = HEAD;
    data_len = 0;
}



string
HeadFlit::toString () const
{
    stringstream str;
    str << "HEAD: " << Flit :: toString() <<
        " src: " << src_id <<
        " dst: " << dst_id <<
        " class: " << mclass <<
	" enter_network_time: " << enter_network_time <<
        " ";
    return str.str();
}


void
HeadFlit::populate_head_flit(void)
{
    // May want to construct control bit streams etc here
}




// BodyFlit class implementation
BodyFlit::BodyFlit()
{
    type = BODY;
}


string BodyFlit :: toString () const
{
    stringstream str;
    str << "BODY: " << Flit :: toString() <<
        " ";
    return str.str();
}


void
BodyFlit::populate_body_flit(void)
{
    // May want to construct control bit streams etc here
}




// TailFlit class implementation
TailFlit::TailFlit()
{
    type = TAIL ;
}



string TailFlit :: toString () const
{
    stringstream str;
    str << "TAIL: " << Flit :: toString() <<
        " ";
    return str.str();
}


void
TailFlit::populate_tail_flit(void)
{
    // May want to construct control bit streams etc here
}




// FlitLevelPacket class implementation
FlitLevelPacket::FlitLevelPacket()
{
}

FlitLevelPacket::~FlitLevelPacket()
{
}

void FlitLevelPacket :: set_pkt_length(unsigned len)
{
    //This function should only be called to preset the pkt_length when there are
    //no flits yet. It doesn't make sense to change pkt_length at other times.
    assert(flits.size() == 0);
    pkt_length = len;
}


//! Append a flit.
void
FlitLevelPacket::add ( Flit* f )
{   
    switch ( f->type  ) {
        case HEAD:	
            {
		assert(flits.size() == 0);

                this->pkt_length = f->pkt_length;
                this->virtual_channel = f->virtual_channel;

                HeadFlit* hf = static_cast< HeadFlit* >(f);
                this->src_id = hf->src_id;
                this->dst_id = hf->dst_id;
                this->enter_network_time = hf->enter_network_time;
                break;
            }

        case BODY:	
            {
		assert(flits.size() > 0 && flits.size() < pkt_length);
	        //nothing to do.
                break;
            }

        case TAIL:	
            {
		assert(flits.size() > 0);
		assert(flits.size() + 1 == pkt_length);

                TailFlit* tf = static_cast< TailFlit* >(f);
                break;
            }

        default:	
            cerr << " ERROR: Flit type unk " << endl;
            exit(1);
            break;
    }

    flits.push_back(f);
}



Flit* 
FlitLevelPacket::pop_next_flit ( void )
{
    assert(flits.size() > 0);
    Flit* np = flits.front();
    flits.pop_front();
    return np;
}


uint
FlitLevelPacket::size ( void )
{
    return flits.size() ;
}


bool
FlitLevelPacket::has_whole_packet ( void )
{
    if ( flits.size() > 0 )
        return ( flits.size() == flits[0]->pkt_length);
    else
        return false ;
}


void FlitLevelPacket :: dbg_print()
{
    cout << "FlitLevelPacket:\n"
         << "  size= " << flits.size() << "\n";
    if(flits.size() > 0) {
	 cout << "  src= " << src_id << "\n"
	      << "  dst= " << dst_id << "\n"
	      << "  vc= " << virtual_channel << "\n"
	      << "  enter time= " << enter_network_time << "\n"
	      << endl;
    }
}




} // namespace iris
} // namespace manifold

