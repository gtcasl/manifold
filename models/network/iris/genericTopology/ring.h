/*
 * =====================================================================================
 *    Description:  new interface that can be connect all kinds of terminal
 *
 *        Version:  1.0
 *        Created:  10/10/2011 
 *
 *         Author:  Zhenjiang Dong
 *         School:  Georgia Institute of Technology
 *
 * =====================================================================================
 */

#ifndef  MANIFOLD_IRIS_RING_H
#define  MANIFOLD_IRIS_RING_H

#include "../interfaces/genericIrisInterface.h"
#include "../components/simpleRouter.h"


namespace manifold {
namespace iris {

struct ring_init_params {
    ring_init_params() : no_nodes(0), no_vcs(0), credits(0), link_width(0), ni_up_credits(0), ni_upstream_buffer_size(0) {}
    uint no_nodes;
    uint no_vcs;
    uint credits;
    ROUTING_SCHEME rc_method; //may be useful to ditinguish unidirection and bidirection rings.
    uint link_width;
    unsigned ni_up_credits; //network interface credits for output to terminal.
    int ni_upstream_buffer_size; //network interface's output buffer (to terminal) size
};



template <typename T>
class Ring 
{
    public:
        Ring (manifold::kernel::Clock& clk, ring_init_params* params, const Terminal_to_net_mapping* m, SimulatedLen<T>*, VnetAssign<T>*, int ni_credit_type, uint lp_inf, uint lp_rt);
        ~Ring ();

        //connect all components together
        void connect_interface_routers(void);
        void connect_routers(void);
        
        const std::vector <GenNetworkInterface<T>*>& get_interfaces() { return interfaces; }
	const std::vector <SimpleRouter*>& get_routers() {return routers; }

        //the interfaces' component id
        const std::vector <manifold::kernel::CompId_t>& get_interface_id() { return interface_ids; }
        
	void print_stats(std::ostream&);

#ifndef IRIS_TEST
    private:
#endif
       // variables
       const unsigned no_nodes; //# of routers
       manifold::kernel::Clock& clk_r; //the clock for routers and interfaces 
       std::vector <SimpleRouter*> routers; //the routers
       std::vector <GenNetworkInterface<T>*> interfaces; //the interfaces
       std::vector <manifold::kernel::CompId_t> router_ids; //the routers' component ID
       std::vector <manifold::kernel::CompId_t> interface_ids; //the interfaces' component ID 
       
#ifdef IRIS_TEST  
       //trace file that using in debugging     
       std::ofstream outFile_data; 
       std::ofstream outFile_signal;
#endif
       
   protected:     
}; 

//! response for creating the ring topology 
//! @param \c clk  The clock passing from callor
//! @param \c params  The configure parameters for ring network
//! @param \c ni_credit_type  The message type for network interface's credits to terminal.
//! @param \c lp_inf  The logic process id for interfaces
//! @param \c lp_rt  The logic process id for routers
template <typename T>
Ring<T>::Ring(manifold::kernel::Clock& clk, ring_init_params* params, const Terminal_to_net_mapping* mapping, SimulatedLen<T>* slen, VnetAssign<T>* vn, int ni_credit_type, uint lp_inf, uint lp_rt) :
    no_nodes(params->no_nodes),
    clk_r(clk)
{
#ifdef IRIS_TEST
    //open trace file
    outFile_data.open("ring_data.tr");

    //handle exception
    if(outFile_data.fail())
       std::cout<<"fail to open file"<<std::endl;
       
    //open trace file
    outFile_signal.open("ring_signal.tr");

    //handle exception
    if(outFile_signal.fail())
       std::cout<<"fail to open file"<<std::endl;   
#endif

    assert(params->ni_up_credits > 0);
    assert(ni_credit_type != 0);
    assert(params->ni_upstream_buffer_size > 0);

    //parameters for interface
    inf_init_params i_p_inf;
    
    //parameters for interface
    i_p_inf.linkWidth = params->link_width;
    i_p_inf.num_credits = params->credits;
    i_p_inf.upstream_credits = params->ni_up_credits;
    i_p_inf.up_credit_msg_type = ni_credit_type;
    i_p_inf.num_vc = params->no_vcs; 
    i_p_inf.upstream_buffer_size = params->ni_upstream_buffer_size;
    

    //parameters for router
    router_init_params i_p_rt;
            
    //parameters for router
    i_p_rt.no_nodes = params->no_nodes;
    i_p_rt.grid_size = params->no_nodes; //defualt in simplerouter
    i_p_rt.no_ports = 3;
    i_p_rt.no_vcs = params->no_vcs;
    i_p_rt.credits = params->credits;
    i_p_rt.rc_method = RING_ROUTING;
    
    NIInit<T> niInit(mapping, slen, vn);


    //creat interfaces and routers
    for ( uint i=0; i< no_nodes; i++)
    {
        //interface_ids.push_back( manifold::kernel::Component::Create< GenNetworkInterface<T> >(lp_inf, i, mapping, slen, vn, &i_p_inf) );
        interface_ids.push_back( manifold::kernel::Component::Create< GenNetworkInterface<T> >(lp_inf, i, niInit, &i_p_inf) );
#ifdef IRIS_TEST
        outFile_data<< "0.0 N " << i << " " << i << " " << 0 << std::endl;
        outFile_signal<< "0.0 N " << i << " " << i << " " << 0 << std::endl;
#endif
        router_ids.push_back( manifold::kernel::Component::Create<SimpleRouter>(lp_rt, i, &i_p_rt) ); //lp pass from main program
#ifdef IRIS_TEST
        if ((i != 0) && (i != no_nodes - 1))
        {
           outFile_data<< "0.0 N " << i  + no_nodes << " " << i << " " << 1 << std::endl;
           outFile_signal<< "0.0 N " << i  + no_nodes << " " << i << " " << 1 << std::endl;
        }
        else
        {
           outFile_data<< "0.0 N " << i  + no_nodes << " " << i << " " << 2 << std::endl;
           outFile_signal<< "0.0 N " << i  + no_nodes << " " << i << " " << 2 << std::endl;
        }
#endif        
    }
    
    //register interfaces to clock
    for ( uint i=0; i< interface_ids.size(); i++)
    {
        GenNetworkInterface<T>* interface = manifold::kernel::Component::GetComponent< GenNetworkInterface<T> >(interface_ids.at(i));
        if ( interface != NULL )
        {
            manifold::kernel::Clock::Register< GenNetworkInterface<T> >
            (clk_r, interface, &GenNetworkInterface<T>::tick, &GenNetworkInterface<T>::tock); //pass clock from out side
        }
	interfaces.push_back(interface);

    }

    //register routers to clock
    for ( uint i=0; i< router_ids.size(); i++)
    {  
        SimpleRouter* rr= manifold::kernel::Component::GetComponent<SimpleRouter>(router_ids.at(i));
        if ( rr != NULL )
        {  
            manifold::kernel::Clock::Register<SimpleRouter>(clk_r, rr, &SimpleRouter::tick, &SimpleRouter::tock);
        }
	routers.push_back(rr);
    }
     

    for ( uint i=0; i< interface_ids.size(); i++)
    {
        GenNetworkInterface<T>* interface = manifold::kernel::Component::GetComponent< GenNetworkInterface<T> >(interface_ids.at(i));
        if ( interface != NULL ) {
	    SimpleRouter* rr= manifold::kernel::Component::GetComponent<SimpleRouter>(router_ids.at(i));
	    assert(rr);
	    interface->set_router(rr);
	}
    }

    //establish the connections
    connect_interface_routers();
    connect_routers();    
}

//deconstructor
template <typename T>
Ring<T>::~Ring()
{
    for ( uint i=0 ; i<no_nodes; i++ )
    {
        delete interfaces[i];
        delete routers[i];
    }

}

//! connect interface to router
template <typename T>
void
Ring<T>::connect_interface_routers()
{
    const manifold::kernel::Ticks_t LATENCY = 1;
    
    //Connect for the output links of the router 
    for( uint i=0; i<no_nodes; i++)
    {
        //  Interface to router 
        manifold::kernel::Manifold::Connect(interface_ids.at(i), GenNetworkInterface<T>::ROUTER_PORT, 
                                            router_ids.at(i), SimpleRouter::PORT_NI,
                                            &SimpleRouter::handle_link_arrival, LATENCY);
                                      
        //  Router to Interface
        manifold::kernel::Manifold::Connect(router_ids.at(i), SimpleRouter::PORT_NI, 
                                            interface_ids.at(i), GenNetworkInterface<T>::ROUTER_PORT,
                                            &GenNetworkInterface<T>::handle_router, LATENCY);
#ifdef IRIS_TEST
        outFile_data<< "0.0 L " << interfaces.at(i)->id << " " << routers.at(i)->node_id + no_nodes<< std::endl;
#endif 

    }
}



//! connect routers together
template <typename T>
void
Ring<T>::connect_routers()
{
    const manifold::kernel::Ticks_t LATENCY = 1;

    // Configure east - west links for the routers.. in order first WEST then
    // EAST
    for ( uint i=1; i<no_nodes; i++)
    {
        // going west  <-
        // Router->Router DATA 
        manifold::kernel::Manifold::Connect(router_ids.at(i), SimpleRouter::PORT_WEST, 
                                            router_ids.at(i-1), SimpleRouter::PORT_EAST,
                                            &SimpleRouter::handle_link_arrival, LATENCY);
#ifdef IRIS_TEST
        outFile_data<< "0.0 L " << routers.at(i)->node_id + no_nodes<< " " << routers.at(i - 1)->node_id + no_nodes<< std::endl;
#endif
        // going east  ->
        // Router->Router DATA 
        manifold::kernel::Manifold::Connect(router_ids.at(i-1), SimpleRouter::PORT_EAST, 
                                            router_ids.at(i), SimpleRouter::PORT_WEST,
                                            &SimpleRouter::handle_link_arrival, LATENCY);
#ifdef IRIS_TEST
        outFile_data<< "0.0 L " << routers.at(i)->node_id + no_nodes<< " " << routers.at(i - 1)->node_id + no_nodes<< std::endl;
#endif
    }

    // router 0 and end router
    // going west <-
    manifold::kernel::Manifold::Connect(router_ids.at(0), SimpleRouter::PORT_WEST, 
                                        router_ids.at(no_nodes-1), SimpleRouter::PORT_EAST,
                                        &SimpleRouter::handle_link_arrival, LATENCY);
#ifdef IRIS_TEST
        outFile_data<< "0.0 L " << routers.at(0)->node_id + no_nodes<< " " << routers.at(no_nodes-1)->node_id + no_nodes<< std::endl;
#endif

 
    // going east ->
    manifold::kernel::Manifold::Connect(router_ids.at(no_nodes-1), SimpleRouter::PORT_EAST, 
                                        router_ids.at(0), SimpleRouter::PORT_WEST,
                                        &SimpleRouter::handle_link_arrival, LATENCY);
#ifdef IRIS_TEST
        outFile_data<< "0.0 L " << routers.at(0)->node_id + no_nodes<< " " << routers.at(no_nodes-1)->node_id + no_nodes<< std::endl;
#endif
                                        

}



template <typename T>
void Ring<T> :: print_stats(std::ostream& out)
{
    for(unsigned i=0; i<interfaces.size(); i++) {
	if(interfaces[i])
	    interfaces[i]->print_stats(out);
    }

    for(unsigned i=0; i<routers.size(); i++) {
	if(routers[i])
	    routers[i]->print_stats(out);
    }
}



} //Iris
} //Manifold

#endif   /* ----- #ifndef MANIFOLD_IRIS_RING_H ----- */
