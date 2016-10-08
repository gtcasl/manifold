#include "sysBuilder_llp.h"

#include "kernel/clock.h"
#include "kernel/component.h"
#include "kernel/manifold.h"
#include "mcp_cache-iris/mcp-iris.h"
#include "CaffDRAM/McMap.h"

using namespace manifold::kernel;
using namespace libconfig;

//====================================================================
//====================================================================
SysBuilder_llp :: SysBuilder_llp(const char* fname)
{
    try {
        m_config.readFile(fname);
        m_config.setAutoConvert(true);
    }
    catch (FileIOException e) {
        cerr << "Cannot read configuration file " << fname << endl;
        exit(1);
    }
    catch (ParseException e) {
        cerr << "Cannot parse configuration file " << fname << endl;
        exit(1);
    }

    m_conf_read = false;

    m_proc_builder = 0;
    m_cache_builder = 0;
    m_network_builder = 0;
    m_mc_builder = 0;
    m_qsim_builder = 0;

#ifdef LIBKITFOX
    m_kitfox_builder = 0;
#endif

    m_qsim_osd = 0;

    m_default_clock = 0;
}

SysBuilder_llp :: ~SysBuilder_llp()
{
    delete m_proc_builder;
    delete m_cache_builder;
    delete m_network_builder;
    delete m_mc_builder;
    delete m_qsim_builder;

    delete m_default_clock;
}




//====================================================================
//====================================================================
void SysBuilder_llp :: config_system()
{
    assert(m_conf_read == false);
    config_components();
    m_conf_read = true;
}



//====================================================================
//====================================================================
void SysBuilder_llp :: config_components()
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


#ifdef LIBKITFOX
        //kitfox
        try {
            const char* kitfox_chars = m_config.lookup("kitfox_config");
            uint64_t sampling_interval = m_config.lookup("kitfox_interval");
            m_kitfox_builder = new KitFoxBuilder(kitfox_chars, sampling_interval);
            if(m_kitfox_builder == NULL)
            {
                    cerr << "KitFox config  " << kitfox_chars << "  contains errors\n";
                    exit(1);
            }
        }
        catch (SettingNotFoundException e) {
            cerr << "KitFox config file not specified\n";
            exit(1);
        }
#endif

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
        //else if (proc_str == "SPX") m_proc_builder = new Spx_builder(this);
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
            if(cache_str == "MCP") m_cache_builder = new MCP_lp_lls_builder(this);
            else {
                cerr << "Cache type  " << cache_str << "  not supported\n";
                exit(1);
            }
        }
        catch (SettingNotFoundException e) {
            m_cache_builder = new MCP_lp_lls_builder(this); //default is MCP_lp_lls
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
// for QsimClient and trace front-end
//====================================================================
void SysBuilder_llp :: build_system(FrontendType type, int n_lps, vector<string>& args, int part)
{
    assert(m_conf_read == true);

    // create default clock
    m_default_clock = 0;
    if(m_DEFAULT_CLOCK_FREQ > 0)
        m_default_clock = new Clock(m_DEFAULT_CLOCK_FREQ);

    //create network
    if(m_network_builder->get_type() == NetworkBuilder :: IRIS) {
        dep_injection_for_iris();
    }

    //??????????????? todo: network should be able to use different clock
    m_network_builder->create_network(*m_default_clock, part);

    assert(m_proc_builder->get_fe_type() == ProcBuilder::INVALID_FE_TYPE);
    switch(type) {
        case FT_QSIMCLIENT: {
            m_proc_builder->set_fe_type(ProcBuilder::QSIMCLIENT);
            create_qsimclient_nodes(n_lps, args, part);
            break;
        }
        case FT_TRACE: {
            m_proc_builder->set_fe_type(ProcBuilder::TRACE);
            create_trace_nodes(n_lps, args, part);
            break;
        }
        default: {
            assert(0);
            break;
        }
    }

    //connect components
    connect_components();
}


//====================================================================
// For QsimLib front-end
//====================================================================
void SysBuilder_llp :: build_system(Qsim::OSDomain* qsim_osd, vector<string>& args)
{
    assert(m_conf_read == true);

    // create default clock
    m_default_clock = 0;
    if(m_DEFAULT_CLOCK_FREQ > 0)
        m_default_clock = new Clock(m_DEFAULT_CLOCK_FREQ);

    //create network
    if(m_network_builder->get_type() == NetworkBuilder :: IRIS) {
        dep_injection_for_iris();
    }

    //??????????????? todo: network should be able to use different clock
    m_network_builder->create_network(*m_default_clock, PART_1);

    assert(m_proc_builder->get_fe_type() == ProcBuilder::INVALID_FE_TYPE);
    m_proc_builder->set_fe_type(ProcBuilder::QSIMLIB);
    create_qsimlib_nodes(qsim_osd, args);

    //connect components
    connect_components();
}


//====================================================================
// For QsimProxy front-end
//====================================================================
void SysBuilder_llp :: build_system(vector<string>& args, const char *stateFile, const char* appFile, int n_lps, int part)
{
    assert(m_conf_read == true);

    // create default clock
    m_default_clock = 0;
    if(m_DEFAULT_CLOCK_FREQ > 0)
        m_default_clock = new Clock(m_DEFAULT_CLOCK_FREQ);

    //create network
    if(m_network_builder->get_type() == NetworkBuilder :: IRIS) {
        dep_injection_for_iris();
    }

    //??????????????? todo: network should be able to use different clock
    m_network_builder->create_network(*m_default_clock, PART_1);

    assert(m_proc_builder->get_fe_type() == ProcBuilder::INVALID_FE_TYPE);
    m_proc_builder->set_fe_type(ProcBuilder::QSIMPROXY);
    create_qsimproxy_nodes(args,stateFile,appFile,n_lps,part);

    //connect components
    connect_components();
}



//====================================================================
//====================================================================
//! @arg \c n_lps  number of LPs
void SysBuilder_llp :: create_qsimclient_nodes(int n_lps, vector<string>& args, int part)
{
    switch(m_proc_builder->get_proc_type()) {
   /*     case ProcBuilder::PROC_ZESTO: {*/
            //if(args.size() != 2) {
                //cerr << "Zesto core needs: <server> <port>\n";
                //exit(1);
            //}
            //Zesto_builder* z = dynamic_cast<Zesto_builder*>(m_proc_builder);
            //assert(z);
            //z->set_qsimclient_vals(args[0].c_str(), atoi(args[1].c_str()));
            //break;
        //}
        //case ProcBuilder::PROC_SIMPLE: {
            //if(args.size() != 2) {
                //cerr << "SimpleProc core needs:  <server> <port>\n";
                //exit(1);
            //}
            //Simple_builder* s = dynamic_cast<Simple_builder*>(m_proc_builder);
            //assert(s);
            //s->set_qsimclient_vals(m_cache_builder->get_l1_block_size(), args[0].c_str(), atoi(args[1].c_str()));
            //break;
        //}

        case ProcBuilder::PROC_SPX: {
            if(args.size() != 0) {
                cerr << "SPX core requires no command-line arguments\n";
                exit(1);
            }
            Spx_builder* s = dynamic_cast<Spx_builder*>(m_proc_builder);
            assert(s);
            break;
        }
        default: { assert(0); }
    }

    create_nodes(FT_QSIMCLIENT, n_lps, part);
}


//====================================================================
//====================================================================
void SysBuilder_llp :: create_qsimlib_nodes(Qsim::OSDomain* qsim_osd, vector<string>& args)
{
    switch(m_proc_builder->get_proc_type()) {
   /*     case ProcBuilder::PROC_ZESTO: {*/
            //if(args.size() != 0) {
                //cerr << "Zesto core needs:  [no arguments]\n";
                //exit(1);
            //}
            //Zesto_builder* z = dynamic_cast<Zesto_builder*>(m_proc_builder);
            //assert(z);
            //z->set_qsimlib_vals(qsim_osd);
            //break;
        //}
        //case ProcBuilder::PROC_SIMPLE: {
            //if(args.size() != 0) {
                //cerr << "SimpleProc core needs:  [no arguments]\n";
                //exit(1);
            //}
            //Simple_builder* s = dynamic_cast<Simple_builder*>(m_proc_builder);
            //assert(s);
            //s->set_qsimlib_vals(m_cache_builder->get_l1_block_size(), qsim_osd);
            //break;
        //}

        case ProcBuilder::PROC_SPX: {
            if(args.size() != 0) {
                cerr << "SPX core needs:  [no arguments]\n";
                exit(1);
            }
            Spx_builder* s = dynamic_cast<Spx_builder*>(m_proc_builder);
            assert(s);
            s->set_qsimlib_vals(qsim_osd);
            break;
        }
        default: { assert(0); }
    }

    create_nodes(FT_QSIMLIB, 1, PART_1);
}


//====================================================================
//====================================================================
void SysBuilder_llp :: create_qsimproxy_nodes(vector<string>& args, const char* stateFile, const char* appFile, int n_lps, int part)
{
    m_qsim_builder = new QsimProxyBuilder(this);
    m_qsim_builder->read_config(m_config, stateFile, appFile);
    m_qsim_builder->create_qsim(0);

    switch(m_proc_builder->get_proc_type()) {
       /* case ProcBuilder::PROC_ZESTO: {*/
            //if(args.size() != 0) {
                //cerr << "Zesto core needs:  [no arguments]\n";
                //exit(1);
            //}
            //Zesto_builder* z = dynamic_cast<Zesto_builder*>(m_proc_builder);
            //assert(z);
            //break;
        //}
        //case ProcBuilder::PROC_SIMPLE: {
            //if(args.size() != 0) {
                //cerr << "SimpleProc core needs:  [no arguments]\n";
                //exit(1);
            //}
            //Simple_builder* s = dynamic_cast<Simple_builder*>(m_proc_builder);
            //assert(s);
            //break;
       /* }*/

        case ProcBuilder::PROC_SPX: {
            if(args.size() != 0) {
                cerr << "SPX core needs:  [no arguments]\n";
                exit(1);
            }
            Spx_builder* s = dynamic_cast<Spx_builder*>(m_proc_builder);
            assert(s);
            break;
        }
        default: { assert(0); }
    }

    create_nodes(FT_QSIMPROXY, n_lps, part);
}


//====================================================================
//====================================================================
//! @arg \c n_lps  number of LPs
void SysBuilder_llp :: create_trace_nodes(int n_lps, vector<string>& args, int part)
{
    switch(m_proc_builder->get_proc_type()) {
       /* case ProcBuilder::PROC_ZESTO: {*/
            //if(args.size() != 1) {
                //cerr << "Usage for Zesto core:  <trace_file_basename>\n";
                //exit(1);
            //}
            //Zesto_builder* z = dynamic_cast<Zesto_builder*>(m_proc_builder);
            //assert(z);
            //z->set_trace_vals(args[0].c_str());
            //break;
        //}
        //case ProcBuilder::PROC_SIMPLE: {
            //if(args.size() != 1) {
                //cerr << "Usage for SimpleProc core:  <trace_file_basename>\n";
                //exit(1);
            //}
            //Simple_builder* s = dynamic_cast<Simple_builder*>(m_proc_builder);
            //assert(s);
            //s->set_trace_vals(m_cache_builder->get_l1_block_size(), args[0].c_str());
            //break;
        //}

        default: { assert(0); }
    }

    create_nodes(FT_TRACE, n_lps, part);
}


//====================================================================
//====================================================================
void SysBuilder_llp :: create_nodes(int type, int n_lps, int part)
{
    m_node_conf.resize(MAX_NODES);

    switch(part) {
        case PART_1: {
            do_partitioning_1_part(n_lps);
            break;
        }
        case PART_Y: {
            do_partitioning_y_part(n_lps);
            break;
        }
        default: { assert(0); }
    }

    switch(type) {
        case FT_QSIMCLIENT: {
            m_proc_builder->create_qsimclient_procs(proc_id_lp_map);
            break;
        }
        case FT_QSIMLIB: {
            m_proc_builder->create_qsimlib_procs(proc_id_lp_map);
            break;
        }
        case FT_QSIMPROXY: {
            m_proc_builder->create_qsimproxy_procs(proc_id_lp_map);
            break;
        }
        case FT_TRACE: {
            m_proc_builder->create_trace_procs(proc_id_lp_map);
            break;
        }
        default: { assert(0); }
    }


    //??????????????????? todo cache should have separate clock
    if(m_cache_builder->get_type() == CacheBuilder::MCP_CACHE || m_cache_builder->get_type() == CacheBuilder::MCP_L1L2) {
        dep_injection_for_mcp();
    }
    m_cache_builder->create_caches(*m_default_clock);
    m_mc_builder->create_mcs(mc_id_lp_map);

}



//====================================================================
//====================================================================
void SysBuilder_llp :: do_partitioning_1_part(int n_lps)
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
void SysBuilder_llp :: do_partitioning_y_part(int n_lps)
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
// todo: move this to cache_builder !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
void SysBuilder_llp :: dep_injection_for_mcp()
{
    if(m_mc_builder->get_type() == MemControllerBuilder::CAFFDRAM) {
        CaffDRAM_builder* caffdram = dynamic_cast<CaffDRAM_builder*>(m_mc_builder);
        manifold::caffdram::CaffDramMcMap* mc_map = new manifold::caffdram::CaffDramMcMap(mc_node_idx_vec, caffdram->get_settings());

        MCP_cache_builder* mcp = dynamic_cast<MCP_cache_builder*>(m_cache_builder);
        mcp->set_mc_map_obj(mc_map);
    }
    else {
        manifold::uarch::PageBasedMap* mc_map = new manifold::uarch::PageBasedMap(mc_node_idx_vec, 12); //assuming page size= 2^12

        MCP_cache_builder* mcp = dynamic_cast<MCP_cache_builder*>(m_cache_builder);
        mcp->set_mc_map_obj(mc_map);
    }
}


//====================================================================
//====================================================================
void SysBuilder_llp :: dep_injection_for_iris()
{
    if(m_cache_builder->get_type() == CacheBuilder::MCP_CACHE || m_cache_builder->get_type() == CacheBuilder::MCP_L1L2) {
        MCP_cache_builder* mcp = dynamic_cast<MCP_cache_builder*>(m_cache_builder);
        MCPSimLen* simLen = new MCPSimLen(mcp->get_l2_block_size(), mcp->get_coh_type(), mcp->get_mem_type(), mcp->get_credit_type(),
                                      this->mc_node_idx_vec);
        MCPVnet* vnet = new MCPVnet(mcp->get_coh_type(), mcp->get_mem_type(), mcp->get_credit_type(), this->mc_node_idx_vec);

        Iris_builder* iris = dynamic_cast<Iris_builder*>(m_network_builder);
    iris->dep_injection(simLen, vnet);
    }
}



//====================================================================
//====================================================================
void SysBuilder_llp :: connect_components()
{
    if(m_qsim_builder)
        m_proc_builder->connect_proc_qsim_proxy(m_qsim_builder);

    m_proc_builder->connect_proc_cache(m_cache_builder);
    m_cache_builder->connect_cache_network(m_network_builder);
    m_mc_builder->connect_mc_network(m_network_builder);

#ifdef LIBKITFOX
    if(m_kitfox_builder){
        m_proc_builder->connect_proc_kitfox_proxy(m_kitfox_builder);
    }
#endif
}


//====================================================================
//====================================================================
void SysBuilder_llp :: pre_simulation()
{
    m_cache_builder->pre_simulation();
    m_network_builder->pre_simulation();
}


//====================================================================
//====================================================================
void SysBuilder_llp :: print_config(ostream& out)
{
    for(int i=0; i<MAX_NODES; i++) {
        out << "node " << i;
    if(m_node_conf[i].type == CORE_NODE)
        out << "  core node";
    else if(m_node_conf[i].type == MC_NODE)
        out << "  mc node";
    else if(m_node_conf[i].type == L2_NODE)
        out << "  l2 node";
    else
        out << "  empty node";
    if(m_node_conf[i].type != EMPTY_NODE)
        out << " lp= " << m_node_conf[i].lp << endl;
        else
        out << "\n";
    }

    out <<"\n********* Configuration *********\n";

#ifdef FORECAST_NULL
    out << "Forecast Null: on\n";
#else
    out << "Forecast Null: off\n";
#endif

    m_proc_builder->print_config(out);
    m_cache_builder->print_config(out);
    m_mc_builder->print_config(out);
    m_network_builder->print_config(out);
    if(m_qsim_builder)
        m_qsim_builder->print_config(out);
}


//====================================================================
//====================================================================
void SysBuilder_llp :: print_stats(ostream& out)
{
    m_proc_builder->print_stats(out);
    m_cache_builder->print_stats(out);
    m_mc_builder->print_stats(out);
    m_network_builder->print_stats(out);

    Manifold::print_stats(out);
}
