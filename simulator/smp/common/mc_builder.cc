#include "mc_builder.h"
#include "sysBuilder_llp.h"
#include "cache_builder.h"
#include "mcp-cache/coh_mem_req.h"
#include "CaffDRAM/McMap.h"

using namespace libconfig;
using namespace manifold::kernel;
using namespace manifold::uarch;
using namespace manifold::mcp_cache_namespace;
using namespace manifold::iris;
using namespace manifold;

void MemControllerBuilder :: print_config(std::ostream& out)
{
    out << "Memory controller:\n";
    out << "  num of nodes: " << m_NUM_MC << endl;

}




void CaffDRAM_builder :: read_config(Config& config)
{
    try {
    Setting& mc_nodes = config.lookup("mc.node_idx");
    m_NUM_MC = mc_nodes.getLength();

    m_MC_DOWNSTREAM_CREDITS = config.lookup("mc.downstream_credits");

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



void CaffDRAM_builder :: create_mcs(map<int, int>& id_lp)
{
    caffdram::Controller::Set_msg_types(m_MEM_MSG_TYPE, m_CREDIT_MSG_TYPE);

    for(map<int,int>::iterator it = id_lp.begin(); it != id_lp.end(); ++it) {
        int node_id = (*it).first;
    int lp = (*it).second;
    int cid = Component::Create<caffdram::Controller>(lp, node_id, m_dram_settings, m_MC_DOWNSTREAM_CREDITS);
    m_mc_id_cid_map[node_id] = cid;
    }

}



void CaffDRAM_builder :: connect_mc_network(NetworkBuilder* net_builder)
{
    switch(net_builder->get_type()) {
        case NetworkBuilder::IRIS: {
        Iris_builder* irisBuilder = dynamic_cast<Iris_builder*>(net_builder);
        assert(irisBuilder != 0);

        const std::vector<CompId_t>& ni_cids = net_builder->get_interface_cid();
        for(map<int, int>::iterator it = m_mc_id_cid_map.begin(); it != m_mc_id_cid_map.end(); ++it) {
            int node_id = (*it).first;
            int mc_cid = (*it).second;
            if (irisBuilder->get_topology() == "TORUS6P")
                {
                    assert(node_id >= 0 && node_id < int(ni_cids.size())/2 );
                    //????????????????????????? todo: use proper clock!!
                    //???????????????????????? todo: Mem_msg is MCP-cache
                    switch(m_sysBuilder->get_cache_builder()->get_type()) {
                    case CacheBuilder::MCP_CACHE:
                    case CacheBuilder::MCP_L1L2:
                        Manifold :: Connect(mc_cid, caffdram::Controller::PORT0, &caffdram::Controller::handle_request<manifold::mcp_cache_namespace::Mem_msg>,
                                            ni_cids[node_id*2+1], GenNetworkInterface<NetworkPacket>::TERMINAL_PORT,
                                            &GenNetworkInterface<NetworkPacket>::handle_new_packet_event, Clock::Master(), Clock::Master(), 1, 1);
                        break;
                    default:
                        assert(0);
                    }
                }
            else
                {
                    assert(node_id >= 0 && node_id < int(ni_cids.size()) );
                    //????????????????????????? todo: use proper clock!!
                    //???????????????????????? todo: Mem_msg is MCP-cache
                    switch(m_sysBuilder->get_cache_builder()->get_type()) {
                    case CacheBuilder::MCP_CACHE:
                    case CacheBuilder::MCP_L1L2:
                        Manifold :: Connect(mc_cid, caffdram::Controller::PORT0, &caffdram::Controller::handle_request<manifold::mcp_cache_namespace::Mem_msg>,
                                            ni_cids[node_id], GenNetworkInterface<NetworkPacket>::TERMINAL_PORT,
                                            &GenNetworkInterface<NetworkPacket>::handle_new_packet_event, Clock::Master(), Clock::Master(), 1, 1);
                        break;
                    default:
                        assert(0);
                    }
                }

        }//for
        }
        break;
        default:
        assert(0);
        break;
    }//switch

}

void CaffDRAM_builder :: set_mc_map_obj(manifold::uarch::DestMap *mc_map)
{
    caffdram::CaffDramMcMap *m = dynamic_cast<caffdram::CaffDramMcMap*>(mc_map);

    for(map<int, int>::iterator it = m_mc_id_cid_map.begin(); it != m_mc_id_cid_map.end(); ++it) {
        int node_id = (*it).first;
        caffdram::Controller* mc = manifold::kernel::Component :: GetComponent<caffdram::Controller>(m_mc_id_cid_map[node_id]);

        if (mc)
            mc->set_mc_map(m);
    }
}


void CaffDRAM_builder :: print_config(std::ostream& out)
{
    MemControllerBuilder :: print_config(out);
    out << "  MC type: CaffDRAM\n";
}


void CaffDRAM_builder :: print_stats(std::ostream& out)
{
    for(map<int, int>::iterator it = m_mc_id_cid_map.begin(); it != m_mc_id_cid_map.end(); ++it) {
        int cid = (*it).second;
    caffdram::Controller* mc = Component :: GetComponent<caffdram::Controller>(cid);
    if(mc)
        mc->print_stats(out);
    }
}


//####################################################################
// DramSim2
//####################################################################

void DramSim_builder :: read_config(Config& config)
{
    try {
    Setting& mc_nodes = config.lookup("mc.node_idx");
    m_NUM_MC = mc_nodes.getLength();

    try {
        Setting& mc_clocks = config.lookup("mc.clocks");
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
    const char* chars = config.lookup("mc.dramsim2.dev_file");
    m_DEV_FILE = chars;
    chars = config.lookup("mc.dramsim2.sys_file");
    m_SYS_FILE = chars;
    m_MEM_SIZE = config.lookup("mc.dramsim2.size");
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



void DramSim_builder :: create_mcs(map<int, int>& id_lp)
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

    dramsim::Dram_sim::Set_msg_types(m_MEM_MSG_TYPE, m_CREDIT_MSG_TYPE);

    dramsim::Dram_sim_settings settings(m_DEV_FILE.c_str(), m_SYS_FILE.c_str(), m_MEM_SIZE, false, m_MC_DOWNSTREAM_CREDITS);

    Clock* clock = 0;
    if(m_use_default_clock)
    clock = m_sysBuilder->get_default_clock();
    else if(m_clocks.size() == 1)
    clock = m_clocks[0];

    int i=0;
    for(map<int,int>::iterator it = id_lp.begin(); it != id_lp.end(); ++it) {
        int node_id = (*it).first;
    int lp = (*it).second;
    if(!m_use_default_clock && m_clocks.size() > 1)
        clock = m_clocks[i++];
    int cid = Component :: Create<dramsim::Dram_sim>(lp, node_id, settings, *clock);
    m_mc_id_cid_map[node_id] = cid;
    }
}



void DramSim_builder :: connect_mc_network(NetworkBuilder* net_builder)
{
    switch(net_builder->get_type()) {
        case NetworkBuilder::IRIS: {
        Iris_builder* irisBuilder = dynamic_cast<Iris_builder*>(net_builder);
        assert(irisBuilder != 0);

        const std::vector<CompId_t>& ni_cids = net_builder->get_interface_cid();
        for(map<int, int>::iterator it = m_mc_id_cid_map.begin(); it != m_mc_id_cid_map.end(); ++it) {
            int node_id = (*it).first;
            int mc_cid = (*it).second;
            dramsim::Dram_sim* dram_sim = Component :: GetComponent<dramsim::Dram_sim>(mc_cid);
                    if (irisBuilder->get_topology() == "TORUS6P")
                    {
              assert(node_id >= 0 && node_id < int(ni_cids.size())/2 );
              if(dram_sim) { //no need to call Connect if MC is not in this LP
            //????????????????????????? todo: MCP use proper clock!!
            switch(m_sysBuilder->get_cache_builder()->get_type()) {
                case CacheBuilder::MCP_CACHE:
                case CacheBuilder::MCP_L1L2:
                Manifold::Connect(mc_cid, dramsim::Dram_sim::PORT0,
                                    &dramsim::Dram_sim::handle_incoming<manifold::mcp_cache_namespace::Mem_msg>,
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
                Manifold :: Connect(mc_cid, dramsim::Dram_sim::PORT0,
                                    &dramsim::Dram_sim::handle_incoming<manifold::mcp_cache_namespace::Mem_msg>,
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

}

void DramSim_builder :: set_mc_map_obj(manifold::uarch::DestMap *mc_map)
{
    for(map<int, int>::iterator it = m_mc_id_cid_map.begin(); it != m_mc_id_cid_map.end(); ++it) {
        int node_id = (*it).first;
        dramsim::Dram_sim* mc = manifold::kernel::Component :: GetComponent<dramsim::Dram_sim>(m_mc_id_cid_map[node_id]);

        if (mc)
            mc->set_mc_map(mc_map);
    }
}

void DramSim_builder :: print_config(std::ostream& out)
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
}


void DramSim_builder :: print_stats(std::ostream& out)
{
    for(map<int, int>::iterator it = m_mc_id_cid_map.begin(); it != m_mc_id_cid_map.end(); ++it) {
        int cid = (*it).second;
    dramsim::Dram_sim* mc = Component :: GetComponent<dramsim::Dram_sim>(cid);
    if(mc)
        mc->print_stats(out);
    }
}

//####################################################################
// PageVault
//####################################################################

void PageVault_builder::read_config(Config &config) {
  pagevault::config_t* _config = pagevault::get_config();
  try {    
    // system settings
    {
      const Setting& mc_nodes = config.lookup("mc.node_idx");
      m_NUM_MC = mc_nodes.getLength();
    }
    // network settings    
    _config->mem_msg_type    = config.lookup("network.mem_msg_type");
    _config->credit_msg_type = config.lookup("network.credit_msg_type");
    _config->downstream_credits = config.lookup("mc.downstream_credits");
    
    // dramsim2 settings
    {
      const Setting& dramsim2_cfg = config.lookup("dramsim2");
      _config->dramsim2.dev_file = (const char*)dramsim2_cfg["dev_file"];
      _config->dramsim2.sys_file = (const char*)dramsim2_cfg["sys_file"];
      _config->dramsim2.size     = dramsim2_cfg["size"];
    }
    
    // hmcsim settings 
    {
      const Setting& hmc_cfg = config.lookup("hmcsim");
      _config->hmc.vault_clock = hmc_cfg["vault_clock"];
      _config->hmc.num_devs    = hmc_cfg["num_devs"];
      _config->hmc.capacity    = hmc_cfg["capacity"];
      _config->hmc.block_size  = hmc_cfg["block_size"];
      _config->hmc.num_links   = hmc_cfg["num_links"];
      _config->hmc.num_vaults  = hmc_cfg["num_vaults"];
      _config->hmc.num_drams   = hmc_cfg["num_drams"];
      _config->hmc.num_banks   = hmc_cfg["num_banks"];
      _config->hmc.queue_depth = hmc_cfg["queue_depth"];
      _config->hmc.xbar_depth  = hmc_cfg["xbar_depth"];
    }    
  } catch (SettingNotFoundException e) {
    cout << e.getPath() << " not set." << endl;
    exit(1);
  } catch (SettingTypeException e) {
    cout << e.getPath() << " has incorrect type." << endl;
    exit(1);
  }
}

void PageVault_builder::create_mcs(map<int, int> &id_lp) {
  // create controller instances  
  Clock *clock = m_sysBuilder->get_default_clock();
  for (map<int, int>::iterator it = id_lp.begin(); it != id_lp.end(); ++it) {
    int node_id = (*it).first;
    int lp = (*it).second;    
    int cid = Component::Create<pagevault::Controller>(lp, node_id, clock);
    m_mc_id_cid_map[node_id] = cid;
  }
}

void PageVault_builder::connect_mc_network(NetworkBuilder *net_builder) {
  switch (net_builder->get_type()) {
  case NetworkBuilder::IRIS: {
    Iris_builder *irisBuilder = dynamic_cast<Iris_builder *>(net_builder);
    assert(irisBuilder != 0);

    const std::vector<CompId_t> &ni_cids = net_builder->get_interface_cid();
    for (map<int, int>::iterator it = m_mc_id_cid_map.begin();
         it != m_mc_id_cid_map.end(); ++it) {
      int node_id = (*it).first;
      assert(node_id >= 0 && node_id < int(ni_cids.size()));
      int mc_cid = (*it).second;
      
      pagevault::Controller* mc =
                Component::GetComponent<pagevault::Controller>(mc_cid);
      assert(mc); 
      
      switch (m_sysBuilder->get_cache_builder()->get_type()) {
      case CacheBuilder::MCP_CACHE:
      case CacheBuilder::MCP_L1L2:
        Manifold::Connect(
            mc_cid, pagevault::Controller::PORT0,
            &pagevault::Controller::handle_request,
            ni_cids[node_id], GenNetworkInterface<NetworkPacket>::TERMINAL_PORT,
            &GenNetworkInterface<NetworkPacket>::handle_new_packet_event,
            *(mc->get_clock()), Clock::Master(), 1, 1);
        break;
      default:
        assert(0);
      }
    } // for
  } break;
  default:
    assert(0);
    break;
  } // switch
}

void PageVault_builder::set_mc_map_obj(manifold::uarch::DestMap *mc_map) {
  for (auto it = m_mc_id_cid_map.begin(); it != m_mc_id_cid_map.end(); ++it) {
    int node_id = it->first;
    pagevault::Controller *mc = manifold::kernel::Component::GetComponent<pagevault::Controller>(m_mc_id_cid_map[node_id]);
    if (mc)
      mc->set_mc_map(mc_map);
  }
}

void PageVault_builder::print_config(std::ostream &out) {
  MemControllerBuilder::print_config(out);
  out << "  MC type: PageVault\n";
}

void PageVault_builder::print_stats(std::ostream &out) {
  for (map<int, int>::iterator it = m_mc_id_cid_map.begin();
       it != m_mc_id_cid_map.end(); ++it) {
    int cid = (*it).second;
    pagevault::Controller *mc =
        Component::GetComponent<pagevault::Controller>(cid);
    if (mc)
      mc->print_stats(out);
  }
}
