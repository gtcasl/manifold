#include "network_builder.h"
#include "cache_builder.h"
//#include "mcp_cache-iris/mcp-iris.h"

using namespace libconfig;
using namespace manifold::iris;
using namespace manifold::uarch;
using namespace manifold::kernel;

Iris_builder :: Iris_builder()
{
    m_simLen = 0;
    m_vnet = 0;
    m_default_simLen = false;
    m_default_vnet = false;
}


void Iris_builder :: read_network_topology(libconfig::Config& config)
{

    try {
	const char* topo1 = config.lookup("network.topology");
	m_net_topo = topo1;
	if(m_net_topo != "RING" && m_net_topo != "TORUS") {
	    cerr << "Unknown network topology: " << m_net_topo << ". Iris only supports RING and TORUS.\n";
	    exit(1);
	}

	m_x_dimension = config.lookup("network.x_dimension");
	m_y_dimension = config.lookup("network.y_dimension");
	assert(m_x_dimension > 0 && m_y_dimension > 0);
	assert(m_x_dimension * m_y_dimension > 1);

	if(this->m_net_topo == "RING")
	    assert(m_y_dimension == 1);

	if(m_net_topo == "RING") {
	    ring_params.no_nodes = m_x_dimension * m_y_dimension;
	    ring_params.no_vcs = config.lookup("network.num_vcs");
	    ring_params.credits = config.lookup("network.credits");
	    ring_params.link_width = config.lookup("network.link_width");
	    ring_params.rc_method = RING_ROUTING;
	    ring_params.ni_up_credits = config.lookup("network.ni_up_credits");
	    ring_params.ni_upstream_buffer_size = config.lookup("network.ni_up_buffer");
	}
	else if(this->m_net_topo == "TORUS") {
	    torus_params.x_dim = m_x_dimension;
	    torus_params.y_dim = m_y_dimension;
	    torus_params.no_vcs = config.lookup("network.num_vcs");
	    torus_params.credits = config.lookup("network.credits");
	    torus_params.link_width = config.lookup("network.link_width");
	    torus_params.ni_up_credits = config.lookup("network.ni_up_credits");
	    torus_params.ni_upstream_buffer_size = config.lookup("network.ni_up_buffer");
	}
	//COH_MSG_TYPE = config.lookup("network.coh_msg_type");
	//MEM_MSG_TYPE = config.lookup("network.mem_msg_type");
	CREDIT_MSG_TYPE = config.lookup("network.credit_msg_type");
    }
    catch (SettingNotFoundException e) {
	cerr << e.getPath() << " not set." << endl;
	exit(1);
    }
    catch (SettingTypeException e) {
	cerr << e.getPath() << " has incorrect type." << endl;
	exit(1);
    }

}


void Iris_builder :: dep_injection(SimulatedLen<NetworkPacket>* simLen, VnetAssign<NetworkPacket>* vnet)
{
    m_simLen = simLen;
    m_vnet = vnet;
}

void Iris_builder :: create_network(manifold::kernel::Clock& clock, int part) // CacheBuilder* cache_builder)
{
    //Terminal address to network address mapping
    //Using Simple_terminal_to_net_mapping means node ids must be 0 to MAX_NODES-1.
    Simple_terminal_to_net_mapping* mapping = new Simple_terminal_to_net_mapping();


    //ToDo: mapping, simLen, vnet are not deleted
    assert(m_simLen != 0 && m_vnet != 0);
    if(m_simLen == 0) {
        m_default_simLen = true;
	m_simLen = new SimulatedLen<NetworkPacket>();
    }
    if(m_vnet == 0) {
        m_default_vnet = true;
	m_vnet = new VnetAssign<NetworkPacket>();
    }

    m_ring = 0;
    m_torus = 0;

    if(this->m_net_topo == "RING") {
	m_ring = topoCreator<NetworkPacket>::create_ring(clock, &(this->ring_params), mapping, (SimulatedLen<NetworkPacket>*)m_simLen, (VnetAssign<NetworkPacket>*)m_vnet, this->CREDIT_MSG_TYPE, 0, 0); //network on LP 0
    }
    else {
	vector<int> node_lp;
	node_lp.resize(m_x_dimension*m_y_dimension);
	switch(part) {
	    case PART_1:
		for (int i = 0; i < m_x_dimension*m_y_dimension; i++) {
		    node_lp[i] = 0;
		}
		break;
	    case PART_2:
		for (int i = 0; i < m_x_dimension*m_y_dimension; i++) {
		    if(i < m_x_dimension*m_y_dimension/2)
			node_lp[i] = 0;
		    else
			node_lp[i] = 1;
		}
		break;
	    case PART_Y:
		for (int i = 0; i < m_x_dimension*m_y_dimension; i++) {
		    node_lp[i] = i / m_x_dimension;
		}
		break;
	    default:
		assert(0);
	}//switch
	m_torus = topoCreator<NetworkPacket>::create_torus(clock, &(this->torus_params), mapping, (SimulatedLen<NetworkPacket>*)m_simLen, (VnetAssign<NetworkPacket>*)m_vnet, this->CREDIT_MSG_TYPE, &node_lp); //network on LP 0
    }

}



const std::vector<manifold::kernel::CompId_t>& Iris_builder :: get_interface_cid()
{
    return (m_ring != 0) ? m_ring->get_interface_id() : m_torus->get_interface_id();
}


void Iris_builder :: pre_simulation()
{

#ifdef FORECAST_NULL
    const std::vector<GenNetworkInterface<NetworkPacket>*>& nis = (m_ring != 0) ? m_ring->get_interfaces() : m_torus->get_interfaces();
    const std::vector<SimpleRouter*>& routers = (m_ring != 0) ? m_ring->get_routers() : m_torus->get_routers();

    assert(nis.size() == routers.size());

    for(unsigned i=0; i<nis.size(); i++) {
	if(nis[i] != 0) {
	    //?????????????????????????????? todo: use proper clock!
	    Clock::Master().register_output_predictor(nis[i], &GenNetworkInterface<NetworkPacket>::do_output_to_terminal_prediction);
	}

	if(routers[i] != 0 && routers[i]->get_cross_lp_flag()) {
	    //?????????????????????????????? todo: use proper clock!
	    Clock::Master().register_output_predictor(routers[i], &SimpleRouter::do_output_to_router_prediction);
	}
    }
#endif

}


void Iris_builder :: print_config(std::ostream& out)
{
    out << "Network type: Iris\n";
    out << "  topology: " << m_net_topo << endl;
    out << "  X dim: " << m_x_dimension << endl
        << "  Y dim: " << m_y_dimension << endl;

    if(m_default_simLen)
        out << "  use default simLen object!\n";
    if(m_default_vnet)
        out << "  use default vnet object!\n";
}

void Iris_builder :: print_stats(std::ostream& out)
{
    if(m_ring)
	m_ring->print_stats(out);
    else
	m_torus->print_stats(out);

}





