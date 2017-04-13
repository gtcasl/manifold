#include "sysBuilder_l1l2.h"

using namespace manifold::kernel;
using namespace libconfig;

//====================================================================
//====================================================================
void SysBuilder_l1l2 :: config_components()
{
    try {
	    //simulation parameters
	    STOP = m_config.lookup("simulation_stop");

	    try {
	        m_DEFAULT_CLOCK_FREQ = m_config.lookup("default_clock");
	        assert(m_DEFAULT_CLOCK_FREQ > 0);
	    }
	    catch (SettingNotFoundException e) {
	        //if default clock not defined
	        m_DEFAULT_CLOCK_FREQ = -1;
	    }


    	//network
	    try {
	        const char* net_chars = m_config.lookup("network_type");
	        string net_str = net_chars;
	        if(net_str == "IRIS") m_network_builder = new Iris_builder();
	        else {
		        cerr << "Network type  " << net_str << "  not supported\n";
		        exit(1);
	        }
	    }
	    catch (SettingNotFoundException e) {
	        m_network_builder = new Iris_builder(); //default is Iris
	    }

	    // Determine network topology
	    m_network_builder->read_network_topology(m_config);
	    MAX_NODES = m_network_builder->get_max_nodes();


	    // processor
	    const char* proc_chars = m_config.lookup("processor.type");
	    string proc_str = proc_chars;
   /*     if(proc_str == "ZESTO") m_proc_builder = new Zesto_builder(this);*/
		//else if (proc_str == "SIMPLE") m_proc_builder = new Simple_builder(this);
        //else if(proc_str == "SPX") m_proc_builder = new Spx_builder(this);
        if (proc_str == "SPX") m_proc_builder = new Spx_builder(this);
        else {
	        cerr << "Processor type  " << proc_str << "  not supported\n";
		    exit(1);
	    }
	    m_proc_builder->read_config(m_config);


	    //cache
	    try {
	        const char* cache_chars = m_config.lookup("cache_type");
	        string cache_str = cache_chars;
	        if(cache_str == "MCP_L1L2") m_cache_builder = new MCP_l1l2_builder(this);
            else {
	            cerr << "Cache type  " << cache_str << "  not supported\n";
		        exit(1);
	        }
        }
	    catch (SettingNotFoundException e) {
	        m_cache_builder = new MCP_l1l2_builder(this);
	    }
	    m_cache_builder->read_cache_config(m_config);


	    //memory controller
	    const char* mem_chars = m_config.lookup("mc.type");
	    string mem_str = mem_chars;
	    if(mem_str == "CAFFDRAM") m_mc_builder = new CaffDRAM_builder(this);
	    else if(mem_str == "DRAMSIM") m_mc_builder = new DramSim_builder(this);
        else {
	        cerr << "Memory controller type  " << mem_str << "  not supported\n";
		    exit(1);
	    }
	    m_mc_builder->read_config(m_config);


    	//processor assignment
	    //the node indices of processors are in an array, each value between 0 and MAX_NODES-1
	    Setting& setting_proc = m_config.lookup("processor.node_idx");
    	int num_proc = setting_proc.getLength(); //number of processors
	    assert(num_proc >=1 && num_proc < MAX_NODES);

	    this->proc_node_idx_vec.resize(num_proc);

	    for(int i=0; i<num_proc; i++) {
	        assert((int)setting_proc[i] >=0 && (int)setting_proc[i] < MAX_NODES);
	        proc_node_idx_set.insert((int)setting_proc[i]);
	        this->proc_node_idx_vec[i] = (int)setting_proc[i];
	    }
	    assert(proc_node_idx_set.size() == (unsigned)num_proc); //verify no 2 indices are the same


	    //memory controller assignment
	    //the node indices of MC are in an array, each value between 0 and MAX_NODES-1
	    Setting& setting_mc = m_config.lookup("mc.node_idx");
	    int num_mc = setting_mc.getLength(); //number of mem controllers
	    assert(num_mc >=1 && num_mc < MAX_NODES);

	    this->mc_node_idx_vec.resize(num_mc);

	    for(int i=0; i<num_mc; i++) {
	        assert((int)setting_mc[i] >=0 && (int)setting_mc[i] < MAX_NODES);
	        mc_node_idx_set.insert((int)setting_mc[i]);
	        this->mc_node_idx_vec[i] = (int)setting_mc[i];
	    }
	    assert(mc_node_idx_set.size() == (unsigned)num_mc); //verify no 2 indices are the same

	    //verify MC indices are not used by processors
	    for(int i=0; i<num_mc; i++) {
	        for(int j=0; j<num_proc; j++) 
	            assert(this->mc_node_idx_vec[i] != this->proc_node_idx_vec[j]);
	    }


	    //L2 configuration
	    //the node indices of L2 are in an array, each value between 0 and MAX_NODES-1
	    Setting& setting_l2 = m_config.lookup("l2_cache.node_idx");
	    int num_l2 = setting_l2.getLength(); //number of L2 nodes
	    assert(num_l2 >=1 && num_l2 < MAX_NODES);

	    this->l2_node_idx_vec.resize(num_l2);

	    for(int i=0; i<num_l2; i++) {
	        assert((int)setting_l2[i] >=0 && (int)setting_l2[i] < MAX_NODES);
	        this->l2_node_idx_set.insert((int)setting_l2[i]);
	        this->l2_node_idx_vec[i] = (int)setting_l2[i];
	    }
	    assert(l2_node_idx_set.size() == (unsigned)num_l2); //verify no 2 indices are the same

	    //verify L2 indices are not used by processors
	    for(int i=0; i<num_l2; i++) {
	        for(int j=0; j<num_proc; j++)
	        assert(this->l2_node_idx_vec[i] != this->proc_node_idx_vec[j]);
	    }
	    //verify L2 indices are not used by MCs
	    for(int i=0; i<num_l2; i++) {
	        for(int j=0; j<num_mc; j++)
	            assert(this->l2_node_idx_vec[i] != this->mc_node_idx_vec[j]);
	    }
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
void SysBuilder_l1l2 :: do_partitioning_1_part(int n_lps)
{
    int lp_idx = 1; //the network is LP 0
    for(int i=0; i<MAX_NODES; i++) {
        if(n_lps == 1)
            lp_idx = 0;
        else if(n_lps == 2)
            lp_idx = 1;

        if(mc_node_idx_set.find(i) != mc_node_idx_set.end()) { //MC node
            m_node_conf[i].type = MC_NODE;
            m_node_conf[i].lp = 0;
	        mc_id_lp_map[i] = m_node_conf[i].lp;
        }
        else if(l2_node_idx_set.find(i) != l2_node_idx_set.end()) { //L2 node
            m_node_conf[i].type = L2_NODE;
            m_node_conf[i].lp = 0;
	        l2_id_lp_map[i] = m_node_conf[i].lp;
        }
        else if(proc_node_idx_set.find(i) != proc_node_idx_set.end()) {
            m_node_conf[i].type = CORE_NODE;
            if(n_lps < 3) m_node_conf[i].lp = lp_idx;
            else m_node_conf[i].lp = lp_idx%(n_lps-1)+1;
	        proc_id_lp_map[i] = m_node_conf[i].lp;
            lp_idx++;
        }
        else { m_node_conf[i].type = EMPTY_NODE; }
    }
}


//====================================================================
//====================================================================
void SysBuilder_l1l2 :: do_partitioning_y_part(int n_lps)
{
    int x_dim = m_network_builder->get_x_dim();
    int y_dim = m_network_builder->get_y_dim();
    assert(y_dim > 1);
    assert(n_lps > 2);

    int lp_idx = y_dim; //the network is LP 0 -- y_dim-1
    for(int i=0; i<MAX_NODES; i++) {
        if(mc_node_idx_set.find(i) != mc_node_idx_set.end()) { //MC node
            m_node_conf[i].type = MC_NODE;
	        m_node_conf[i].lp = i/x_dim;
	        mc_id_lp_map[i] = m_node_conf[i].lp;
        }
        else if(l2_node_idx_set.find(i) != l2_node_idx_set.end()) { //L2 node
            m_node_conf[i].type = L2_NODE;
	        m_node_conf[i].lp = i/x_dim;
	        l2_id_lp_map[i] = m_node_conf[i].lp;
        }
        else if(proc_node_idx_set.find(i) != proc_node_idx_set.end()) {
            m_node_conf[i].type = CORE_NODE;
            if(n_lps < 3) m_node_conf[i].lp = lp_idx;
            else m_node_conf[i].lp = lp_idx%(n_lps-1)+1;
	        proc_id_lp_map[i] = m_node_conf[i].lp;
            lp_idx++;
        }
        else { m_node_conf[i].type = EMPTY_NODE; }
    }
}
