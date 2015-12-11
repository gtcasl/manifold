#ifndef MANIFOLD_IRIS_GENONEVCIRISINTERFACE_H
#define MANIFOLD_IRIS_GENONEVCIRISINTERFACE_H

/*
 * =====================================================================================
 *    Description:  new interface that can be connect all kinds of terminal 
                    but have not VC
 *
 *        Version:  1.0
 *        Created:  08/29/2011 
 *
 *         Author:  Zhenjiang Dong
 *         School:  Georgia Institute of Technology
 *
 * =====================================================================================
 */

#include        "../interfaces/genericHeader.h"
#include        "../interfaces/mapping.h"
#include        "../data_types/linkData.h"
#include	"../data_types/flit.h"
#include        "kernel/component.h"
#include        "kernel/manifold.h"
#include        <iostream>
#include        <assert.h>


namespace manifold {
namespace iris {


struct simple_inf_init_params{
    uint linkWidth; //how many bit can be sent in one flit
    uint num_credits; //# credits
};

/* *********** the simple interface start here ************ */

//! This is a simple one-channel (i.e., no virtual channels) network interface which is meant to
//! be used with the crossbar switch which also does not support virtual channel. It is templated
//! so it can accept any terminal data type as long as it supports the following:
//! - The data type has no pointer members.
//! - The data type has no virtual functions.
//! - The data type has no STL containers.
//! - The data type supports a member function: get_dst() which returns the packet source's terminal ID.
//!
template<typename T>
class GenOneVcIrisInterface: public manifold::kernel::Component
{
    public:
        //the enums that specifies the ports
        enum { DATAOUT=0, DATATOTERMINAL};
        enum { DATAIN=0, DATAFROMTERMINAL};
        
        //life cycle
        GenOneVcIrisInterface (unsigned ifid, const Terminal_to_net_mapping*, simple_inf_init_params* i_p);
        ~GenOneVcIrisInterface (); 

        /* ====================  Event handlers  (unsynchronized)   ======================================= */
        void handle_new_packet_event( int port, T* data); //modified
        void handle_router( int port, LinkData* data); 

        /* ====================  Clocked funtions  ======================================= */
        void tick (void);
        void tock (void);

	void print_stats(std::ostream& out);

#ifndef IRIS_TEST
    private:
#endif    
         /* ====================  convert the pkt to flits(or inversely)  ======================================= */
        void to_flit_level_packet(FlitLevelPacket* flp, uint lw, T* data);
        T* from_flit_level_packet(FlitLevelPacket* flp);

        const unsigned credits; //# credits for each channel
        const unsigned link_width; //the link bandwidth in bits

        std::list<T*> input_pkt_buffer; //the infinite input buffer for the input from terminal
        FlitLevelPacket proc_out_buffer; //A buffer where a packet is turned into flits; one per VC.
        FlitLevelPacket proc_in_buffer; //A buffer where flits for the same packet are assembled.
	                                //One per VC.
        bool is_proc_out_buffer_free; //whether the proc_out buffer is available
        int downstream_credits; //the credit correspond for link to network
        
        const unsigned id;
	const Terminal_to_net_mapping* term_ni_mapping; //terminal to network interface ID mapping

        //stats
        uint64_t stat_packets_in_from_router; 
        uint64_t stat_packets_out_to_router;
	uint64_t stat_packets_in_from_terminal;
	uint64_t stat_packets_out_to_terminal;

};  /* -----  end of class generic interface  ----- */


//! the constructor
//! initializing all the vector to given size and assign initial value to them
//! @param \c i_p  # of credits for each channel, the link bandwidth in bits
template<typename T>
GenOneVcIrisInterface<T>::GenOneVcIrisInterface (unsigned ifid, const Terminal_to_net_mapping* mapping, simple_inf_init_params* i_p) :
        credits(i_p->num_credits), link_width(i_p->linkWidth), id(ifid), term_ni_mapping(mapping)
{
    //link_width must be multiple of 8
    assert(link_width % 8 == 0); 

    downstream_credits = credits;
    is_proc_out_buffer_free = true;

    // Init stats
    stat_packets_in_from_terminal = 0;
    stat_packets_out_to_terminal = 0;
    stat_packets_in_from_router = 0;
    stat_packets_out_to_router = 0;

}


//! the deconstructor
template<typename T>
GenOneVcIrisInterface<T>::~GenOneVcIrisInterface ()
{
}

//! response for putting flits into buffer and add the downstream credit
//! @param \c port  The port # where the data come from
//! @param \c data  The link data come from network which can be two types: credit and flit
template<typename T>
void
GenOneVcIrisInterface<T>::handle_router (int port, LinkData* data )
{   
    switch ( data->type ) {
        case FLIT:
            {
#ifdef DEBUG_IRIS_INTERFACE
std::cout << "Interface " << id << " got flit from router: " << data->f->toString() << std::endl;
#endif
                if ( data->f->type == TAIL )
                    stat_packets_in_from_router++;

                //push the incoming flit to the router in buffer 
                proc_in_buffer.add(data->f);
                
                if ( data->f->type != TAIL && data->f->pkt_length != 1)
                {
                    //generate credit and send back to data source
                    LinkData* ld =  new LinkData();
                    ld->type = CREDIT;
                    ld->src = id;
                    ld->vc = data->vc;

#ifdef DEBUG_IRIS_INTERFACE
                    std::cout << " SEND Head/Body CREDIT vc " << ld->vc << std::endl;
#endif
                    //send credit to router
                    Send(DATAOUT, ld);
                }
                break;
            }

        case CREDIT:
            {
                downstream_credits++;
                assert(downstream_credits <= (int)credits);
                break;
            }

        default:	
            assert(0);
            break;
    }	
    // delete ld.. if ld contains a flit it is not deleted. see destructor
    delete data;
}


//! putting packets into the input buffer for terminal
//! @param \c port  The port # where the data come from
//! @param \c data  The packet come from terminal
template<typename T>
void
GenOneVcIrisInterface<T>::handle_new_packet_event (int port, T* data )
{  
    assert(port == DATAFROMTERMINAL);
    input_pkt_buffer.push_back(data);
    stat_packets_in_from_terminal++;
}


//! Generate a new packet when all the flits in a packet are received; flits are deleted.
//! @param \c flp  Pointer to a FlitLevelPacket, which is a container that holds all flits for a packet.
//! @return  Pointer to a packet.
template<typename T>
T*
GenOneVcIrisInterface<T>::from_flit_level_packet(FlitLevelPacket* flp)
{
    T* message = new T;
    uint8_t* ptr = (uint8_t*) message;
    uint byte_count = 0;
    
    while(flp->size() > 0)
    {
        Flit* f = flp->pop_next_flit ();
        if (f->type == BODY)
        {   
	    assert(byte_count < sizeof(T));
            for (unsigned j = 0; j < link_width/8; j++)
            {
                if(byte_count < sizeof(T)) {
		    *ptr = static_cast<BodyFlit*>(f)->data[j];
		    ptr++;
		    byte_count++;
		}
            }
        } 
	delete f;
    }
    
    return message;
}


//! send flit and credit out to given dst
template<typename T>
void
GenOneVcIrisInterface<T>::tick ( void )
{
    // see if there is anything to send and if there is credit
    if ((proc_out_buffer.size() > 0) && (downstream_credits > 0))
    {
	//for HEAD flit, do we need to wait for credits to reach max value????

        Flit* f = proc_out_buffer.pop_next_flit();
        f->virtual_channel = 0;
        
        //reduce the credit by one
        downstream_credits--;
        
        //if it is the tail flit the out buffer will be free
        if ( f->type == TAIL || f->pkt_length == 1 )
        {
           is_proc_out_buffer_free = true;
           assert(proc_out_buffer.size() == 0);
	   stat_packets_out_to_router++;
        }
        
        //send the flit over the winner channel
        LinkData* ld =  new LinkData();
        ld->type = FLIT;
        ld->src = this->id; //ld->src only useful for debug
        ld->f = f;
        ld->vc = 0; 
        
        Send(DATAOUT, ld);               
    }

    //Now process inputs from switch/router
    //Pull entire packets from proc_in_buffer 
    //find the finished flit level pkt convert the filts to mem_req and send it back to cache
    if ( proc_in_buffer.has_whole_packet() )
    {  
        //send a new packet to the terminal if received tail pkt.
        T* np = from_flit_level_packet(&proc_in_buffer);
	assert(proc_in_buffer.size() == 0);

        Send(DATATOTERMINAL, np);
	stat_packets_out_to_terminal++;
          
            
        /*  Send the tail credit back. All other flit credits were sent
         *  as soon as flit was got in link-arrival */
        LinkData* ld =  new LinkData();
        ld->type = CREDIT;

        ld->src = this->id;
        ld->vc = 0;

#ifdef DEBUG_IRIS_INTERFACE
std::cout << "Interface " << id << " SEND CREDIT to router after delivering packet to terminal." << std::endl;
#endif
        //send credit for the tail flit to router
        Send(DATAOUT,ld);
    }
}


//! transfer the incoming packet to proc_out_buffer and decide which vc is should use
template<typename T>
void
GenOneVcIrisInterface<T>::tock ( void )
{
    // Take packets from input_pkt_buffer, convert them into flits, and store in free slots of proc_out_buffer
    if (is_proc_out_buffer_free && !input_pkt_buffer.empty())
    {  
       //convert the packet to flits and copy it to out buffer
       T* pkt = input_pkt_buffer.front();
       to_flit_level_packet( &proc_out_buffer, link_width, pkt);

       //assign virtual channel
       proc_out_buffer.virtual_channel = 0;
               
       is_proc_out_buffer_free = false;
       assert( proc_out_buffer.size() != 0 );
            
       //remove the element in pkt buffer
       delete pkt;
       input_pkt_buffer.pop_front();
    }
}

//! Take an input packet and create a FlitLevelPacket.
//! @param \c lw  The link bandwidth in bits
//! @param \c data  The packet from terminal
//! @param \c flp  The point of outbound proc_out_buffer
template<typename T>
void
GenOneVcIrisInterface<T>::to_flit_level_packet(FlitLevelPacket* flp, uint lw, T* data)
{
    assert(flp->size() == 0);

    /* find the number of flits */
    uint extra_flits = 2; //head and tail
    
    //calculate # of body flit needed to send a pkt
    uint no_bf = uint ((sizeof(T)*8)/lw);
    double compare =double (sizeof(T)*8)/ lw;
    
    if (compare > no_bf)
       no_bf ++;
    
    //generate the flit level packet
    uint tot_flits = extra_flits + no_bf;  
    flp->set_pkt_length(tot_flits);

    //convert to network address
    flp->dst_id = data->get_dst();
    flp->src_id = this->id;
    
    //generate the header flit
    HeadFlit* hf = new HeadFlit();
    hf->pkt_length = tot_flits;
    hf->src_id = this->id;
    hf->type = HEAD;
    hf->dst_id = flp->dst_id;
    flp->add(hf);

    //generate all the body flits
    uint8_t* ptr = (uint8_t*)data;
    
    //the flits count
    uint byte_count = 0;
    
    for ( uint i=0; i<no_bf; i++)
    {
        BodyFlit* bf = new BodyFlit();
        bf->type = BODY;
        bf->pkt_length = tot_flits;
        flp->add(bf);
        
        for (uint j = 0; j < link_width/8; j++)
        {
            if (byte_count < sizeof(T))
            {
                bf->data[j] = *ptr;
                ptr++;
                byte_count++;
            }
        }
    }

    //generate tail flits
    TailFlit* tf = new TailFlit();
    tf->type = TAIL;
    tf->pkt_length = tot_flits;
    flp->add(tf);

    assert( flp->has_whole_packet());
}



template<typename T>
void GenOneVcIrisInterface<T> :: print_stats(std::ostream& out)
{
    out << "Interface " << id << ":\n";
    out << "  Packets in from terminal: " << stat_packets_in_from_terminal << "\n";
    out << "  Packets out to terminal:  " << stat_packets_out_to_terminal << "\n";
    out << "  Packets in from router: " << stat_packets_in_from_router << "\n";
    out << "  Packets out to router:  " << stat_packets_out_to_router << "\n";
}




} //namespace iris
} //namespace manifold


#endif // MANIFOLD_IRIS_GENONEVCIRISINTERFACE_H
