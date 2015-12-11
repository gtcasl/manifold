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

#ifndef  MANIFOLD_IRIS_CROSSBAR_H
#define  MANIFOLD_IRIS_CROSSBAR_H

//#include "../interfaces/genericHeader.h"
#include "../components/genOneVcIrisInterface.h"
#include "../components/CrossBarSwitch.h"


namespace manifold {
namespace iris {

struct CrossBar_init_params {
    uint no_nodes;
    uint credits;
    uint link_width;
};

template <typename T>
class CrossBar 
{
    public:
        //constructor and deconstructor
        CrossBar (manifold::kernel::Clock& clk, const CrossBar_init_params* params, const Terminal_to_net_mapping*, uint lp_inf, uint lp_rt);
        ~CrossBar ();

        //connect all components together
        void connect_interface_CrossBarSwtich(void);
        
        //the interfaces
        const std::vector <GenOneVcIrisInterface<T>*>& get_interfaces() { return interfaces; }

        //the interfaces' component id
        const std::vector <manifold::kernel::CompId_t>& get_interface_id() { return interface_ids; }
        

#ifndef IRIS_TEST
    private:
#endif
       // variables
       const unsigned no_nodes; //# of interfaces
       manifold::kernel::Clock& clk_c; //the clock for routers and interfaces 
       CrossBarSwitch* cb_switch; //the cross Bar switch
       std::vector <GenOneVcIrisInterface<T>*> interfaces; //the interfaces
       manifold::kernel::CompId_t cb_switch_id; //the switch's component ID
       std::vector <manifold::kernel::CompId_t> interface_ids; //the interfaces' component ID 
       
   protected:     
}; 

//! response for creating the CrossBar topology 
//! @param \c clk  The clock passing from callor
//! @param \c params  The configure parameters for CrossBar network
//! @param \c lp_inf  The logic process id for interfaces
//! @param \c lp_rt  The logic process id for routers
template <typename T>
CrossBar<T>::CrossBar(manifold::kernel::Clock& clk, const CrossBar_init_params* params, const Terminal_to_net_mapping* mapping, uint lp_inf, uint lp_rt) :
    no_nodes(params->no_nodes),
    clk_c(clk)
{
    //parameters for interface
    simple_inf_init_params i_p_inf;
    
    //parameters for interface
    i_p_inf.linkWidth = params->link_width;
    i_p_inf.num_credits = params->credits;
    

    //parameters for router
    CB_swith_init_params i_p_switch;
            
    //parameters for router
    i_p_switch.no_nodes = params->no_nodes;
    i_p_switch.credits = params->credits;
    
    //creat interfaces and switch
    cb_switch_id = manifold::kernel::Component::Create< CrossBarSwitch >(lp_rt, &i_p_switch);
    for ( uint i=0; i< no_nodes; i++)
        interface_ids.push_back( manifold::kernel::Component::Create< GenOneVcIrisInterface<T> >(lp_inf, i, mapping, &i_p_inf) );
        
    //register interfaces to clock
    for ( uint i=0; i< interface_ids.size(); i++)
    {
        GenOneVcIrisInterface<T>* interface = manifold::kernel::Component::GetComponent< GenOneVcIrisInterface<T> >(interface_ids.at(i));
        if ( interface != NULL )
        {
            manifold::kernel::Clock::Register< GenOneVcIrisInterface<T> >
            (clk_c, interface, &GenOneVcIrisInterface<T>::tick, &GenOneVcIrisInterface<T>::tock); //pass clock from out side
        }
	interfaces.push_back(interface);

    }

    //register switch to clock
    CrossBarSwitch* cbs = manifold::kernel::Component::GetComponent<CrossBarSwitch>(cb_switch_id);
    if ( cbs != NULL )
        manifold::kernel::Clock::Register<CrossBarSwitch>(clk_c, cbs, &CrossBarSwitch::tick, &CrossBarSwitch::tock);
    cb_switch = cbs;
     

    //establish the connections
    connect_interface_CrossBarSwtich();
}

//! deconstructor
template <typename T>
CrossBar<T>::~CrossBar()
{
    for ( uint i=0 ; i<no_nodes; i++ )
        delete interfaces[i];
}

//! connect interface to router
template <typename T>
void
CrossBar<T>::connect_interface_CrossBarSwtich()
{
    const manifold::kernel::Ticks_t LATENCY = 1;
    
    //Connect for the output links of the router 
    for( uint i=0; i<no_nodes; i++)
    {
        //  Interface to router 
        manifold::kernel::Manifold::Connect(interface_ids.at(i), GenOneVcIrisInterface<T>::DATAOUT, 
                                            cb_switch_id, i,
                                            &CrossBarSwitch::handle_inf_arrival, LATENCY);
                                      
        //  Router to Interface
        manifold::kernel::Manifold::Connect(cb_switch_id, i, 
                                            interface_ids.at(i), GenOneVcIrisInterface<T>::DATAIN,
                                            &GenOneVcIrisInterface<T>::handle_router, LATENCY);
    }
}

} //Iris
} //Manifold

#endif   /* ----- #ifndef MANIFOLD_IRIS_CrossBar_H ----- */
