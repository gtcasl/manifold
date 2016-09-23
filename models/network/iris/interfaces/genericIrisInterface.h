#ifndef MANIFOLD_IRIS_GENERICIRISINTERFACE_H
#define MANIFOLD_IRIS_GENERICIRISINTERFACE_H

/*
 * =====================================================================================
 *    Description:  new interface that can be connect all kinds of terminal
 *
 *        Version:  1.0
 *        Created:  08/29/2011 
 *
 *         Author:  Zhenjiang Dong
 *         School:  Georgia Institute of Technology
 *
 * =====================================================================================
 */

#include        "genericHeader.h"
#include        "mapping.h"
#include        "simulatedLen.h"
#include        "vnetAssign.h"
#include        "../data_types/linkData.h"
#include	"../data_types/flit.h"
#include	"../components/genericBuffer.h"
#include	"../components/simpleArbiter.h"
#include	"../components/simpleRouter.h"
#include        "kernel/component.h"
#include        "kernel/manifold.h"
#include        <iostream>
#include        <assert.h>


namespace manifold {
namespace iris {


//Network interface initialization parameters
struct inf_init_params{
    unsigned num_vc;
    unsigned linkWidth;
    unsigned num_credits;
    unsigned upstream_credits; //credits for the output link to terminal
    int up_credit_msg_type; //message type for credits to terminal
    unsigned upstream_buffer_size; //size of output buffer to terminal
};


class NetworkInterfaceBase: public manifold::kernel::Component 
{
    public:
        //NetworkInterfaceBase(){}
        NetworkInterfaceBase(unsigned ifid, const Terminal_to_net_mapping* m ) : id(ifid), term_ni_mapping(m) {}

        // ====================  Event handlers at the interface-router interface    =======================================
        virtual void handle_router ( int inputId, LinkData* data ) = 0;  //data or credit send form router

        // ====================  Clocked funtions =======================================
        virtual void tick (void) = 0;
        virtual void tock (void) = 0;

	unsigned get_id() { return id; }

	virtual void print_stats(std::ostream& out) {}

#ifndef IRIS_TEST
    protected:
#endif
        const unsigned id;
	const Terminal_to_net_mapping* term_ni_mapping; //terminal to network interface ID mapping

}; 



// *********** the generic interface start here ************

template<typename T>
class NIInit {
public:
    NIInit(const Terminal_to_net_mapping* m, SimulatedLen<T>* s, VnetAssign<T>* v) :
        mapping(m), slen(s), vnet(v)
	{}

    const Terminal_to_net_mapping* mapping;
    SimulatedLen<T>* slen;
    VnetAssign<T>* vnet;
};

//! Class T must support get_dst() which returns destination ID.
//!
//! Data structures used:
//!
//!       |    ...    |
//!       |-----------|  input_pkt_buffer; no size limit;     output_pkt_buffer holds packets going out
//!       |-----------|  holds packets from terminal          to terminal; its size is limited by
//!                                                           UPSTREAM_BUFFER_SIZE
//!
//!      -----------  proc_out_buffer: a vector;
//!     |  |  |  |  | one element for a VC; each               proc_in_buffer is similar.
//!      -----------  element holds a FlitLevelPacket.
//!
//! A packet is first converted to FlitLeverPacket and stored in proc_out_buffer; then flits are moved
//! to router_out_buffer one by one.
//!
//! On the incoming (from router) side, as flits arrive, they are put in router_in_buffer. Flits are then
//! pulled from router_in_buffer to proc_in_buffer until a whole FlitLevelePacket is assembled.
//!
//!      -----------  router_out_buffer: a vector; one
//!     |  |  |  |  | element for a VC; each a GenericBuffer   router_in_buffer is similar.
//!      -----------  holding flits.
//!
//!
template<typename T>
class GenNetworkInterface : public NetworkInterfaceBase
{
    public:
        //the enums that specifies the ports
        enum { ROUTER_PORT=0, TERMINAL_PORT};
        
        GenNetworkInterface (unsigned ifid, const NIInit<T>&, inf_init_params*);
        ~GenNetworkInterface (); 

        // Event handlers  (unsynchronized)
        void handle_new_packet_event( int port, T* data);
        void handle_router( int port, LinkData* data); 

        // Clocked funtions
        void tick (void);
        void tock (void);

	#ifdef FORECAST_NULL
        void do_output_to_terminal_prediction();
	#endif

	virtual void print_stats(std::ostream& out);

        void set_router(SimpleRouter* s) { m_router = s; }

	void dbg_print();

#ifndef IRIS_TEST
    protected:
#endif    
	#ifdef FORECAST_NULL
	//Override base class function.
        virtual void remote_input_notify(manifold::kernel::Ticks_t when, void* data, int port);
	#endif

#ifndef IRIS_TEST
    private:
#endif    
        const int CREDIT_PKT; //credit packets' message type

         // convert the pkt to flits(or inversely)
        void to_flit_level_packet(FlitLevelPacket* flp, T* data, manifold::kernel::Ticks_t enter_network_time);
        T* from_flit_level_packet(FlitLevelPacket* flp);

	bool try_send_to_terminal();
	void send_credit_to_terminal(T*); //used only by tock()

	void process_incoming_credit();

        const unsigned no_vcs; //# of virtual channel
        const unsigned credits; //# credits for each channel
	const int UPSTREAM_FULL_CREDITS; //credits for output link to terminal
        const unsigned LINK_WIDTH; //the link bandwidth in bits
        unsigned last_inpkt_winner; //This is the input VC from router where we pulled in a complete packet.
	                            //This is used so the VCs are processed in a round-robin manner.
        const unsigned UPSTREAM_BUFFER_SIZE;

        #ifdef IRIS_STATS_T2T_DELAY
	//if collect terminal-to-terminal delay
	struct PktWrapper {
	    PktWrapper(T* p, manifold::kernel::Ticks_t t) : pkt(p), enter_network_time(t) {}
            T* pkt;
	    manifold::kernel::Ticks_t enter_network_time; //time when the packet enters the network
	};
        std::list<PktWrapper> input_pkt_buffer;
        std::list<PktWrapper> output_pkt_buffer; //size limit of this buffer is UPSTREAM_BUFFER_SIZE
	#else
        //the infinite input buffer for the input from terminal
        std::list<T*> input_pkt_buffer;
        std::list<T*> output_pkt_buffer; //size limit of this buffer is UPSTREAM_BUFFER_SIZE
	#endif

	int upstream_credits; //credits for output link to terminal
        
        std::vector<FlitLevelPacket> proc_out_buffer; //A buffer where a packet is turned into flits; one per VC.

        GenericBuffer router_out_buffer; //Buffer for outgoing flits; from here flits enter the router.

        std::vector<bool> is_proc_out_buffer_free; //whether a slot of the proc_out_buffer is available.
	                                           //Note this is set to true when the whole packet has left the
						   //interface, NOT when the whole packet has been moved from
						   //proc_out_buffer to router_out_buffer.

        std::vector < int > downstream_credits; //the credit correspond for link to network

        GenericBuffer router_in_buffer; //input buffer for flits coming in from router.

        std::vector<FlitLevelPacket> proc_in_buffer; //A buffer where flits for the same packet are assembled.
	                                             //One per VC.
        
        //the arbiter used in interface
        SimpleArbiter* arbiter; 

	SimulatedLen<T>* simLen; //this object has a function that gives us the simulated length of a nework packet.
	VnetAssign<T>* vnet; //this object has a function that gives us the virtual network ID for a nework packet.
    

	#ifdef FORECAST_NULL
        std::list<manifold::kernel::Ticks_t> m_send_credit_tick; //record when we are going to send credit to terminal
        std::list<manifold::kernel::Ticks_t> m_recv_credit_tick; //record when a received credit is going to be processed

        std::list<manifold::kernel::Ticks_t> m_input_msg_ticks;
	#endif


        //stats
        uint64_t stat_packets_in_from_router; 
        uint64_t stat_sfpackets_in_from_router;  //single-flit packets
        uint64_t stat_packets_out_to_router;
        uint64_t stat_sfpackets_out_to_router;
	uint64_t stat_packets_in_from_terminal;
	uint64_t stat_packets_out_to_terminal;
	unsigned stat_max_input_buffer_length;
	unsigned stat_max_output_buffer_length;
	uint64_t stat_t2t_delay; //terminal-to-terminal delay; measures num of cycles it takes
	                         //for packets to go through the network to its destination

	SimpleRouter* m_router; //the router the NI is connected to
	unsigned stat_pred_called;
	unsigned stat_pred_made;
	unsigned stat_pred_no_flits_from_router;
};


//! the constructor
//! initializing all the vector to given size and assign initial value to them
//! @param \c i_p  # of virtual channels, # of credits for each channel, the link bandwidth in bits
template<typename T>
GenNetworkInterface<T>::GenNetworkInterface (unsigned ifid, const NIInit<T>& niInit, inf_init_params* i_p) :
	NetworkInterfaceBase(ifid, niInit.mapping),
	CREDIT_PKT(i_p->up_credit_msg_type),
        no_vcs(i_p->num_vc), credits(i_p->num_credits), UPSTREAM_FULL_CREDITS(i_p->upstream_credits), LINK_WIDTH(i_p->linkWidth),
	UPSTREAM_BUFFER_SIZE(i_p->upstream_buffer_size),
	router_out_buffer(i_p->num_vc, 6*i_p->num_credits),
	router_in_buffer(i_p->num_vc, 6*i_p->num_credits),
	simLen(niInit.slen),
	vnet(niInit.vnet)
{
    
    assert(LINK_WIDTH % 8 == 0); //LINK_WIDTH must be multiple of 8
    assert(i_p->upstream_credits > 0);
    assert(i_p->up_credit_msg_type != 0);
    assert(UPSTREAM_BUFFER_SIZE > 0);

    //initializing vectors to given size 
    proc_out_buffer.resize ( no_vcs );
    is_proc_out_buffer_free.resize(no_vcs);
    proc_in_buffer.resize(no_vcs);
    downstream_credits.resize( no_vcs );
    //router_ob_packet_complete.resize(no_vcs);

    arbiter = new FCFSSimpleArbiter(i_p->num_vc);

    last_inpkt_winner = -1; //set to -1 so the very first time VC 0 has highest priority.

    //initializing the values for each vector
    for ( uint i=0; i<no_vcs; i++)
    {
        is_proc_out_buffer_free[i] = true;
        //router_ob_packet_complete[i] = false;
        downstream_credits[i] = credits;
    }
    upstream_credits = UPSTREAM_FULL_CREDITS;


    // Init stats
    stat_packets_in_from_terminal = 0;
    stat_packets_out_to_terminal = 0;
    stat_packets_in_from_router = 0;
    stat_sfpackets_in_from_router = 0;
    stat_packets_out_to_router = 0;
    stat_sfpackets_out_to_router = 0;
    stat_max_input_buffer_length = 0;
    stat_max_output_buffer_length = 0;
    stat_t2t_delay = 0;

    m_router = 0;
    stat_pred_called = 0;
    stat_pred_made = 0;
    stat_pred_no_flits_from_router = 0;
}


//! the deconstructor
template<typename T>
GenNetworkInterface<T>::~GenNetworkInterface ()
{
    delete arbiter;
}

//! response for putting flits into buffer and add the downstream credit
//! @param \c port  The port # where the data come from
//! @param \c data  The link data come from network which can be two types: credit and flit
template<typename T>
void
GenNetworkInterface<T>::handle_router (int port, LinkData* data )
{   
    switch ( data->type ) {
        case FLIT:
            {
#ifdef DEBUG_IRIS_INTERFACE
std::cout << "@ " << manifold::kernel::Manifold::NowTicks() << " Interface " << id << " got flit from router: " << data->f->toString() << std::endl;
#endif

                if ( data->f->type == TAIL || data->f->pkt_length == 1 ) {
		    #ifdef STATS
                    stat_packets_in_from_router++;
		    if(data->f->pkt_length == 1)
			stat_sfpackets_in_from_router++;
		    #endif
		}
                
                //push the incoming flit to the router in buffer 
                router_in_buffer.push(data->vc, data->f);
                
                if ( data->f->type != TAIL && data->f->pkt_length != 1)
                {
                    //generate credit and send back to data source
                    LinkData* ld =  new LinkData();
                    ld->type = CREDIT;
                    ld->src = id;
                    ld->vc = data->vc;

#ifdef DEBUG_IRIS_INTERFACE
std::cout << "Interface " << id << " SEND Head/Body CREDIT vc " << ld->vc << std::endl;
#endif
                    //send credit to router
                    Send(ROUTER_PORT, ld);
                }
                break;
            }

        case CREDIT:
            {
                downstream_credits[data->vc]++;
                assert(downstream_credits[data->vc] <= (int)credits);
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
GenNetworkInterface<T>::handle_new_packet_event (int port, T* data )
{  
    assert(port == TERMINAL_PORT);
    if(data->get_type() == CREDIT_PKT) {
        //upstream_credits++;
	//assert(upstream_credits <= UPSTREAM_FULL_CREDITS);
	manifold::kernel::Manifold::Schedule(1,&GenNetworkInterface::process_incoming_credit, this);
	#ifdef FORECAST_NULL
	m_recv_credit_tick.push_back(manifold::kernel::Manifold::NowTicks() + 1); //?????????????????????? NI should have a ref to its clock
	#endif
	                                                                          //instead of using the default.
	delete data;
    }
    else {

#ifdef DBG_IRIS
    T* pkt = (T*) data;
    cout << "@ " << manifold::kernel::Manifold::NowTicks() << " iris received pkt src= " << pkt->get_src() << " port= " << pkt->get_src_port() << " dst= " << pkt->get_dst() << " port= " << pkt->get_dst_port() <<endl;
#endif
	#ifdef IRIS_STATS_T2T_DELAY
	input_pkt_buffer.push_back(PktWrapper(data, manifold::kernel::Manifold::NowTicks()));
	#else
	input_pkt_buffer.push_back(data);
	#endif
	#ifdef STATS
	stat_packets_in_from_terminal++;
	if(input_pkt_buffer.size() > stat_max_input_buffer_length)
	    stat_max_input_buffer_length = input_pkt_buffer.size();
	#endif
    }
}



template<typename T>
void GenNetworkInterface<T>::process_incoming_credit()
{  
    upstream_credits++;
    assert(upstream_credits <= UPSTREAM_FULL_CREDITS);
}


//! Generate a new packet when all the flits in a packet are received; flits are deleted.
//! @param \c flp  Pointer to a FlitLevelPacket, which is a container that holds all flits for a packet.
//! @return  Pointer to a packet.
template<typename T>
T*
GenNetworkInterface<T>::from_flit_level_packet(FlitLevelPacket* flp)
{
    T* message = new T;
    uint8_t* ptr = (uint8_t*) message;
    unsigned byte_count = 0;
    
    while(flp->size() > 0)
    {
        Flit* f = flp->pop_next_flit ();
        if (f->type == HEAD || f->type == BODY)
        {   
	    uint8_t* data = 0;
	    if(f->type == HEAD)
	        data = static_cast<HeadFlit*>(f)->data;
	    else
	        data = static_cast<BodyFlit*>(f)->data;
            for (int j = 0; j < HeadFlit::MAX_DATA_SIZE; j++)
            {
                if(byte_count < sizeof(T)) {
		    *ptr = *data;
		    ptr++;
		    data++;
		    byte_count++;
		}
		else
		    break;
            }
        } 
	delete f;
    }
    
    return message;
}


//! 1. For each VC, move one flit from proc_out_buffer to router_out_buffer, if available.
//! 2. Make arbitration for each VC with a flit to send. The winner leaves the interface and enter the router.
//! 3. Check proc_in_buffer to see if entire packets have been received; if so, deliver packets to terminal.
//! 4. For each VC, move one flit from router_in_buffer to proc_in_buffer, if possible.
//!
template<typename T>
void
GenNetworkInterface<T>::tick ( void )
{
    // For each VC, move one flit from the proc_out_buffer (which holds FlitLevelPkt) to the
    // router_out_buffer (which holds individual flits)
    for ( uint i=0 ; i<no_vcs ; i++ )
    {
        if ( proc_out_buffer[i].size()>0 )
        {
            //router_out_buffer is flit level buffer
            Flit* f = proc_out_buffer[i].pop_next_flit();
            f->virtual_channel = i;
            router_out_buffer.push(i, f);
        }
    }
    
    //Arbitration request. Only make a request when there's credit for sending.
    for ( uint i=0; i<no_vcs ; i++ ) {
        if( downstream_credits[i]>0 && router_out_buffer.get_occupancy(i)>0)
        {
            Flit* f= router_out_buffer.peek(i);
            if((f->type != HEAD)||( f->type == HEAD  && downstream_credits[i] == (int)credits))
            {
		if(!arbiter->is_requested(i))
		    arbiter->request(i);
            }
        }
    }

    // Pick a winner for the arbiter
    uint winner = arbiter->pick_winner();
    if(winner < no_vcs) {
        arbiter->clear_request(winner);

        // send out a flit as credits were already checked earlier this cycle
        Flit* f = router_out_buffer.pull(winner);

        //reduce the local downstream credit
        downstream_credits[winner]--;

        //if this is the last flit the proc out buffer will available again
        if ( f->type == TAIL || f->pkt_length == 1 )
        {
            //router_ob_packet_complete[winner] = false;
	    //mark the proc_out_buffer slot as free
            is_proc_out_buffer_free[winner] = true;
	    #ifdef STATS
	    stat_packets_out_to_router++;
	    if(f->pkt_length == 1)
	        stat_sfpackets_out_to_router++;
	    #endif
        }

        //send the flit over the winner channel
        LinkData* ld =  new LinkData();
        ld->type = FLIT;
        ld->src = this->id; //ld->src only useful for debug
        ld->f = f;
        ld->vc = winner;

#ifdef DEBUG_IRIS_INTERFACE
std::cout << "Interface " << id << " SEND FLIT to router on VC " << winner << " Flit is " << f->toString() << std::endl;
#endif    
        //send data to router
        Send(ROUTER_PORT, ld);
    }



    //Now process inputs from router

    //first try to send a packet from the output buffer
    bool sent_one = try_send_to_terminal();

    //Pull entire packets from proc_in_buffer in a round-robin manner.
    bool found = false;
    //decide which vc to use under the FIFO principal
    const unsigned start_ch = last_inpkt_winner + 1;
    for ( uint i=0; i<no_vcs ; i++) {
        unsigned ch = start_ch + i;
	if(ch >= no_vcs) //wrap around
	    ch -= no_vcs;
	if(proc_in_buffer[ch].has_whole_packet())
	{
	    //a whole packet has been received.
            last_inpkt_winner= ch;
            found = true;
            break;
        }
    }

    //find the finished flit level pkt convert the filts to packet
    if ( found && (output_pkt_buffer.size() < UPSTREAM_BUFFER_SIZE) )
    {
        T* np = from_flit_level_packet(&proc_in_buffer[last_inpkt_winner]);
        #ifdef IRIS_STATS_T2T_DELAY
	output_pkt_buffer.push_back(PktWrapper(np, proc_in_buffer[last_inpkt_winner].enter_network_time));
	#else
	output_pkt_buffer.push_back(np);
	#endif

	#ifdef STATS
	if(output_pkt_buffer.size() > stat_max_output_buffer_length)
	    stat_max_output_buffer_length = output_pkt_buffer.size();
	#endif

	if(!sent_one) //each tick we only send one packet out.
	    try_send_to_terminal();
	#if 0
        T* np = from_flit_level_packet(&proc_in_buffer[last_inpkt_winner]);
	assert(proc_in_buffer[last_inpkt_winner].size() == 0);

        Send(TERMINAL_PORT, np);
	stat_packets_out_to_terminal++;
	#endif
          
            
        //  Send the tail credit back. All other flit credits were sent
        //  as soon as flit was got in link-arrival
        LinkData* ld =  new LinkData();
        ld->type = CREDIT;

        ld->src = this->id;
        ld->vc = last_inpkt_winner;

#ifdef DEBUG_IRIS_INTERFACE
std::cout << "Interface " << id << " SEND CREDIT to router on vc " << ld->vc << " after delivering packet to terminal." << std::endl;
#endif
        //send credit for the tail flit to router
        Send(ROUTER_PORT,ld);
    }



    //handle receiving flits: push flits coming in from router in buffer to the in buffer
    for ( uint i=0; i<no_vcs ; i++ )
    {
        if( router_in_buffer.get_occupancy(i) > 0 &&  proc_in_buffer[i].has_whole_packet() == false)
        { 
            Flit* ptr = router_in_buffer.pull(i);
	    proc_in_buffer[i].add(ptr);         

	    if(ptr->type == HEAD)
	        proc_in_buffer[i].enter_network_time = (static_cast<HeadFlit*>(ptr))->enter_network_time;
        }
    }
}


//! transfer the incoming packet to proc_out_buffer and decide which vc is should use
template<typename T>
void
GenNetworkInterface<T>::tock ( void )
{
    // Take packets from input_pkt_buffer, convert them into flits, and store in free slots of proc_out_buffer
    for ( uint i=0; i < no_vcs; i++ )
    {
        if (is_proc_out_buffer_free[i] && downstream_credits[i] == (int)credits && !input_pkt_buffer.empty())
        {  
            //convert the mem_req to flits and copy it to out buffer
            manifold::kernel::Ticks_t enter_net_time = 0;

	    #ifdef IRIS_STATS_T2T_DELAY
	    T* pkt = input_pkt_buffer.front().pkt;
	    enter_net_time = input_pkt_buffer.front().enter_network_time;
	    #else
	    T* pkt = input_pkt_buffer.front();
	    #endif

	    //current supports 2 virtual networks; packets for virtual network 0 will be moved to even-numbered
	    //proc_out_buffer location and packets for virtual net 1 will be moved to odd-numbered proc_out_buffer
	    //location.
	    if(vnet->get_virtual_net(pkt) == 0)
	    {
	        if(i%2 == 1)
	            continue;
	    }
	    else
	    {
	        assert(vnet->get_virtual_net(pkt) == 1);

	        if(i%2 == 0)
	            continue;
	    }

            to_flit_level_packet( &proc_out_buffer[i], pkt, enter_net_time);

            //assign virtual channel
            proc_out_buffer[i].virtual_channel = i;
               
            is_proc_out_buffer_free[i] = false;
            assert( proc_out_buffer[i].size() != 0 );
            
            //remove the element in pkt buffer
	    int src_port = pkt->get_src_port();
	    delete pkt;
            input_pkt_buffer.pop_front();

	    //send a credit back
	    T* credit = new T;
	    credit->set_type(CREDIT_PKT);
	    credit->set_dst_port(src_port);
	    //Send(TERMINAL_PORT, credit);
	    //use schedule Half to ensure credit is sent on rising edge.
	    manifold::kernel::Manifold::ScheduleHalf(1, &GenNetworkInterface::send_credit_to_terminal, this, credit);
	    #ifdef FORECAST_NULL
	    m_send_credit_tick.push_back(manifold::kernel::Manifold::NowTicks() + 1);
	    #endif
        }
    }
}


template<typename T>
void
GenNetworkInterface<T> :: send_credit_to_terminal(T* credit)
{
    #ifdef FORECAST_NULL
    assert(m_send_credit_tick.size() > 0);
    assert(m_send_credit_tick.front() == manifold::kernel::Manifold::NowTicks());
    m_send_credit_tick.pop_front();
    if(m_input_msg_ticks.size() > 0)
	m_input_msg_ticks.pop_front();
    #endif

    Send(TERMINAL_PORT, credit);
//cout << "SSSSS @ " << manifold::kernel::Manifold::NowTicks() << " NI " << get_id() << " sends credit.\n";
//cout.flush();
}


//! Take an input packet and create a FlitLevelPacket.
//! @param \c pkt  The packet from terminal
//! @param \c flp  The point of outbound proc_out_buffer
//! @param \c enter_time  The clock cycle when the packet enters the network interface from the terminal.
template<typename T>
void
GenNetworkInterface<T>::to_flit_level_packet(FlitLevelPacket* flp, T* pkt, manifold::kernel::Ticks_t enter_time)
{
    assert(flp->size() == 0);

    //calculate # of flits needed to send a pkt
    //The number of flits is determined by the simulated length of the packet, not
    //the actual length. On the other hand, everything in the actual packet must be transmitted.

    unsigned num_bytes = HeadFlit :: HEAD_FLIT_OVERHEAD + simLen->get_simulated_len(pkt);
    unsigned num_flits = num_bytes * 8 / LINK_WIDTH;
    if(num_bytes * 8 % LINK_WIDTH != 0)
        num_flits++;

    //ensure the actual packet fits in the number of flits allocated.
    assert((int)(sizeof(T)/num_flits) <= HeadFlit :: MAX_DATA_SIZE);

    //generate the flit level packet
    unsigned tot_flits = num_flits;
    if(num_flits > 1)
        tot_flits++; //If data doesn't fit in headflit, we also need tail flit.

    flp->set_pkt_length(tot_flits);

    //convert to network address
    flp->dst_id = term_ni_mapping->terminal_to_net(pkt->get_dst());
    flp->src_id = this->id;
    
    //generate the header flit
    HeadFlit* hf = new HeadFlit();
    hf->pkt_length = tot_flits;
    hf->src_id = this->id;
    hf->dst_id = flp->dst_id;
    hf->enter_network_time = enter_time;

    if(vnet->get_virtual_net(pkt) == 0)
        hf->mclass = PROC_REQ;
    else {
	assert(vnet->get_virtual_net(pkt) == 1);
        hf->mclass = MC_RESP;
    }

    //cout << "pkt type: " << pkt->get_type() << ", dst: " << pkt->get_dst() << " port: " << pkt->get_dst_port() << ", src: " << pkt->get_src() << " port: " << pkt->get_src_port() << ", src id: " << hf->src_id << ", dst id: " << hf->dst_id << endl;

    if (pkt->get_dst_port() == manifold::uarch::LLP_ID || pkt->get_dst_port() == manifold::uarch::LLS_ID ) { //LLP::LLP_ID, LLP::LLS_ID
        hf->term = CACHE;
    } else if (pkt->get_type() == 456) { //m_MEM_MSG_TYPE
        hf->term = MEMORY;
    } else {
        cout << "Invalid terminal distination" << endl;
        exit(1);
    }

    //copy data to head flit
    unsigned byte_count = 0;

    uint8_t* ptr = (uint8_t*)pkt;

    for (int j = 0; j < HeadFlit::MAX_DATA_SIZE; j++)
    {
	if (byte_count < sizeof(T))
	{
	    hf->data[j] = *ptr;
	    ptr++;
	    byte_count++;
	}
	else {
	    break;
        }
    }
    
    hf->data_len = byte_count;
    flp->add(hf);
    
    //copy data to body flits
    for (int i=0; i<(int)num_flits-1; i++) //num_flits includes one head flit
    {
        BodyFlit* bf = new BodyFlit();
        bf->type = BODY;
        bf->pkt_length = tot_flits;
        flp->add(bf);
        
        for (int j = 0; j < HeadFlit::MAX_DATA_SIZE; j++)
        {
            if (byte_count < sizeof(T))
            {
                bf->data[j] = *ptr;
                ptr++;
                byte_count++;
            }
	    else
	       break;
        }
    }

    //generate tail flits
    if(num_flits > 1) {
	TailFlit* tf = new TailFlit();
	tf->type = TAIL;
	tf->pkt_length = tot_flits;
	flp->add(tf);
    }

    assert( flp->has_whole_packet());
}

//! @return true if a packet is sent; false otherwise.
template<typename T>
bool GenNetworkInterface<T> :: try_send_to_terminal()
{
    if (!output_pkt_buffer.empty() && upstream_credits > 0) {
        #ifdef IRIS_STATS_T2T_DELAY
	T* pkt = output_pkt_buffer.front().pkt;
	stat_t2t_delay += (manifold::kernel::Manifold::NowTicks() - output_pkt_buffer.front().enter_network_time);
	#else
	T* pkt = output_pkt_buffer.front();
	#endif
	output_pkt_buffer.pop_front();
        Send(TERMINAL_PORT, pkt);
//cout << "SSSSS @ " << manifold::kernel::Manifold::NowTicks() << " NI " << get_id() << " sends packet.\n";
//cout.flush();
	#ifdef STATS
	stat_packets_out_to_terminal++;
	#endif
        upstream_credits--;
	return true;
   }

   return false;
}


template<typename T>
void GenNetworkInterface<T> :: print_stats(std::ostream& out)
{
    out << "Interface " << id << ":\n";
    out << "  Packets in from terminal: " << stat_packets_in_from_terminal << "\n";
    out << "  Packets out to terminal:  " << stat_packets_out_to_terminal << "\n";
    out << "  Total / single-flit packets in from router: " << stat_packets_in_from_router << " / " << stat_sfpackets_in_from_router << "\n";
    out << "  Total / single-flit packets out to router:  " << stat_packets_out_to_router << " / " << stat_sfpackets_out_to_router << "\n";
    out << "  Max input buffer size:  " << stat_max_input_buffer_length << "\n";
    out << "  Max output buffer size:  " << stat_max_output_buffer_length << "\n";

    //out << " pred made= " << stat_pred_made << " router_empty= " << stat_pred_no_flits_from_router << endl;
    #ifdef IRIS_STATS_T2T_DELAY
    out << "  Avg terminal-to-terminal delay: " << (double)stat_t2t_delay/stat_packets_out_to_terminal << "\n";
    #endif
}


template<typename T>
void GenNetworkInterface<T> :: dbg_print()
{
    cout << "Cycle= " << manifold::kernel::Manifold::NowTicks() << " interface " << get_id() << "\n"
         << "  proc_in_buffer occupancy: ";
    for(unsigned i=0; i<no_vcs; i++) {
        cout << proc_in_buffer[i].size() << " ";
    }
    cout << endl;
    cout << "  router_in_buffer occupancy: ";
    for(unsigned i=0; i<no_vcs; i++) {
        cout << router_in_buffer.get_occupancy(i) << " ";
    }
    cout << endl;
}





#ifdef FORECAST_NULL
template<typename T>
void GenNetworkInterface<T> :: remote_input_notify(manifold::kernel::Ticks_t when, void* data, int port)
{
    T* pkt = (T*)data;
    if(pkt->get_type() == CREDIT_PKT) {
//cout << "NI " << get_id() << " input notify " << when << "  is credit\n";
	return;
    }
    else {
//cout << "NI " << get_id() << " input notify " << when << "  is msgggggggg\n";
        m_input_msg_ticks.push_back(when);
    }
}


template<typename T>
void GenNetworkInterface<T> :: do_output_to_terminal_prediction()
{
//stat_pred_called++;
    if(this->border_ports.size() <= TERMINAL_PORT) {
	assert(this->border_ports.size() == 0); //NI is not connected to another LP
	return;
    }


    manifold::kernel::BorderPort* bp = this->border_ports[TERMINAL_PORT];
    assert(bp != 0);
    //currently Forcast Null Message only supports models with a single clock.
    assert(bp->get_clk() == &(manifold::kernel::Clock::Master()));

    manifold::kernel::Ticks_t now = this->get_clock()->NowTicks();


    if(m_send_credit_tick.size() > 0 && m_send_credit_tick.front() == now) {
        //there is a credit scheduled for the current tick
//cout << "@ " << manifold::kernel::Manifold::NowTicks() << " NI " << get_id() << " no pred - credit scheduled\n";
	bp->update_output_tick(now);
        return; 
    }


    if (!output_pkt_buffer.empty()) {
	//if there is a packet in output buffer ready to send, then no prediction
//cout << "@ " << manifold::kernel::Manifold::NowTicks() << " NI " << get_id() << " no pred - output buffer\n";
	bp->update_output_tick(now);
        return;
    #if 0
        if(upstream_credits > 0) {
//cout << "@ " << manifold::kernel::Manifold::NowTicks() << " NI " << get_id() << " no pred - out buffer has packet, up_credits= " << upstream_credits << "\n";
	    return;
	}

	bool credit_scheduled = false; //credit scheduled for this tick
	while(m_recv_credit_tick.size() > 0 && m_recv_credit_tick.front() <= manifold::kernel::Manifold::NowTicks()) {
	    if(m_recv_credit_tick.front() == manifold::kernel::Manifold::NowTicks())
	        credit_scheduled = true;
	    m_recv_credit_tick.pop_front();
	}

        if(credit_scheduled) {
//cout << "@ " << manifold::kernel::Manifold::NowTicks() << " NI " << get_id() << " no pred - out buffer has packet, credit scheduled\n";
	    return;
	}
#endif
    }

    //Pull entire packets from proc_in_buffer in a round-robin manner.
    bool found = false;
    const unsigned start_ch = last_inpkt_winner + 1;
    for ( unsigned i=0; i<no_vcs ; i++) {
        unsigned ch = start_ch + i;
	if(ch >= no_vcs) //wrap around
	    ch -= no_vcs;
	if(proc_in_buffer[ch].has_whole_packet())
	{
	    //a whole packet has been received.
            found = true;
            break;
        }
    }

    //if ( found && (output_pkt_buffer.size() < UPSTREAM_BUFFER_SIZE) ) {
    if (found) {
//cout << "@ " << manifold::kernel::Manifold::NowTicks() << " NI " << get_id() << " no pred - whole packet\n";
	bp->update_output_tick(now);
        return;
    }

        manifold::kernel::Ticks_t when = 0; //the tick for the next possible output

	if(m_input_msg_ticks.size() > 0 ) {
	    if(m_input_msg_ticks.front() < now)
		when = 1;
	    else
	        when = m_input_msg_ticks.front() - now;
	}
	else {
	    manifold::kernel::Ticks_t ticks[no_vcs]; //tick for next possible output

	    bool router_consulted = false;
	    manifold::kernel::Ticks_t tick_from_router = 0;

            for(unsigned i=0; i<no_vcs; i++) {
	        if(proc_in_buffer[i].size() > 0) {
		    ticks[i] = proc_in_buffer[i].get_pkt_length() - proc_in_buffer[i].size(); //# of flits remaining
//cout << "NI " << get_id() << " proc_in_buffer " << i << " not empty, len-size= " << ticks[i] << "\n";
		}
		else {//proc_in_buffer[i] is empty
		    //look into router_in_buffer
		    if(router_in_buffer.get_occupancy(i) > 0) {
		        Flit* hf = router_in_buffer.peek(i);
			assert(hf);
			assert(hf->type == HEAD);
			//since proc_in_buffer[i] is empty, and we can move at most 1 flit per tick to proc_in_buffer,
			//it will take pkt_length ticks to get the entire packet into proc_in_buffer[i], then 1 more
			//tick to deliver to terminal
			//NOTE: current tick hasn't bee processed.
			ticks[i] = hf->pkt_length - 1 + 1;
//cout << "NI " << get_id() << " proc_in_buffer empty, router_in_buf " << i << " not empty, pktlen= " << ticks[i] << "\n";
		    }
		    else { //router_in_buffer(i) empty
		        //pkt_len -1 + 2; min pkt_le is 1
			ticks[i] = 2;
			if(router_consulted == false) {
			    tick_from_router = m_router->earliest_to_NI();
			    router_consulted = true;
			    if(tick_from_router == 3)
			        stat_pred_no_flits_from_router++;
			}
			ticks[i] = tick_from_router + 2; //next tick it will be in proc_in_buffer; the next after
			                                 //that for delivery
//cout << "NI " << get_id() << " proc_in_buffer empty, router_in_buf empty " << i << " pred= " << ticks[i] << "\n";
		    }
		}
	    }

	    when = ticks[0];
	    for(unsigned int i=1; i<no_vcs; i++) {
	        if(ticks[i] < when)
		    when = ticks[i];
	    }
	}

	when += now;
//cout << "@ " << manifold::kernel::Manifold::NowTicks() << " NI " << get_id() << " do pred, tick= " << when << "\n";

	bp->update_output_tick(when);

//stat_pred_made++;


}
#endif //#ifdef FORECAST_NULL




} //namespace iris
} //namespace manifold


#endif // MANIFOLD_IRIS_GENERICIRISINTERFACE_H
