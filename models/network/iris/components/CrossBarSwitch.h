/*
 * =====================================================================================
 *
 *       Filename:  CrossBarSwith.h
 *
 *    Description:  a n*n swith network without virtual channel feature and pipeline
 *
 *        Version:  1.0
 *        Created:  10/29/2011 
 *       Revision:  none
 *       Compiler:  g++/mpicxx
 *
 *         Author:  Zhenjiang Dong
 *         school:  Georgia Institute of Technology
 * =====================================================================================
 */
 
#ifndef  MANIFOLD_IRIS_CROSSBARSWITCH_H
#define  MANIFOLD_IRIS_CROSSBARSWITCH_H

#include	"../interfaces/genericHeader.h"
#include	"../data_types/linkData.h"
#include        "kernel/component.h"

namespace manifold {
namespace iris {


//one port to one interface
struct InBufferState
{
    public:
        int input_port; //input port #
        int output_port; //output port #
        ullint pkt_in_time; //pkt in time
#ifdef IRIS_DEBUG
        uint fid;
#endif
};
 
//the parameters needed to initialize switch 
struct CB_swith_init_params {
    uint no_nodes;
    uint credits;
};

//! on/off switch
class CrossBarSwitch: public manifold::kernel::Component 
{
     public:
 
        /* ====================  LIFECYCLE     ======================================= */
        CrossBarSwitch();
        CrossBarSwitch(const CB_swith_init_params* i_p); 
        ~CrossBarSwitch (); 

        /* ====================  Event handlers     ======================================= */
        void handle_inf_arrival( int inputid, LinkData* data); 

        /* ====================  Clocked funtions  ======================================= */
        void tick (void);
        void tock (void);

        /* ====================  print out status ======================================= */
        std::string print_stats ( void ); 
        
#ifndef IRIS_TEST
    private:
#endif
        //void do_input_buffering(HeadFlit*, uint, uint);
	const unsigned ports; //equal to no of interfaces
        const unsigned credits;
        std::vector <InBufferState> input_buffer_state; //record down the buffer status (in/out port, pkt in time)
        std::vector < std::deque<uint> > order_record; //record down the order of input flits such that guarantee FIFO of the output
        std::vector < bool > channel_state_record; //record down which output channel are in use, true->in use, false->available
        std::vector < std::deque<Flit*> > in_buffer; //input flit buffer
	std::vector<uint> downstream_credits; //credit

	//status record
	uint64_t stat_packets_in; //total # input pkt
        uint64_t stat_packets_out; //total # output pkt
        uint64_t stat_flits_in; //total # input flit
        uint64_t stat_flits_out; //total # output flit
        std::vector<ullint> cumu_port_delay; //cumulated delay for each port
        std::vector<ullint> port_pkt_count; //total # of pkt out from each port
        std::vector<ullint> port_lst_pkt_out; //cycle count for last pkt out from one port 
        ullint switch_ave_delay; //the evarage delay for switch
}; 


} //namespace iris
} //namespace manifold


#endif // MANIFOLD_IRIS_CROSSBARSWITCH_H
