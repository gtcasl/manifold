/*!
 * =====================================================================================
 *
 *       Filename:  genericrc.h
 *
 *    Description:  All routing algos are implemented here for the time being.
 *    Need to re arrange
 *
 *        Version:  1.0
 *        Created:  02/19/2010 11:54:57 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Mitchelle Rasquinha (), mitchelle.rasquinha@gatech.edu
 *        Company:  Georgia Institute of Technology
 *
 * =====================================================================================
 */
#ifndef  MANIFOLD_IRIS_GENERICRC_H
#define  MANIFOLD_IRIS_GENERICRC_H

#include	"../interfaces/genericHeader.h"
#include        "../data_types/flit.h"


namespace manifold {
namespace iris {


struct GenericRCSettings {
    unsigned grid_size;
    unsigned node_id;
    ROUTING_SCHEME rc_method;
    unsigned no_nodes;
};

class GenericRC
{
    public:
        GenericRC (unsigned vcs, const GenericRCSettings&);
        virtual ~GenericRC(){}
        void push( Flit* f, uint vc );
        uint get_output_port ( uint channel);
        uint get_virtual_channel ( uint ch );
        std::string toString() const;
        //uint no_adaptive_ports( uint ch );
        //uint no_adaptive_vcs( uint ch );
        //uint speculate_port ( Flit* f, uint ch );
        //uint speculate_channel ( Flit* f, uint ch );
        //bool is_empty();

#ifdef IRIS_TEST
    public:
#else
    protected:
#endif

	virtual void decide_vc_simple(HeadFlit* hf);
	virtual void decide_vc_for_ring(uint dest, uint my, HeadFlit* hf);

        std::vector < uint > possible_out_ports;
        std::vector < uint > possible_out_vcs;


#ifdef IRIS_TEST
    public:
#else
    private:
#endif
        const uint node_id;
        const uint no_nodes;
        uint grid_size;
        const ROUTING_SCHEME rc_method;

        //std::string name;
        void route_twonode( HeadFlit* hf );
        void route_torus( HeadFlit* hf );
        void route_torus6p( HeadFlit* hf );
        void route_ring( HeadFlit* hf );
        //uint route_x_y( uint addr );
        //void route_ring_uni( HeadFlit* hf );

        //bool do_request_reply_network;

        /*
         * =====================================================================================
         *        Class:  Address
         *  Description:  
         * =====================================================================================
         */
        class Address
        {
            public:
                bool route_valid;
                unsigned int channel;
                unsigned int out_port;
                uint last_adaptive_port;
                uint last_vc;
                std::vector < uint > possible_out_ports;
                std::vector < uint > possible_out_vcs;
        };

        std::vector<Address> addresses; //one per virtual channel, total Ports * VCs per port

}; /* -----  end of class GenericRC  ----- */



//request-reply network.
class ReqReplyRC : public GenericRC {
    public:
        ReqReplyRC (unsigned vcs, const GenericRCSettings& s) : GenericRC(vcs, s) {}

#ifdef IRIS_TEST
    public:
#else
    protected:
#endif
	virtual void decide_vc_simple(HeadFlit* hf);
	virtual void decide_vc_for_ring(uint dest, uint my, HeadFlit* hf);
};


} // namespace iris
} // namespace manifold


#endif // MANIFOLD_IRIS_GENERICRC_H

