#ifndef HMC_BUILDER_H
#define HMC_BUILDER_H

#include <map>
#include <libconfig.h++>
#include "mc_builder.h"
#include "DRAMSim2/dram_sim.h"
#include "iris/genericTopology/genericTopoCreator.h"
#include "iris/interfaces/genericIrisInterface.h"
#include "uarch/networkPacket.h"
#include "uarch/DestMap.h"



//class SysBuilder_llp;
//class NetworkBuilder;
//class MemControllerBuilder;

//builder for hmc
class hmc_builder : public MemControllerBuilder {
public:
    hmc_builder(SysBuilder_llp* b) : MemControllerBuilder(b) {
        m_simLen = 0;
        m_vnet = 0;
        m_default_simLen = false;
        m_default_vnet = false;
    }

    int get_type() { return HMC; }

    void read_config(libconfig::Config&);
    void create_mcs(std::map<int, int>& id_lp);
    void connect_mc_network(NetworkBuilder*);

    void print_config(std::ostream&);
    void print_stats(std::ostream&);

    void dep_injection(manifold::iris::SimulatedLen<manifold::uarch::NetworkPacket>* simLen,
                       manifold::iris::VnetAssign<manifold::uarch::NetworkPacket>* vnet);

private:
    int m_MC_DOWNSTREAM_CREDITS; //credits for sending down to network
    int m_MEM_MSG_TYPE;
    int m_CREDIT_MSG_TYPE;
    std::string m_DEV_FILE; //device file name
    std::string m_SYS_FILE; //system file name
    unsigned m_MEM_SIZE; //mem size;
    
    std::string m_net_topo; //topology
    int m_x_dimension;
    int m_y_dimension;
    manifold::iris::torus6p_init_params torus6p_params;
    manifold::iris::hmcNet<manifold::uarch::NetworkPacket>* m_torus;
    manifold::iris::SimulatedLen<manifold::uarch::NetworkPacket>* m_simLen;
    manifold::iris::VnetAssign<manifold::uarch::NetworkPacket>* m_vnet;
    bool m_default_simLen;
    bool m_default_vnet;
};






#endif // #ifndef HMC_BUILDER_H
