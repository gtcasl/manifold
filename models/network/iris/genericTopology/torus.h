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

#ifndef  MANIFOLD_IRIS_TORUS_H
#define  MANIFOLD_IRIS_TORUS_H

#include "../interfaces/genericIrisInterface.h"
#include "../components/simpleRouter.h"

namespace manifold {
namespace iris {

struct torus_init_params {
    torus_init_params() : x_dim(0), y_dim(0), no_vcs(0), credits(0), link_width(0), ni_up_credits(0), ni_upstream_buffer_size(0) {}

    uint x_dim;
    uint y_dim;
    uint no_vcs;
    uint credits;
    uint link_width; //in bits.
    unsigned ni_up_credits; //network interface credits for output to terminal.
    int ni_upstream_buffer_size; //network interface's output buffer (to terminal) size
    //ROUTING_SCHEME rc_method;
};



template <typename T>
class Torus
{
    public:
        //constructor and deconstructor
        //Torus (manifold::kernel::Clock& clk, torus_init_params* params, const Terminal_to_net_mapping* mapping, SimulatedLen<T>*, VnetAssign<T>*, int ni_credit_type, int lp=0); //all interfaces and routers in one LP
	//! @param \c node_lp   router idx to LP mapping
        Torus (manifold::kernel::Clock& clk, torus_init_params* params, const Terminal_to_net_mapping* mapping, SimulatedLen<T>*, VnetAssign<T>*, int ni_credit_type, vector<int>* node_lp);
        ~Torus ();

        //connect all components together
        void connect_interface_routers(void);
        void connect_routers(void);
        
	const std::vector <GenNetworkInterface<T>*>& get_interfaces() { return interfaces; }
	const std::vector <SimpleRouter*>& get_routers() {return routers; }

        //the interfaces' component id
        const std::vector <manifold::kernel::CompId_t>& get_interface_id() { return interface_ids; }
       
 	//the routers' component id
	const std::vector <manifold::kernel::CompId_t>& get_router_id() { return router_ids; }
	void print_stats(std::ostream&);

#ifndef IRIS_TEST
    private:
#endif
       // variables
       const unsigned x_dim; // x dimension
       const unsigned y_dim; // y dimension
       manifold::kernel::Clock& clk_t; //the clock for routers and interfaces 
       std::vector <SimpleRouter*> routers; //the routers
       std::vector <GenNetworkInterface<T>* > interfaces; //the interfaces
       std::vector <manifold::kernel::CompId_t> router_ids; //the routers' component ID
       std::vector <manifold::kernel::CompId_t> interface_ids; //the interfaces' component ID  
       
       //the node to lp map
       vector<int>* node_lp; 
#ifdef IRIS_TEST  
       //trace file that using in debugging     
       std::ofstream outFile_data; 
       std::ofstream outFile_signal;
#endif  
     
   protected:     
}; 

//! @param \c clk  The clock passing from callor
//! @param \c params  The configure parameters for torus network
//! @param \c ni_credit_type  The message type for network interface's credits to terminal.
//! @param \c lp_inf  The logic process id for interfaces
//! @param \c lp_rt  The logic process id for routers
template <typename T>
Torus<T>::Torus(manifold::kernel::Clock& clk, torus_init_params* params, const Terminal_to_net_mapping* mapping, SimulatedLen<T>* slen, VnetAssign<T>* vn, int ni_credit_type, vector<int>* node_lp) :
    x_dim(params->x_dim),
    y_dim(params->y_dim),
    clk_t(clk),
    node_lp(node_lp)
{
#ifdef IRIS_TEST
    uint grid_count = 0;
    
    //open trace file
    outFile_data.open("torus_data.tr");

    //handle exception
    if(outFile_data.fail())
       std::cout<<"fail to open file"<<std::endl;
       
    //open trace file
    outFile_signal.open("torus_signal.tr");

    //handle exception
    if(outFile_signal.fail())
       std::cout<<"fail to open file"<<std::endl;   
#endif

    assert(params->ni_up_credits > 0);
    assert(ni_credit_type != 0);
    assert(params->ni_upstream_buffer_size > 0);
    
    //parameters for interface
    inf_init_params i_p_inf;
    
    i_p_inf.linkWidth = params->link_width;
    i_p_inf.num_credits = params->credits;
    i_p_inf.upstream_credits = params->ni_up_credits;
    i_p_inf.up_credit_msg_type = ni_credit_type;
    i_p_inf.num_vc = params->no_vcs; 
    i_p_inf.upstream_buffer_size = params->ni_upstream_buffer_size;
    

    //parameters for router
    router_init_params i_p_rt;

    i_p_rt.no_nodes = params->x_dim * params->y_dim;
    i_p_rt.grid_size = params->x_dim;
    i_p_rt.no_ports = 5;
    i_p_rt.no_vcs = params->no_vcs;
    i_p_rt.credits = params->credits;
    i_p_rt.rc_method = TORUS_ROUTING; 
    
    NIInit<T> niInit(mapping, slen, vn);

    const unsigned no_nodes = x_dim * y_dim;
    //creat interfaces and routers
    //check if the size of node_lp is equal to no_nodes
    if (node_lp->size() != no_nodes)
    {
        cout<<"Bad node to lp mapping!!"<<endl;
	exit(1);
    }

    for ( uint i=0; i< node_lp->size(); i++)
    {
        interface_ids.push_back( manifold::kernel::Component::Create< GenNetworkInterface<T> >(node_lp->at(i), i, niInit, &i_p_inf) );
#ifdef IRIS_TEST
        outFile_data<< "0.0 N " << i << " " << i << " " << 0 << std::endl;
        outFile_signal<< "0.0 N " << i << " " << i << " " << 0 << std::endl;
#endif        
        router_ids.push_back( manifold::kernel::Component::Create<SimpleRouter>(node_lp->at(i), i, &i_p_rt) ); 
// 	cout<<"node id: "<< node_lp->at(i).node_id <<" node lp: "<<node_lp->at(i).lp<<endl;
#ifdef IRIS_TEST
        if ((i%x_dim != 0) && (i%x_dim != (x_dim - 1)))
        {
           if (grid_count%2 == 0)
           {
              outFile_data<< "0.0 N " << i  + no_nodes << " " << i%x_dim + 6<< " " << 1 + grid_count*2<< std::endl;
              outFile_signal<< "0.0 N " << i  + no_nodes << " " << i%x_dim + 6<< " " << 1 + grid_count*2<< std::endl;
           }
           if (grid_count%2 == 1)
           {
              outFile_data<< "0.0 N " << i  + no_nodes << " " << double(i%x_dim) + 6 + 0.5<< " " << 1 + grid_count*2<< std::endl;
              outFile_signal<< "0.0 N " << i  + no_nodes << " " << double(i%x_dim) + 6 + 0.5<< " " << 1 + grid_count*2<< std::endl;
           }           
        }
        else
        {
           if (grid_count%2 == 0)
           {
               outFile_data<< "0.0 N " << i  + no_nodes << " " << i%x_dim + 6<< " " << 2 + grid_count*2 << std::endl;
               outFile_signal<< "0.0 N " << i  + no_nodes << " " << i%x_dim + 6<< " " << 2 + grid_count*2 << std::endl;
           }  
           if (grid_count%2 == 1)
           {
               outFile_data<< "0.0 N " << i  + no_nodes << " " << double(i%x_dim) + 6 + 0.5<< " " << 2 + grid_count*2 << std::endl;
               outFile_signal<< "0.0 N " << i  + no_nodes << " " << double(i%x_dim) + 6 + 0.5<< " " << 2 + grid_count*2 << std::endl;
           }  
        }
        
        if (i%x_dim == (x_dim - 1))
           grid_count++;
#endif        
    }
    
    //register interfaces to clock
    for ( uint i=0; i< interface_ids.size(); i++)
    {
        GenNetworkInterface<T>* interface = manifold::kernel::Component::GetComponent< GenNetworkInterface<T> >(interface_ids.at(i));
        if ( interface != NULL )
        {
            manifold::kernel::Clock::Register< GenNetworkInterface<T> >
            (clk_t, interface, &GenNetworkInterface<T>::tick, &GenNetworkInterface<T>::tock); //pass clock from out side
        }
	interfaces.push_back(interface);
    }
    
    //register router to clock
    for ( uint i=0; i< router_ids.size(); i++)
    {
        SimpleRouter* rr= manifold::kernel::Component::GetComponent<SimpleRouter>(router_ids.at(i));
        if ( rr != NULL )
        {
            manifold::kernel::Clock::Register<SimpleRouter>(clk_t, rr, &SimpleRouter::tick, &SimpleRouter::tock);
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
Torus<T>::~Torus()
{
    for ( uint i=0 ; i<x_dim * y_dim; i++ )
    {
        delete interfaces[i];
        delete routers[i];
    }

}

//! connect interface to router
template <typename T>
void
Torus<T>::connect_interface_routers()
{
    int LATENCY = 1;
    //  Connect for the output links of the router 
    for( uint i=0; i<x_dim*y_dim; i++)
    {
	manifold::kernel::Manifold::Connect(interface_ids.at(i), GenNetworkInterface<T>::ROUTER_PORT, 
					    router_ids.at(i), SimpleRouter::PORT_NI,
					    &SimpleRouter::handle_link_arrival , static_cast<manifold::kernel::Ticks_t>(LATENCY));
	manifold::kernel::Manifold::Connect(router_ids.at(i), SimpleRouter::PORT_NI, 
					    interface_ids.at(i), GenNetworkInterface<T>::ROUTER_PORT,
					    &GenNetworkInterface<T>::handle_router , static_cast<manifold::kernel::Ticks_t>(LATENCY));
	#ifdef IRIS_TEST
            if(interfaces.at(i))
		outFile_data<< "0.0 L " << interfaces.at(i)->id << " " << routers.at(i)->node_id + x_dim*y_dim<< std::endl;
// 		cout<< "0.0 L " << interfaces.at(i)->id << " " << routers.at(i)->node_id + x_dim*y_dim<< std::endl;
	#endif
    }
}


//! connect routers together
template <typename T>
void
Torus<T>::connect_routers()
{
    const manifold::kernel::Ticks_t LATENCY = 1;
    //const unsigned no_nodes = x_dim * y_dim;

    // Configure east - west links for the routers.. in order first WEST then
    for ( uint i=0; i<y_dim; i++) { //for all rows
        for ( uint j=1; j<x_dim; j++)  //for all columns except the left-most
        {
	    uint rno = router_ids.at(i*x_dim + j);
	    uint rno2 = router_ids.at(i*x_dim + j-1);

	    //check if this connection go across lp
	    if (node_lp->at(i*x_dim + j) != node_lp->at(i*x_dim + j-1)){
		if (routers[i*x_dim + j])
		    routers[i*x_dim + j]->set_port_cross_lp(SimpleRouter::PORT_WEST);
		if (routers[i*x_dim + j-1])
		    routers[i*x_dim + j-1]->set_port_cross_lp(SimpleRouter::PORT_EAST);
		//cout<<"router: "<<i*x_dim + j<<" at lp: "<<node_lp->at(i*x_dim + j)<<" WEST to router: "<<i*x_dim + j-1<<" at lp: "<<node_lp->at(i*x_dim + j-1)<<" EAST "<<endl;
	    }  

	    // going west  <-
	    manifold::kernel::Manifold::Connect(rno, SimpleRouter::PORT_WEST, 
						rno2, SimpleRouter::PORT_EAST,
						&SimpleRouter::handle_link_arrival, LATENCY);
	#ifdef IRIS_TEST
           #if 0
		const unsigned no_nodes = x_dim * y_dim;
		outFile_data<< "0.0 L " << routers.at(i*x_dim + j)->node_id + no_nodes << " " 
		<< routers.at(i*x_dim + j-1)->node_id + no_nodes<< std::endl;
	   #endif  
	#endif
		    // going east  ->
		    // Router->Router DATA 
		    manifold::kernel::Manifold::Connect(rno2, SimpleRouter::PORT_EAST, 
		                                        rno, SimpleRouter::PORT_WEST,
		                                        &SimpleRouter::handle_link_arrival, LATENCY);
	#ifdef IRIS_TEST
           #if 0
		outFile_data<< "0.0 L " << routers.at(i*x_dim + j)->node_id + no_nodes<< " " 
		<< routers.at(i*x_dim + j-1)->node_id + no_nodes<< std::endl;
	   #endif  
	#endif
        }
    }

    //connect north south
    for ( uint i=1; i<y_dim; i++) { //starting from 2nd row
        for ( uint j=0; j<x_dim; j++) //for all columns
        {
	    uint rno = router_ids.at(i*x_dim + j);
	    uint up_rno =router_ids.at((i-1)*x_dim + j);

	    // going north ^

	    //check if this connection go across lp
	    if (node_lp->at(i*x_dim + j) != node_lp->at((i-1)*x_dim + j)){
		if (routers[i*x_dim + j])
		    routers[i*x_dim + j]->set_port_cross_lp(SimpleRouter::PORT_NORTH);
		if (routers[(i-1)*x_dim + j])
		    routers[(i-1)*x_dim + j]->set_port_cross_lp(SimpleRouter::PORT_SOUTH);
		//cout<<"router: "<<i*x_dim + j<<" at lp: "<<node_lp->at(i*x_dim + j)<<" NORTH to router: "<<(i-1)*x_dim + j<<" at lp: "<<node_lp->at((i-1)*x_dim + j)<<" SOUTH "<<endl;
	    }  

	    manifold::kernel::Manifold::Connect(rno, SimpleRouter::PORT_NORTH, 
						up_rno, SimpleRouter::PORT_SOUTH,
						&SimpleRouter::handle_link_arrival, LATENCY);
	#ifdef IRIS_TEST
           #if 0
		outFile_data<< "0.0 L " << routers.at(i*x_dim + j)->node_id + no_nodes<< " " 
		<< routers.at(i*x_dim + j- x_dim)->node_id + no_nodes<< std::endl;
	   #endif  
	#endif
		    // going south->
		    // Router->Router DATA 
		    manifold::kernel::Manifold::Connect(up_rno, SimpleRouter::PORT_SOUTH, 
		                                        rno, SimpleRouter::PORT_NORTH,
		                                        &SimpleRouter::handle_link_arrival, LATENCY);
	#ifdef IRIS_TEST
           #if 0
		outFile_data<< "0.0 L " << routers.at(i*x_dim + j)->node_id + no_nodes<< " " 
		<< routers.at(i*x_dim + j- x_dim)->node_id + no_nodes<< std::endl;
	   #endif  
	#endif
        }
    }


    // connect edge nodes East-west
    for ( uint i=0; i<y_dim; i++)
    {
	uint rno = router_ids.at(i*x_dim);
	uint end_rno = router_ids.at(i*x_dim+x_dim-1);

	// router 0 and end router of every row
	// going west <-
	//check if this connection go across lp
	if (node_lp->at(i*x_dim) != node_lp->at(i*x_dim+x_dim-1)){
	    if (routers[i*x_dim])
		routers[i*x_dim]->set_port_cross_lp(SimpleRouter::PORT_WEST);
	    if (routers[i*x_dim+x_dim-1])
		routers[i*x_dim+x_dim-1]->set_port_cross_lp(SimpleRouter::PORT_EAST);
	    //cout<<"router: "<<i*x_dim<<" at lp: "<<node_lp->at(i*x_dim)<<" WEST to router: "<<i*x_dim+x_dim-1<<" at lp: "<<node_lp->at(i*x_dim+x_dim-1)<<" EAST "<<endl;
	}

	manifold::kernel::Manifold::Connect(rno, SimpleRouter::PORT_WEST, 
					    (end_rno), SimpleRouter::PORT_EAST,
					    &SimpleRouter::handle_link_arrival, LATENCY);
	#ifdef IRIS_TEST
           #if 0
		outFile_data<< "0.0 L " << routers.at(i*x_dim)->node_id + no_nodes<< " " 
		<< routers.at(i*x_dim + x_dim-1)->node_id + no_nodes<< std::endl;
	   #endif  
	#endif

		// going east ->
		manifold::kernel::Manifold::Connect(end_rno, SimpleRouter::PORT_EAST, 
		                                    rno, SimpleRouter::PORT_WEST,
		                                    &SimpleRouter::handle_link_arrival, LATENCY);
	#ifdef IRIS_TEST
           #if 0
		outFile_data<< "0.0 L " << routers.at(i*x_dim)->node_id + no_nodes<< " " 
		<< routers.at(i*x_dim + x_dim-1)->node_id + no_nodes<< std::endl;
	   #endif  
	#endif  
    }


    // connect edge nodes north-south
    for ( uint i=0; i<x_dim; i++) //for all columns
    {
	uint rno = router_ids.at(i);
	uint end_rno = router_ids.at((y_dim-1)*x_dim+i);

	// router 0 and end router of every row
	// going west <-
	//check if this connection go across lp
	if (node_lp->at(i) != node_lp->at((y_dim-1)*x_dim+i)){
	    if (routers[i])
		routers[i]->set_port_cross_lp(SimpleRouter::PORT_NORTH);
	    if (routers[(y_dim-1)*x_dim+i])
		routers[(y_dim-1)*x_dim+i]->set_port_cross_lp(SimpleRouter::PORT_SOUTH);
	    //cout<<"router: "<<i<<" at lp: "<<node_lp->at(i)<<" NORTH to router: "<<(y_dim-1)*x_dim+i<<" at lp: "<<node_lp->at((y_dim-1)*x_dim+i)<<" SOUTH "<<endl;
	}

	manifold::kernel::Manifold::Connect((rno), SimpleRouter::PORT_NORTH, 
					    (end_rno), SimpleRouter::PORT_SOUTH,
					    &SimpleRouter::handle_link_arrival, LATENCY);
	#ifdef IRIS_TEST
           #if 0
		outFile_data<< "0.0 L " << routers.at(i)->node_id + no_nodes<< " " 
		<< routers.at((no_nodes/x_dim-1)*x_dim+i)->node_id + no_nodes<< std::endl;
           #endif
	#endif

		// going east ->
		manifold::kernel::Manifold::Connect(end_rno, SimpleRouter::PORT_SOUTH, 
		                                    rno, SimpleRouter::PORT_NORTH,
		                                    &SimpleRouter::handle_link_arrival, LATENCY);
	#ifdef IRIS_TEST
           #if 0
		outFile_data<< "0.0 L " << routers.at(i)->node_id + no_nodes<< " " 
		<< routers.at((no_nodes/x_dim-1)*x_dim+i)->node_id + no_nodes<< std::endl;
           #endif
	#endif
    }

}

template <typename T>
void Torus<T> :: print_stats(std::ostream& out)
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

#endif  
/* ----- #ifndef MANIFOLD_IRIS_TORUS_H  ----- */
