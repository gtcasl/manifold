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

#ifndef MANIFOLD_IRIS_GENERICTOPOCREATOR_H
#define MANIFOLD_IRIS_GENERICTOPOCREATOR_H

#include "../interfaces/genericHeader.h"
#include "ring.h"
#include "torus.h"
#include "CrossBar.h"

/* *********** the class topology creator start here ************ */
namespace manifold {
namespace iris {
  
//! Class T must contain destnation and source addresss
template <typename T> 
class topoCreator {
public:
    //constructor and deconstructor
    topoCreator() {}
    ~topoCreator ();
    
    //ring and torus creators
    static Ring<T>* create_ring(manifold::kernel::Clock& clk, ring_init_params* params, const Terminal_to_net_mapping*, SimulatedLen<T>*, VnetAssign<T>*, int ni_credit_type, uint lp_inf, uint lp_rt);
    static Torus<T>* create_torus(manifold::kernel::Clock& clk, torus_init_params* params, const Terminal_to_net_mapping*, SimulatedLen<T>*, VnetAssign<T>*, int ni_credit_type, vector<int>* node_lp); 
    static CrossBar<T>* create_CrossBar(manifold::kernel::Clock& clk, const CrossBar_init_params* params, const Terminal_to_net_mapping*, SimulatedLen<T>*, uint lp_inf, uint lp_rt);    
 
#ifndef IRIS_TEST
    private:
#endif
    manifold::kernel::Clock& clk_tc;    
};
/* -----  end of topology creator   ----- */

//!the deconstrector
template<typename T>
topoCreator<T>::~topoCreator ()
{
}

//! response for creating the ring topology 
//! @param \c clk  The clock passing from callor
//! @param \c params  The configure parameters for ring network
//! @param \c ni_credit_type  The message type for network interface's credits to terminal.
//! @param \c lp_inf  The logic process id for interfaces
//! @param \c lp_rt  The logic process id for routers
template<typename T>
Ring<T>* topoCreator<T>::create_ring(manifold::kernel::Clock& clk, ring_init_params* params, const Terminal_to_net_mapping* mapping, SimulatedLen<T>* simLen, VnetAssign<T>* vn,  int ni_credit_type, uint lp_inf, uint lp_rt)  
{
    Ring<T>* tp = 0;
    
    //check if the configure parameter has correct type
    if (params->rc_method == RING_ROUTING)
       //call ring constructor to creat the ting network
       tp = new manifold::iris::Ring<T>(clk, params, mapping, simLen, vn, ni_credit_type, lp_inf, lp_rt);
    else
       std::cout<<" Wrong routing mothed is assigned! "<<std::endl;
    
    return tp;  
}

//! response for creating the torus topology 
//! @param \c clk  The clock passing from callor
//! @param \c params  The configure parameters for torus network
//! @param \c ni_credit_type  The message type for network interface's credits to terminal.
//! @param \c node_lp  LP assignment of each interface-router pair.
template<typename T>
Torus<T>* topoCreator<T>::create_torus(manifold::kernel::Clock& clk, torus_init_params* params, const Terminal_to_net_mapping* mapping, SimulatedLen<T>* simLen, VnetAssign<T>* vn, int ni_credit_type, vector<int>* node_lp)
{
    Torus<T>* tp = 0;
    
    //call torus constructor to creat the ting network
    tp = new manifold::iris::Torus<T>(clk, params, mapping, simLen, vn, ni_credit_type, node_lp);
    
    return tp;  
}

//! response for creating the Cross-Bar topology 
//! @param \c clk  The clock passing from callor
//! @param \c params  The configure parameters for Cross-Bar network
//! @param \c lp_inf  The logic process id for interfaces
//! @param \c lp_rt  The logic process id for routers
template<typename T>
CrossBar<T>* topoCreator<T>::create_CrossBar(manifold::kernel::Clock& clk, const CrossBar_init_params* params, const Terminal_to_net_mapping* mapping, SimulatedLen<T>* simLen, uint lp_inf, uint lp_rt)
{
    CrossBar<T>* tp = 0;
    tp = new manifold::iris::CrossBar<T>(clk, params, mapping, lp_inf, lp_rt);
    return tp;
}

} //iris
} //maniflod

#endif /* MANIFOLD_IRIS_GENERICTOPOCREATOR_H */
