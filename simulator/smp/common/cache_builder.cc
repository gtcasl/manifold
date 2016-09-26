#include "cache_builder.h"
#include "mcp-cache/MESI_LLP_cache.h"
#include "mcp-cache/MESI_LLS_cache.h"
#include "network_builder.h"
#include "sysBuilder_llp.h"
#include "mcp-cache/cache_types.h"
#include "mc_builder.h"

#include "mcp-cache/MESI_L1_cache.h"
#include "mcp-cache/MESI_L2_cache.h"
#include "sysBuilder_l1l2.h"

using namespace manifold::kernel;
using namespace manifold::uarch;
using namespace manifold::mcp_cache_namespace;
using namespace manifold::caffdram;
using namespace manifold::iris;
using namespace libconfig;


void MCP_cache_builder :: set_mc_map_obj(manifold::uarch::DestMap* mc_map)
{
    assert(l2_settings.mc_map == 0);
    l2_settings.mc_map = mc_map;
}




//====================================================================
//====================================================================
void MCP_lp_lls_builder :: read_cache_config(Config& config)
{
    try {
	l1_cache_parameters.name = config.lookup("llp_cache.name");
	l1_cache_parameters.size = config.lookup("llp_cache.size");
	l1_cache_parameters.assoc = config.lookup("llp_cache.assoc");
	l1_cache_parameters.block_size = config.lookup("llp_cache.block_size");
	l1_cache_parameters.hit_time = config.lookup("llp_cache.hit_time");
	l1_cache_parameters.lookup_time = config.lookup("llp_cache.lookup_time");
	l1_cache_parameters.replacement_policy = RP_LRU;

	l1_settings.mshr_sz = config.lookup("llp_cache.mshr_size");
	l1_settings.downstream_credits = config.lookup("llp_cache.downstream_credits");

	l2_cache_parameters.name = config.lookup("lls_cache.name");
	l2_cache_parameters.size = config.lookup("lls_cache.size");
	l2_cache_parameters.assoc = config.lookup("lls_cache.assoc");
	l2_cache_parameters.block_size = config.lookup("lls_cache.block_size");
	l2_cache_parameters.hit_time = config.lookup("lls_cache.hit_time");
	l2_cache_parameters.lookup_time = config.lookup("lls_cache.lookup_time");
	l2_cache_parameters.replacement_policy = RP_LRU;

	l2_settings.mshr_sz = config.lookup("lls_cache.mshr_size");
	l2_settings.downstream_credits = config.lookup("lls_cache.downstream_credits");

	m_COH_MSG_TYPE = config.lookup("network.coh_msg_type");
	m_MEM_MSG_TYPE = config.lookup("network.mem_msg_type");
	m_CREDIT_MSG_TYPE = config.lookup("network.credit_msg_type");
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


//====================================================================
//====================================================================
void MCP_lp_lls_builder :: create_caches(Clock& clk)
{
    L1_cache :: Set_msg_types(m_COH_MSG_TYPE, m_CREDIT_MSG_TYPE);
    L2_cache :: Set_msg_types(m_COH_MSG_TYPE, m_MEM_MSG_TYPE, m_CREDIT_MSG_TYPE);


    map<int,int>& id_lp = m_sysBuilder->get_proc_id_lp_map();

    vector<int> node_idx_vec;
    for(map<int,int>::iterator it = id_lp.begin(); it != id_lp.end(); ++it) {
        node_idx_vec.push_back((*it).first);
    }

    PageBasedMap* l2_map = new PageBasedMap(node_idx_vec, 12); //page size = 2^12
    l1_settings.l2_map = l2_map;


    for(map<int,int>::iterator it = id_lp.begin(); it != id_lp.end(); ++it) {
        int node_id = (*it).first;
        int lp = (*it).second;
	LP_LLS_unit* unit = new LP_LLS_unit(lp, node_id, l1_cache_parameters, l2_cache_parameters, l1_settings, l2_settings, clk, clk, clk, m_CREDIT_MSG_TYPE); 
	m_caches[node_id] = unit;
    }
}



//====================================================================
//====================================================================
void MCP_lp_lls_builder :: connect_cache_network(NetworkBuilder* net_builder)
{
    switch(net_builder->get_type()) {
        case NetworkBuilder::IRIS: {
		Iris_builder* irisBuilder = dynamic_cast<Iris_builder*>(net_builder);
		assert(irisBuilder != 0);

		const std::vector<CompId_t>& ni_cids = net_builder->get_interface_cid();
		for(map<int, LP_LLS_unit*>::iterator it = m_caches.begin(); it != m_caches.end(); ++it) {
		    int node_id = (*it).first;
		    LP_LLS_unit* unit = (*it).second;
		    int cache_cid = unit->get_mux_cid();
                    if (irisBuilder->get_topology() == "TORUS6P")
                    {
		      assert(node_id >= 0 && node_id < int(ni_cids.size())/2 );
		      //????????????????????????? todo: use proper clock!!
		      switch(m_sysBuilder->get_mc_builder()->get_type()) {
		        case MemControllerBuilder::CAFFDRAM:
			    Manifold :: Connect(cache_cid, MuxDemux::PORT_NET, &MuxDemux::handle_net<manifold::mcp_cache_namespace::Mem_msg>,
						ni_cids[node_id*2], GenNetworkInterface<NetworkPacket>::TERMINAL_PORT,
						&GenNetworkInterface<NetworkPacket>::handle_new_packet_event, Clock::Master(), Clock::Master(), 1, 1);
			    break;
		        case MemControllerBuilder::DRAMSIM:
                       //     cout << node_id << " " << ni_cids[node_id] << endl;
			    Manifold :: Connect(cache_cid, MuxDemux::PORT_NET, &MuxDemux::handle_net<manifold::uarch::Mem_msg>,
						ni_cids[node_id*2], GenNetworkInterface<NetworkPacket>::TERMINAL_PORT,
						&GenNetworkInterface<NetworkPacket>::handle_new_packet_event, Clock::Master(), Clock::Master(), 1, 1);
			    break;
			default:
			    assert(0);
			    break;
                      }
		    }
                    else
                    {
		      assert(node_id >= 0 && node_id < int(ni_cids.size()) );
		      //????????????????????????? todo: use proper clock!!
		      switch(m_sysBuilder->get_mc_builder()->get_type()) {
		        case MemControllerBuilder::CAFFDRAM:
			    Manifold :: Connect(cache_cid, MuxDemux::PORT_NET, &MuxDemux::handle_net<manifold::mcp_cache_namespace::Mem_msg>,
						ni_cids[node_id], GenNetworkInterface<NetworkPacket>::TERMINAL_PORT,
						&GenNetworkInterface<NetworkPacket>::handle_new_packet_event, Clock::Master(), Clock::Master(), 1, 1);
			    break;
		        case MemControllerBuilder::DRAMSIM:
			    Manifold :: Connect(cache_cid, MuxDemux::PORT_NET, &MuxDemux::handle_net<manifold::uarch::Mem_msg>,
						ni_cids[node_id], GenNetworkInterface<NetworkPacket>::TERMINAL_PORT,
						&GenNetworkInterface<NetworkPacket>::handle_new_packet_event, Clock::Master(), Clock::Master(), 1, 1);
			    break;
			default:
			    assert(0);
			    break;
		      }
                    }
		}
	    }
	    break;
	default:
	    assert(0);
	    break;
    }//switch

}


//====================================================================
//====================================================================
void MCP_lp_lls_builder :: pre_simulation()
{

#ifdef FORECAST_NULL
    for(map<int, LP_LLS_unit*>::iterator it = m_caches.begin(); it != m_caches.end(); ++it) {
	LP_LLS_unit* unit = (*it).second;
	MuxDemux* muxp = unit->get_mux();
	if(muxp) {
	    Clock::Master().register_output_predictor(muxp, &MuxDemux::do_output_to_net_prediction);
	}
    }
#endif

}


//====================================================================
//====================================================================
void MCP_lp_lls_builder :: print_config(ostream& out)
{
    out << "Cache type: MCP\n";
}


//====================================================================
//====================================================================
void MCP_lp_lls_builder :: print_stats(ostream& out)
{
    for(map<int, LP_LLS_unit*>::iterator it = m_caches.begin(); it != m_caches.end(); ++it) {
	LP_LLS_unit* unit = (*it).second;
	int l1_cid = unit->get_llp_cid();
	int l2_cid = unit->get_lls_cid();
	MESI_LLP_cache* l1 = Component :: GetComponent<MESI_LLP_cache>(l1_cid);
	if(l1) {
	    MESI_LLS_cache* l2 = Component :: GetComponent<MESI_LLS_cache>(l2_cid);
	    l1->print_stats(out);
	    l2->print_stats(out);
	    unit->get_mux()->print_stats(out);
	}
    }

}



#if 0
using namespace libconfig;
using namespace manifold::kernel;
using namespace manifold::uarch;
using namespace manifold::mcp_cache_namespace;
using namespace manifold::iris;
#endif

//#####################################################################
//#####################################################################
MCP_l1l2_builder :: MCP_l1l2_builder(SysBuilder_llp* b) : MCP_cache_builder(b)
{
    SysBuilder_l1l2* builder = dynamic_cast<SysBuilder_l1l2*>(b);
    assert(builder);
}

//====================================================================
//====================================================================
void MCP_l1l2_builder :: read_cache_config(Config& config)
{
    try {
	//cache parameters
	l1_cache_parameters.name = config.lookup("l1_cache.name");
	l1_cache_parameters.size = config.lookup("l1_cache.size");
	l1_cache_parameters.assoc = config.lookup("l1_cache.assoc");
	l1_cache_parameters.block_size = config.lookup("l1_cache.block_size");
	l1_cache_parameters.hit_time = config.lookup("l1_cache.hit_time");
	l1_cache_parameters.lookup_time = config.lookup("l1_cache.lookup_time");
	l1_cache_parameters.replacement_policy = RP_LRU;

	l1_settings.mshr_sz = config.lookup("l1_cache.mshr_size");
	l1_settings.downstream_credits = config.lookup("l1_cache.downstream_credits");

	l2_cache_parameters.name = config.lookup("l2_cache.name");
	l2_cache_parameters.size = config.lookup("l2_cache.size");
	l2_cache_parameters.assoc = config.lookup("l2_cache.assoc");
	l2_cache_parameters.block_size = config.lookup("l2_cache.block_size");
	l2_cache_parameters.hit_time = config.lookup("l2_cache.hit_time");
	l2_cache_parameters.lookup_time = config.lookup("l2_cache.lookup_time");
	l2_cache_parameters.replacement_policy = RP_LRU;

	l2_settings.mshr_sz = config.lookup("l2_cache.mshr_size");
	l2_settings.downstream_credits = config.lookup("l2_cache.downstream_credits");

	m_COH_MSG_TYPE = config.lookup("network.coh_msg_type");
	m_MEM_MSG_TYPE = config.lookup("network.mem_msg_type");
	m_CREDIT_MSG_TYPE = config.lookup("network.credit_msg_type");
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


//====================================================================
//====================================================================
void MCP_l1l2_builder :: create_caches(Clock& clk)
{
    L1_cache :: Set_msg_types(m_COH_MSG_TYPE, m_CREDIT_MSG_TYPE);
    L2_cache :: Set_msg_types(m_COH_MSG_TYPE, m_MEM_MSG_TYPE, m_CREDIT_MSG_TYPE);


    SysBuilder_l1l2* builder = dynamic_cast<SysBuilder_l1l2*>(m_sysBuilder);
    assert(builder);

    map<int,int>& l2_id_lp = builder->get_l2_id_lp_map();

    //first create mapping object for L1s
    vector<int> l2_idx_vec;
    for(map<int,int>::iterator it = l2_id_lp.begin(); it != l2_id_lp.end(); ++it) {
        l2_idx_vec.push_back((*it).first);
    }

    PageBasedMap* l2_map = new PageBasedMap(l2_idx_vec, 12); //page size = 2^12
    l1_settings.l2_map = l2_map;


    //create L1s
    map<int,int>& l1_id_lp = builder->get_proc_id_lp_map();
    for(map<int,int>::iterator it = l1_id_lp.begin(); it != l1_id_lp.end(); ++it) {
        int node_id = (*it).first;
        int lp = (*it).second;
	m_l1_cids[node_id] = Component::Create<MESI_L1_cache>(lp, node_id, l1_cache_parameters, l1_settings);

	MESI_L1_cache* l1 = manifold::kernel::Component :: GetComponent<MESI_L1_cache>(m_l1_cids[node_id]);
	if(l1) {
	    Clock :: Register(clk, (L1_cache*)l1, &L1_cache::tick, (void(L1_cache::*)(void))0);
	}
    }

    //create L2s
    for(map<int,int>::iterator it = l2_id_lp.begin(); it != l2_id_lp.end(); ++it) {
        int node_id = (*it).first;
        int lp = (*it).second;
	//L2s are not registered with clock
	m_l2_cids[node_id] = Component::Create<MESI_L2_cache>(lp, node_id, l2_cache_parameters, l2_settings);
    }

}



//====================================================================
//====================================================================
void MCP_l1l2_builder :: connect_cache_network(NetworkBuilder* net_builder)
{
    switch(net_builder->get_type()) {
        case NetworkBuilder::IRIS: {
		Iris_builder* irisBuilder = dynamic_cast<Iris_builder*>(net_builder);
		assert(irisBuilder != 0);

		const std::vector<CompId_t>& ni_cids = net_builder->get_interface_cid();

		//connect L1 to network
		for(map<int, int>::iterator it = m_l1_cids.begin(); it != m_l1_cids.end(); ++it) {
		    int node_id = (*it).first;
		    assert(node_id >= 0 && node_id < int(ni_cids.size()) );
		    int cache_cid = (*it).second;
		    //????????????????????????? todo: use proper clock!!
		    Manifold :: Connect(cache_cid, L1_cache::PORT_L2, &L1_cache::handle_peer_and_manager_request,
					ni_cids[node_id], GenNetworkInterface<NetworkPacket>::TERMINAL_PORT,
					&GenNetworkInterface<NetworkPacket>::handle_new_packet_event, Clock::Master(), Clock::Master(), 1, 1);
		}

		//connect L2 to network
		for(map<int, int>::iterator it = m_l2_cids.begin(); it != m_l2_cids.end(); ++it) {
		    int node_id = (*it).first;
		    assert(node_id >= 0 && node_id < int(ni_cids.size()) );
		    int cache_cid = (*it).second;
		    //????????????????????????? todo: use proper clock!!
		    switch(m_sysBuilder->get_mc_builder()->get_type()) {
		        case MemControllerBuilder::CAFFDRAM:
			    Manifold :: Connect(cache_cid, L2_cache::PORT_L1, &L2_cache::handle_incoming<manifold::mcp_cache_namespace::Mem_msg>,
						ni_cids[node_id], GenNetworkInterface<NetworkPacket>::TERMINAL_PORT,
						&GenNetworkInterface<NetworkPacket>::handle_new_packet_event, Clock::Master(), Clock::Master(), 1, 1);
			    break;
			default:
			    assert(0);
			    break;
		    }
		}
	    }
	    break;
	default:
	    assert(0);
	    break;
    }//switch

}


//====================================================================
//====================================================================
void MCP_l1l2_builder :: print_config(ostream& out)
{
    out << "Cache type: MCP_L1L2\n";
}


//====================================================================
//====================================================================
void MCP_l1l2_builder :: print_stats(ostream& out)
{
    for(map<int, int>::iterator it = m_l1_cids.begin(); it != m_l1_cids.end(); ++it) {
	int node_id = (*it).first;
	MESI_L1_cache* l1 = manifold::kernel::Component :: GetComponent<MESI_L1_cache>(m_l1_cids[node_id]);
	if(l1) {
	    l1->print_stats(out);
	}
    }

    for(map<int, int>::iterator it = m_l2_cids.begin(); it != m_l2_cids.end(); ++it) {
	int node_id = (*it).first;
	MESI_L2_cache* l2 = manifold::kernel::Component :: GetComponent<MESI_L2_cache>(m_l2_cids[node_id]);
	if(l2) {
	    l2->print_stats(out);
	}
    }

}


