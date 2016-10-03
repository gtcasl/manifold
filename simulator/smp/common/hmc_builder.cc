#include "network_builder.h"
#include "sysBuilder_llp.h"
//#include "cache_builder.h"
#include "hmc_builder.h"
#include "mcp_cache-iris/mcp-iris.h"

using namespace libconfig;
using namespace manifold::iris;
using namespace manifold::uarch;
using namespace manifold::kernel;
using namespace manifold::caffdram;
using namespace manifold::dramsim;


void hmc_builder :: read_config(libconfig::Config& config)
{

    try {
	const char* topo1 = config.lookup("mc.hmc.topology");
	m_net_topo = topo1;
	if(m_net_topo != "TORUS6P") {
	    cerr << "Unknown network topology: " << m_net_topo << ". HMC only supports TORUS6P.\n";
	    exit(1);
 	}

	m_x_dimension = config.lookup("mc.hmc.x_dimension");
	m_y_dimension = config.lookup("mc.hmc.y_dimension");
	assert(m_x_dimension > 0 && m_y_dimension > 0);
	assert(m_x_dimension * m_y_dimension > 1);
	m_NUM_MC = m_x_dimension * m_y_dimension;

	torus6p_params.x_dim = m_x_dimension;
	torus6p_params.y_dim = m_y_dimension;
	torus6p_params.no_vcs = config.lookup("mc.hmc.num_vcs");
	torus6p_params.credits = config.lookup("mc.hmc.credits");
	torus6p_params.link_width = config.lookup("mc.hmc.link_width");
	torus6p_params.ni_up_credits = config.lookup("mc.hmc.ni_up_credits");
	torus6p_params.ni_upstream_buffer_size = config.lookup("mc.hmc.ni_up_buffer");

	try {
	    Setting& mc_clocks = config.lookup("mc.hmc.clocks");
	    assert((unsigned)mc_clocks.getLength() == m_NUM_MC);
	    m_CLOCK_FREQ.resize(m_NUM_MC);

	    for(unsigned i=0; i<m_NUM_MC; i++)
		m_CLOCK_FREQ[i] = (double)mc_clocks[i];

	    m_use_default_clock = false;
	}
	catch (SettingNotFoundException e) {
	    //mc clock not defined; use default
	    m_use_default_clock = true;
	}

	m_MC_DOWNSTREAM_CREDITS = config.lookup("mc.downstream_credits");

	m_MEM_MSG_TYPE = config.lookup("network.mem_msg_type");
	m_CREDIT_MSG_TYPE = config.lookup("network.credit_msg_type");

	//libconfig++ cannot assigne to string directly
	const char* chars = config.lookup("mc.hmc.dev_file");
	m_DEV_FILE = chars;
	chars = config.lookup("mc.hmc.sys_file");
	m_SYS_FILE = chars;
	m_MEM_SIZE = config.lookup("mc.hmc.size");
    }
    catch (SettingNotFoundException e) {
	cout << e.getPath() << " not set." << endl;
	exit(1);
    }
    catch (SettingTypeException e) {
	cout << e.getPath() << " has incorrect type." << endl;
	exit(1);
    }

} 


void hmc_builder :: dep_injection(SimulatedLen<NetworkPacket>* simLen, VnetAssign<NetworkPacket>* vnet)
{ 
    m_simLen = simLen;
    m_vnet = vnet;
}

void hmc_builder :: create_mcs(map<int, int>& id_lp)
{
		
    //create clocks if necessary
    if(m_use_default_clock == false) {
        if(m_CLOCK_FREQ.size() != 1 && m_CLOCK_FREQ.size() != m_NUM_MC) {
	    cerr << "Wrong number of clocks for DRAMSim; requires 1 or " << m_NUM_MC << endl;
	}
        m_clocks.resize(m_CLOCK_FREQ.size());
        for(unsigned i=0; i<m_clocks.size(); i++) {
	    m_clocks[i] = new manifold::kernel::Clock(m_CLOCK_FREQ[i]);
	}
    }

    Dram_sim :: Set_msg_types(m_MEM_MSG_TYPE, m_CREDIT_MSG_TYPE);

    Dram_sim_settings settings(m_DEV_FILE.c_str(), m_SYS_FILE.c_str(), m_MEM_SIZE, false, m_MC_DOWNSTREAM_CREDITS);

    Clock* clock = 0;
    if(m_use_default_clock)
	clock = m_sysBuilder->get_default_clock();
    else if(m_clocks.size() == 1)
	clock = m_clocks[0];

    int i=0;
    int no_hmcs = 0;
    for(map<int,int>::iterator it = id_lp.begin(); it != id_lp.end(); ++it) {
        // TODO how many hmcs, there internal and external connections from config file

  //      int node_id = (*it).first;
//	int lp = (*it).second;
//	if(!m_use_default_clock && m_clocks.size() > 1)
//	    clock = m_clocks[i++];
//	int cid = Component :: Create<Dram_sim>(lp, node_id, settings, *clock);
//	m_mc_id_cid_map[node_id] = cid;
    }

    //////// TODO DRAMSims for 1 HMC. How many is dependent on id_lp ////
    for (int k=0; k<no_hmcs; k++){
        for (int i = 0; i < m_x_dimension*m_y_dimension; i++) {
            int node_id = i;
	    if(!m_use_default_clock && m_clocks.size() > 1)
	        clock = m_clocks[i++];
	    int cid = Component :: Create<Dram_sim>(0, node_id, settings, *clock);
	    m_mc_id_cid_map[k*m_x_dimension*m_y_dimension + node_id] = cid;
        }
    }
    
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

    m_torus = 0;

    vector<int> node_lp;
    node_lp.resize(m_x_dimension*m_y_dimension);
    for (int i = 0; i < m_x_dimension*m_y_dimension; i++) {
	node_lp[i] = 0;
    }

    //    if(this->m_net_topo == "TORUS6P") {
    m_torus = topoCreator<NetworkPacket>::create_hmcnet((clock[0]), &(this->torus6p_params), mapping, (SimulatedLen<NetworkPacket>*)m_simLen, (VnetAssign<NetworkPacket>*)m_vnet, this->m_CREDIT_MSG_TYPE, &node_lp); //network on LP 0
    const std::vector<CompId_t>& ni_cids = m_torus->get_interface_id();
    
    for(map<int, int>::iterator it = m_mc_id_cid_map.begin(); it != m_mc_id_cid_map.end(); ++it) {
        int node_id = (*it).first;
	int mc_cid = (*it).second;
	Dram_sim* dram_sim = Component :: GetComponent<Dram_sim>(mc_cid);

        Manifold :: Connect(mc_cid, Controller::PORT0,
            &Dram_sim::handle_incoming<manifold::mcp_cache_namespace::Mem_msg>,
            ni_cids[node_id*2+1], GenNetworkInterface<NetworkPacket>::TERMINAL_PORT,
            &GenNetworkInterface<NetworkPacket>::handle_new_packet_event,
            *(dram_sim->get_clock()), Clock::Master(), 1, 1);
    }
}



void hmc_builder :: connect_mc_network(NetworkBuilder* net_builder)
{
/*    switch(net_builder->get_type()) {
        case NetworkBuilder::IRIS: {
		Iris_builder* irisBuilder = dynamic_cast<Iris_builder*>(net_builder);
		assert(irisBuilder != 0);

		const std::vector<CompId_t>& ni_cids = net_builder->get_interface_cid();
		for(map<int, int>::iterator it = m_mc_id_cid_map.begin(); it != m_mc_id_cid_map.end(); ++it) {
		    int node_id = (*it).first;
		    int mc_cid = (*it).second;
		    Dram_sim* dram_sim = Component :: GetComponent<Dram_sim>(mc_cid);
                    if (irisBuilder->get_topology() == "TORUS6P")
                    {
		      assert(node_id >= 0 && node_id < int(ni_cids.size())/2 );
		      if(dram_sim) { //no need to call Connect if MC is not in this LP
			//????????????????????????? todo: MCP use proper clock!!
			switch(m_sysBuilder->get_cache_builder()->get_type()) {
			    case CacheBuilder::MCP_CACHE:
			    case CacheBuilder::MCP_L1L2:
				Manifold :: Connect(mc_cid, Controller::PORT0,
				                    &Dram_sim::handle_incoming<manifold::mcp_cache_namespace::Mem_msg>,
						    ni_cids[node_id*2+1], GenNetworkInterface<NetworkPacket>::TERMINAL_PORT,
						    &GenNetworkInterface<NetworkPacket>::handle_new_packet_event,
						    *(dram_sim->get_clock()), Clock::Master(), 1, 1);
				break;
			    default:
				assert(0);
			}
                      }
		    }
                    else
                    {
		      assert(node_id >= 0 && node_id < int(ni_cids.size()) );
		      if(dram_sim) { //no need to call Connect if MC is not in this LP
			//????????????????????????? todo: MCP use proper clock!!
			switch(m_sysBuilder->get_cache_builder()->get_type()) {
			    case CacheBuilder::MCP_CACHE:
			    case CacheBuilder::MCP_L1L2:
				Manifold :: Connect(mc_cid, Controller::PORT0,
				                    &Dram_sim::handle_incoming<manifold::mcp_cache_namespace::Mem_msg>,
						    ni_cids[node_id], GenNetworkInterface<NetworkPacket>::TERMINAL_PORT,
						    &GenNetworkInterface<NetworkPacket>::handle_new_packet_event,
						    *(dram_sim->get_clock()), Clock::Master(), 1, 1);
				break;
			    default:
				assert(0);
			}
		      }
                    }

		}//for
	    }
	    break;
        default:
	    assert(0);
	    break;
    }//switch
*/
}



void hmc_builder :: print_config(std::ostream& out)
{
    MemControllerBuilder :: print_config(out);
    out << "  MC type: DRAMSim2\n";
    out << "  device file: " << m_DEV_FILE << "\n"
        << "  system file: " << m_SYS_FILE << "\n"
	<< "  size: " << m_MEM_SIZE << "\n";
    out << "  clock: ";
    if(m_use_default_clock)
        out << "default\n";
    else {
        for(unsigned i=0; i<m_CLOCK_FREQ.size()-1; i++)
	    out << m_CLOCK_FREQ[i] << ", ";
	out << m_CLOCK_FREQ[m_CLOCK_FREQ.size()-1] << "\n";
    }
    
    out << "Network type: Iris\n";
    out << "  topology: " << m_net_topo << endl;
    out << "  X dim: " << m_x_dimension << endl
        << "  Y dim: " << m_y_dimension << endl;

    if(m_default_simLen)
        out << "  use default simLen object!\n";
    if(m_default_vnet)
        out << "  use default vnet object!\n";
}


void hmc_builder :: print_stats(std::ostream& out)
{
    for(map<int, int>::iterator it = m_mc_id_cid_map.begin(); it != m_mc_id_cid_map.end(); ++it) {
        int cid = (*it).second;
	Dram_sim* mc = Component :: GetComponent<Dram_sim>(cid);
	if(mc)
	    mc->print_stats(out);
    }
    
    m_torus->print_stats(out);
}


