#ifndef NETWORK_BUILDER_H
#define NETWORK_BUILDER_H

#include <libconfig.h++>
#include "iris/genericTopology/genericTopoCreator.h"
#include "iris/interfaces/genericIrisInterface.h"
#include "uarch/networkPacket.h"
//#include "cache_builder.h"



class CacheBuilder;

class NetworkBuilder {
public:
    enum {IRIS}; //types of networks
    NetworkBuilder() {}
    virtual ~NetworkBuilder() {}

    virtual int get_type() = 0;

    virtual void read_network_topology(libconfig::Config& config) = 0;
    virtual int get_max_nodes() = 0;
    virtual int get_x_dim() { return 0; }
    virtual int get_y_dim() { return 0; }
    //virtual void create_network(manifold::kernel::Clock&, int part, CacheBuilder* cache_builder) = 0;
    virtual void create_network(manifold::kernel::Clock&, int part) = 0;
    virtual const std::vector<manifold::kernel::CompId_t>& get_interface_cid() = 0;
    //! action to take just before simulation starts
    virtual void pre_simulation() = 0;

    virtual void print_config(std::ostream&) {}
    virtual void print_stats(std::ostream&) = 0;

protected:
};



//builder for Iris
class Iris_builder : public NetworkBuilder {
public:

    enum { PART_1, PART_2, PART_Y}; //torus partitioning

    Iris_builder();

    int get_type() { return IRIS; }

    void read_network_topology(libconfig::Config& config);

    int get_max_nodes() { return m_x_dimension * m_y_dimension; }
    int get_x_dim() { return m_x_dimension; }
    int get_y_dim() { return m_y_dimension; }

    void dep_injection(manifold::iris::SimulatedLen<manifold::uarch::NetworkPacket>* simLen,
                       manifold::iris::VnetAssign<manifold::uarch::NetworkPacket>* vnet);
    //void create_network(manifold::kernel::Clock&, int part, CacheBuilder* cache_builder);
    void create_network(manifold::kernel::Clock&, int part); //overwrite baseclass
    const std::vector<manifold::kernel::CompId_t>& get_interface_cid();

    void pre_simulation();

    void print_config(std::ostream&);
    void print_stats(std::ostream&);
private:
    std::string m_net_topo; //topology
    int m_x_dimension;
    int m_y_dimension;
    manifold::iris::ring_init_params ring_params;
    manifold::iris::torus_init_params torus_params;
    //int COH_MSG_TYPE;
    //int MEM_MSG_TYPE;
    int CREDIT_MSG_TYPE;
    manifold::iris::Ring<manifold::uarch::NetworkPacket>* m_ring;
    manifold::iris::Torus<manifold::uarch::NetworkPacket>* m_torus;
    manifold::iris::SimulatedLen<manifold::uarch::NetworkPacket>* m_simLen;
    manifold::iris::VnetAssign<manifold::uarch::NetworkPacket>* m_vnet;
    bool m_default_simLen;
    bool m_default_vnet;

};






#endif // #ifndef NETWORK_BUILDER_H
