/*
 * =====================================================================================
 *
 *       Filename:  flit.h
 *
 *    Description:  Classes for network packets
 *
 *        Version:  1.0
 *        Created:  02/12/2011 04:53:48 PM
 *       Revision:  none
 *       Compiler:  g++
 *
 *         Author:  Mitchelle Rasquinha
 *        Company:  Georgia Institute of Technology
 *
 * =====================================================================================
 */
#ifndef  MANIFOLD_IRIS_FLIT_H
#define  MANIFOLD_IRIS_FLIT_H

#include	"../interfaces/genericHeader.h"
#include	<deque>
#include	<stdint.h>


namespace manifold {
namespace iris {


enum flit_type {UNK, HEAD, BODY, TAIL };

/*
 * =====================================================================================
 *        Class:  Phit
 *  Description: Subdividing a phit from a flit. But for now 1Phit = 1Flit 
 * =====================================================================================
 */

class Phit
{
    public:
        Phit();
        ~Phit();
        flit_type ft;

    protected:

    private:

};

/*
 * =====================================================================================
 *        Class:  Flit
 *  Description:  This object defines a single flow control unit. 
 *  FROM TEXT: Flow control is a synchronization protocol for transmitting and
 *  recieving a unit of information. This unit of flow control refers to that
 *  portion of the message whoose transfer must be syncronized. A packet is
 *  divided into flits and flow control between routers is at the flit level. It
 *  can be multicycle depending on the physical channel width.
 * =====================================================================================
 */
// Do not create virtual functions as flit might be sent across LP boundaries.
class Flit
{
    public:
        Flit ();
        ~Flit ();

        flit_type type;
        uint virtual_channel;
        uint pkt_length;
        std::string toString() const;

#ifdef IRIS_DBG
        unsigned flit_id;
	static unsigned NextId;
#endif

};


/*
 * =====================================================================================
 *        Class:  HeadFlit
 *  Description:  All pkt stat variables are passed within the head flit for
 *  simulation purposes. Not passing it in tail as some pkt types may not need
 *  more than one flit. 
 * =====================================================================================
 */
class HeadFlit: public Flit
{
    public:
        static const int HEAD_FLIT_OVERHEAD = 8; //head flit has 8 bytes of overhead
        static const int MAX_DATA_SIZE = 128;

        HeadFlit ();  
        void populate_head_flit(void);
        std::string toString() const;


        uint src_id; //id of src and dst of network interface.
        uint dst_id;
        message_class mclass;

        uint8_t data[MAX_DATA_SIZE]; //support single-flit packets
	int data_len;

        uint64_t enter_network_time;
};



/*
 * =====================================================================================
 *        Class:  BodyFlit
 *  Description:  flits of type body 
 * =====================================================================================
 */
class BodyFlit : public Flit
{
    public:
        BodyFlit (); 

        std::string toString() const;
        void populate_body_flit();
        
        uint8_t data[HeadFlit::MAX_DATA_SIZE]; //payload data
};


/*
 * =====================================================================================
 *        Class:  TailFlit
 *  Description:  flits of type TAIL. Has the pkt sent time. 
 * =====================================================================================
 */
class TailFlit : public Flit
{
    public:
        TailFlit ();

        void populate_tail_flit();
        std::string toString() const;
};


/*
 * =====================================================================================
 *        Class:  FlitLevelPacket
 *  Description:  This is the desription of the lower link layer protocol
 *    class. It translates from the higher message protocol definition of the
 *    packet to flits. Phits are further handled for the physical layer within
 *    this definition. 
 * =====================================================================================
 */

class InvalidAddException {};

class FlitLevelPacket
{
    public:
        FlitLevelPacket ();
        ~FlitLevelPacket ();

	unsigned get_pkt_length() { return pkt_length; }
	void set_pkt_length(unsigned len);

        uint src_id; //src and dst interface id; only useful for debugging
        uint dst_id;
        uint virtual_channel;
        uint64_t enter_network_time; //when the packet enters network

        void add ( Flit* ptr);  /* Appends an incoming flit to the pkt. */
        Flit* pop_next_flit();  /* This will pop the flit from the queue as well. */
        unsigned size();            /* Return the length of the pkt in flits. */
        bool has_whole_packet();    //true if it contains all flits of a single packet.
        std::string toString () const;
	void dbg_print();

#ifndef IRIS_TEST
    private:
#endif
        std::deque<Flit*> flits;
        uint pkt_length; //number of flits in the packet

}; /* -----  end of class FlitLevelPacket  ----- */



} // namespace iris
} // namespace manifold

#endif   //MANIFOLD_IRIS_FLIT_H

