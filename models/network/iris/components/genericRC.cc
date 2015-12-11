#include	"genericRC.h"
#include "simpleRouter.h"
#include "assert.h"

namespace manifold {
namespace iris {

//! @param \c vcs  No. of virtual channels.
GenericRC::GenericRC(unsigned vcs, const GenericRCSettings& setting) :
	node_id(setting.node_id),
	no_nodes(setting.no_nodes),
	grid_size(setting.grid_size),
        rc_method(setting.rc_method),
	addresses(vcs)
{
    for ( uint i = 0 ; i<vcs ; i++ )
    {
        addresses[i].route_valid = false;
        addresses[i].last_vc = 0;
    }

    //do_request_reply_network = false;
}


/*
uint
GenericRC::route_x_y(uint dest)
{
    uint oport = -1;
    uint myx=-1, destx=-1, myy =-1, desty=-1;
    myx = grid_xloc[node_id];  //??????????????????????????? grid_xloc, grid_yloc not initialized
    myy = grid_yloc[node_id];
    destx = grid_xloc[ dest ];
    desty = grid_yloc[ dest ];
    if ( myx == destx && myy == desty )
        oport = 0;
    else if ( myx ==  destx )
    {
        if( desty < myy )
            oport = 3;
        else
            oport = 4;
    }
    else
    {
        if( destx < myx )
            oport = 1;
        else
            oport = 2;
    }

    return oport;
}
*/


void
GenericRC::route_torus(HeadFlit* hf)
{
    uint myx = (int)(node_id%grid_size);
    uint destx = (int)(hf->dst_id%grid_size);
    uint myy = (int)(node_id/grid_size);
    uint desty = (int)(hf->dst_id/grid_size);

    if ( myx == destx  && myy == desty )
    {
        possible_out_ports.push_back(SimpleRouter::PORT_NI);
	decide_vc_simple(hf);
        return;
    } 
    else if ( myx == destx ) /*  reached row but not col */
    {
        /*  Decide the port based on hops around the ring */
        if ( desty > myy )
        {
            if ((desty-myy)>no_nodes/grid_size/2) // > Y_dimension/2
                possible_out_ports.push_back(SimpleRouter::PORT_NORTH);

            else
                possible_out_ports.push_back(SimpleRouter::PORT_SOUTH);
        }
        else
        {
            if ((myy - desty )>no_nodes/grid_size/2)
                possible_out_ports.push_back(SimpleRouter::PORT_SOUTH);
            else
                possible_out_ports.push_back(SimpleRouter::PORT_NORTH);
        }

	// Decide the vc
        if( possible_out_ports[0] == SimpleRouter::PORT_NORTH )
        {
            desty = (grid_size-desty)%grid_size;
            myy= (grid_size-myy)%grid_size;
        }

	decide_vc_for_ring(desty, myy, hf);

        return;
    } 
    /*  both row and col dont match do x first. Y port is 
     *  adaptive in this case and can only be used with the adaptive vc */
    else 
    {
        if ( destx > myx )
        {
            if ((destx - myx)>grid_size/2)
                possible_out_ports.push_back(SimpleRouter::PORT_WEST);
            else
                possible_out_ports.push_back(SimpleRouter::PORT_EAST);
        }
        else
        {
            if ((myx - destx )>grid_size/2)
                possible_out_ports.push_back(SimpleRouter::PORT_EAST);
            else
                possible_out_ports.push_back(SimpleRouter::PORT_WEST);
        }

	// Decide the vc
        if( possible_out_ports[0] == SimpleRouter::PORT_WEST)
        {
            destx = (grid_size-destx)%grid_size;
            myx= (grid_size-myx)%grid_size;
        }

	decide_vc_for_ring(destx, myx, hf);

        return;
    }

    assert(0); //should not reach here
}

/* not used
// this routes unidirectional
void
GenericRC::route_ring_uni(HeadFlit* hf)
{

    grid_size = no_nodes;
    uint myx = node_id;
    uint destx = hf->dst_id;

    if ( myx == destx )
    {
        possible_out_ports.push_back(0);
        if ( hf->mclass== PROC_REQ)
            possible_out_vcs.push_back(0);
        else
        {
            possible_out_vcs.push_back(1);
            // should be able to add 0-4 vcs here but make sure vca can
            // handle multiple selections
        }

        return;
    } 
    else
    {
        possible_out_ports.push_back(2);

        //  Decide the vc
        possible_out_vcs.resize(1);

        if ( destx > myx )
        {
            if ( hf->mclass == PROC_REQ )
                possible_out_vcs[0] = 2;
            else
                possible_out_vcs[0] = 3;
        }   
        else
        {
            if ( hf->mclass == PROC_REQ)
                possible_out_vcs[0] = 0;
            else
                possible_out_vcs[0] = 1;
        }

        return;
    } 
    cout << "ERROR: dint return yet " << endl;

    return;
}
*/

void
GenericRC::route_ring(HeadFlit* hf)
{

    grid_size = no_nodes;
    uint myx = node_id;
    uint destx = hf->dst_id;

    if ( myx == destx )
    {
        possible_out_ports.push_back(SimpleRouter::PORT_NI);
	decide_vc_simple(hf);
        return;
    } 
    else
    {
        if ( destx > myx)
        {
            if ( (destx - myx) > grid_size/2) 
                possible_out_ports.push_back(SimpleRouter::PORT_WEST);
            else
                possible_out_ports.push_back(SimpleRouter::PORT_EAST);
        }
        else
        {
            if ( (myx - destx) > grid_size/2) 
                possible_out_ports.push_back(SimpleRouter::PORT_EAST);
            else
                possible_out_ports.push_back(SimpleRouter::PORT_WEST);
        }

	// Decide the vc
        if( possible_out_ports[0] == SimpleRouter::PORT_WEST)
        {
            destx = (grid_size-destx)%grid_size;
            myx= (grid_size-myx)%grid_size;
        }

	decide_vc_for_ring(destx, myx, hf);

        return;
    } 

    assert(0); //should not reach here
}


void
GenericRC::route_twonode(HeadFlit* hf)
{
    if ( node_id == 0 )
    {
        if ( hf->dst_id == 1)
            possible_out_ports.push_back(SimpleRouter::PORT_EAST);
        else
            possible_out_ports.push_back(SimpleRouter::PORT_NI);
    }

    if ( node_id == 1 )
    {
        if ( hf->dst_id == 0)
            possible_out_ports.push_back(SimpleRouter::PORT_WEST);
        else
            possible_out_ports.push_back(SimpleRouter::PORT_NI);
    }

    //decide vc
    decide_vc_simple(hf);
}


void
GenericRC::push (Flit* f, uint ch )
{
    assert(ch <= addresses.size());

    //Route the head
    if( f->type == HEAD )
    {
        HeadFlit* header = static_cast< HeadFlit* >( f );
        addresses[ch].last_adaptive_port = 0;
	addresses[ch].last_vc = 0;
        addresses[ch].possible_out_ports.clear();
        addresses[ch].possible_out_vcs.clear();
        possible_out_ports.clear();
        possible_out_vcs.clear();

        if( rc_method == TORUS_ROUTING)
        {
            route_torus( header );

            assert ( possible_out_ports.size() == 1);
            assert ( possible_out_vcs.size() == 1);
        }
        else if( rc_method == RING_ROUTING)
        {
            route_ring( header );
        }
        else if( rc_method == TWONODE_ROUTING)
        {
            route_twonode( header );
        }
        else if( rc_method == XY)
        {
	    assert(0);  //XY not used
        }
	else {
	    assert(0);
	}

	addresses[ch].out_port = possible_out_ports.at(0);
	addresses[ch].possible_out_ports.push_back(possible_out_ports.at(0));

	addresses[ch].channel = possible_out_vcs[0];
	addresses[ch].possible_out_vcs.push_back(possible_out_vcs[0]);

        addresses [ch].route_valid = true;
    }
    else if(f->type == TAIL)
    {
        if( !addresses[ch].route_valid)
        {
            printf("TAIL InvalidAddrException" );
        }

        addresses[ch].route_valid = false;
        addresses[ch].possible_out_ports.clear();
        addresses[ch].possible_out_vcs.clear();
        addresses[ch].last_adaptive_port = 0;
        addresses[ch].last_vc = 0;
        possible_out_ports.clear();
        possible_out_vcs.clear();
    }
    else if (f->type == BODY)
    {
        if( !addresses[ch].route_valid)
        {
            printf("BODY InvalidAddrException" );
        }
    }
    else
    {
        printf(" InvalidFlitException fty: %d", f->type);
    }

}		/* -----  end of method genericRC::push  ----- */



void GenericRC :: decide_vc_simple(HeadFlit* hf)
{
    possible_out_vcs.push_back(0);
}

void GenericRC :: decide_vc_for_ring(uint dest, uint my, HeadFlit* hf)
{
    if ( dest > my )
    {
	possible_out_vcs.push_back(1);
    }
    else
    {
	possible_out_vcs.push_back(0);
    }
}






uint
GenericRC::get_output_port ( uint ch)
{
    assert(ch <= addresses.size());

    uint oport = -1;
    if (addresses[ch].last_adaptive_port == addresses[ch].possible_out_ports.size())
        addresses[ch].last_adaptive_port = 0;
    oport  = addresses[ch].possible_out_ports[addresses[ch].last_adaptive_port];
    addresses[ch].last_adaptive_port++;

    return oport;
}		/* -----  end of method genericRC::get_output_port  ----- */



/*
uint
GenericRC::no_adaptive_vcs( uint ch )
{
    return addresses[ch].possible_out_vcs.size();
}

uint
GenericRC::no_adaptive_ports( uint ch )
{
    return addresses[ch].possible_out_ports.size();
}
*/


uint
GenericRC::get_virtual_channel ( uint ch )
{
//The following doesn't seem to be right. last_vc is for the last packet, so
//it's meaningless for the current packet. Also, the number of possible output
//VCs could be different for different packets(?).
#if 0
    uint och = -1;
    if (addresses[ch].last_vc == addresses[ch].possible_out_vcs.size())
        addresses[ch].last_vc = 0;

    och = addresses[ch].possible_out_vcs[addresses[ch].last_vc];
    addresses[ch].last_vc++;

    return och;
#endif
    return  addresses[ch].possible_out_vcs[0];
}


/*
bool
GenericRC::is_empty ()
{
    uint channels = addresses.size();
    for ( uint i=0 ; i<channels ; i++ )
        if(addresses[i].route_valid)
            return false;

    return true;
}*/		/* -----  end of method genericRC::is_empty  ----- */



std::string
GenericRC::toString () const
{
    std::stringstream str;
    str << "GenericRC"
        << "\tchannels: " << addresses.size();
    return str.str();
}		/* -----  end of function GenericRC::toString  ----- */


//####################################################################
// ReqReplyRC
//####################################################################
void ReqReplyRC :: decide_vc_simple(HeadFlit* hf)
{
    if(hf->mclass == PROC_REQ)
        possible_out_vcs.push_back(0);
    else if(hf->mclass == MC_RESP)
        possible_out_vcs.push_back(1);
    else {
        assert(0);
    }
}


void ReqReplyRC :: decide_vc_for_ring(uint dest, uint my, HeadFlit* hf)
{

    if ( dest > my )
    {
	if ( hf->mclass == PROC_REQ)
	    possible_out_vcs.push_back(2);
	else if( hf->mclass == MC_RESP)
	    possible_out_vcs.push_back(3);
	else {
	    assert(0);
	}
    }
    else
    {
	if ( hf->mclass == PROC_REQ)
	    possible_out_vcs.push_back(0);
	else if ( hf->mclass == MC_RESP)
	    possible_out_vcs.push_back(1);
	else {
	    assert(0);
	}
    }
}



} // namespace iris
} // namespace manifold

