#include "proc_builder.h"
#include "sysBuilder_llp.h"
//#include "zesto/qsimclient-core.h"
//#include "zesto/qsimlib-core.h"
//#include "zesto/trace-core.h"
//#include "simple-proc/qsim-proc.h"
//#include "simple-proc/qsimlib-proc.h"
//#include "simple-proc/trace-proc.h"
#include "spx/core.h"
#include "proxy/proxy.h"
#include "mcp-cache/MESI_LLP_cache.h"
#include "mcp-cache/MESI_L1_cache.h"

using namespace libconfig;
using namespace manifold::kernel;
//using namespace manifold::zesto;
//using namespace manifold::simple_proc;
using namespace manifold::spx;
using namespace manifold::qsim_proxy;
using namespace manifold::mcp_cache_namespace;
using namespace manifold::qsim_interrupt_handler;


void ProcBuilder :: print_config(std::ostream& out)
{
    out << "Processor:" << endl;
    out << "  front-end: ";
    switch(m_fe_type) {
        case QSIMCLIENT: {
	        out << " qsimclient\n";
	        break;
        }
        case QSIMLIB: {
	        out << " qsimlib\n";
	        break;
        }
        case QSIMPROXY: {
            out << " qsimproxy\n";
            break;
        }
        case TRACE: {
	        out << " trace\n";
	        break;
        }
        default: { assert(0); }
    }
    out << "  clock: ";
    if(m_use_default_clock) {
        out << "default, freq= " << m_sysBuilder->get_default_clock()->freq << endl;
    }
    else {
        for(unsigned i=0; i<m_CLOCK_FREQ.size()-1; i++)
	        out << m_CLOCK_FREQ[i] << ", ";
	    out << m_CLOCK_FREQ[m_NUM_PROC-1];
    }
}

#if 0
//####################################################################
//####################################################################
void Zesto_builder :: read_config(Config& config)
{
    try {
	    Setting& proc_nodes = config.lookup("processor.node_idx");
	    m_NUM_PROC = proc_nodes.getLength();

	    try {
	        Setting& proc_clocks = config.lookup("processor.clocks");
	        assert((unsigned)proc_clocks.getLength() == m_NUM_PROC);
	        m_CLOCK_FREQ.resize(m_NUM_PROC);

	        for(unsigned i=0; i<m_NUM_PROC; i++)
		        m_CLOCK_FREQ[i] = (double)proc_clocks[i];

	        m_use_default_clock = false;
	    }   
	    catch (SettingNotFoundException e) {
	        //clock not defined; use default
	        m_use_default_clock = true;
	    }

	    const char* chars = config.lookup("processor.config");
	    m_CONFIG_FILE = chars;
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

void Zesto_builder :: create_qsimclient_procs(std::map<int,int>& id_lp)
{
    assert(m_fe_type == QSIMCLIENT);
    assert(m_proc_id_cid_map.size() == 0);

    manifold::zesto::QsimClient_Settings proc_settings(m_server, m_port);
    int cpuid = 0;
    int i=0;
    for(map<int,int>::iterator it = id_lp.begin(); it != id_lp.end(); ++it) {
        int node_id = (*it).first;
        int lp = (*it).second;
        int cid = manifold::kernel::Component::Create<qsimclient_core_t>(lp, node_id, (char*)m_CONFIG_FILE.c_str(), cpuid, proc_settings);
	    cpuid++;
        m_proc_id_cid_map[node_id] = cid;
	    qsimclient_core_t* proc = manifold::kernel::Component :: GetComponent<qsimclient_core_t>(cid);
	    Clock* clk = 0;
	    if(m_use_default_clock) {
	        clk = m_sysBuilder->get_default_clock();
	    }
	    else {
	        clk = m_clocks[i++];
	    }
	    assert(clk);

	    if(proc) {
	        Clock :: Register(*clk, (core_t*)proc, &qsimclient_core_t::tick, (void(core_t::*)(void))0);
	    }
    }
}

void Zesto_builder :: create_qsimlib_procs(std::map<int,int>& id_lp)
{
    assert(m_fe_type == QSIMLIB);
    assert(m_proc_id_cid_map.size() == 0);
    assert(m_qsim_osd != 0);

    int cpuid = 0;
    int i=0;
    for(map<int,int>::iterator it = id_lp.begin(); it != id_lp.end(); ++it) {
        int node_id = (*it).first;
        int lp = (*it).second;
	    assert(lp == 0);
        int cid = manifold::kernel::Component::Create<qsimlib_core_t>(lp, node_id, (char*)m_CONFIG_FILE.c_str(), m_qsim_osd, cpuid);
	    cpuid++;
        m_proc_id_cid_map[node_id] = cid;
	    qsimlib_core_t* proc = manifold::kernel::Component :: GetComponent<qsimlib_core_t>(cid);
	    assert(proc);
	    Clock* clk = 0;
	    if(m_use_default_clock) {
	        clk = m_sysBuilder->get_default_clock();
	    }
	    else {
	        clk = m_clocks[i++];
	    }
	    assert(clk);
	    Clock :: Register(*clk, (core_t*)proc, &qsimlib_core_t::tick, (void(core_t::*)(void))0);
    }
    qsimlib_core_t :: Start_qsim(m_qsim_osd);
}

void Zesto_builder :: create_qsimproxy_procs(std::map<int,int>& id_lp)
{
    cerr << "Zesto does not support Qsim Proxy!\n";
    exit(1);
}

void Zesto_builder :: create_trace_procs(std::map<int,int>& id_lp)
{
    assert(m_fe_type == TRACE);
    assert(m_proc_id_cid_map.size() == 0);

    int cpuid = 0;
    int i=0;
    for(map<int,int>::iterator it = id_lp.begin(); it != id_lp.end(); ++it) {
        char buf[100];
	    sprintf(buf, "%s%d", m_trace, cpuid);
        int node_id = (*it).first;
        int lp = (*it).second;
        int cid = manifold::kernel::Component::Create<trace_core_t>(lp, node_id, (char*)m_CONFIG_FILE.c_str(), buf);
	    cpuid++;
        m_proc_id_cid_map[node_id] = cid;
	    trace_core_t* proc = manifold::kernel::Component :: GetComponent<trace_core_t>(cid);
	    Clock* clk = 0;
	    if(m_use_default_clock) {
	        clk = m_sysBuilder->get_default_clock();
	    }
	    else {
	        clk = m_clocks[i++];
	    }
	    assert(clk);
	    if(proc) {
	        Clock :: Register(*clk, (core_t*)proc, &trace_core_t::tick, (void(core_t::*)(void))0);
	    }
    }
}

void Zesto_builder :: connect_proc_qsim_proxy(QsimBuilder* qsim_builder)
{
    assert(get_fe_type() != QSIMPROXY);
    return;

    /*
    cerr << "Zesto doesn't support Qsim Proxy\n";
    exit(1);
    */
}

void Zesto_builder :: connect_proc_cache(CacheBuilder* cache_builder)
{
    switch(cache_builder->get_type()) {
        case CacheBuilder::MCP_CACHE: {
	        MCP_lp_lls_builder* mcp_builder = dynamic_cast<MCP_lp_lls_builder*>(cache_builder);
		    assert(mcp_builder);

		    map<int, LP_LLS_unit*>& mcp_caches = mcp_builder->get_cache_map();

		    for(map<int,int>::iterator it = m_proc_id_cid_map.begin(); it != m_proc_id_cid_map.end(); ++it) {
		        int node_id = (*it).first;
		        int proc_cid = (*it).second;
		        LP_LLS_unit* unit = mcp_caches[node_id];
		        assert(unit);
		        int cache_cid = unit->get_llp_cid();

		        //connect proc with L1 cache
		        //!!!!!!!!!!!!!!!!!!! todo: use proper clock!
		        Manifold :: Connect(proc_cid, core_t::PORT0, &core_t::cache_response_handler,
					                cache_cid, MESI_LLP_cache::PORT_PROC,
					                &MESI_LLP_cache::handle_processor_request<ZestoCacheReq>, Clock::Master(), Clock::Master(), 1, 1);

		    }//for
	        break;
        }
        case CacheBuilder::MCP_L1L2: {
	        MCP_l1l2_builder* mcp_builder = dynamic_cast<MCP_l1l2_builder*>(cache_builder);
		    assert(mcp_builder);

		    map<int, int>& l1_cids = mcp_builder->get_l1_cids();

		    for(map<int,int>::iterator it = m_proc_id_cid_map.begin(); it != m_proc_id_cid_map.end(); ++it) {
		        int node_id = (*it).first;
		        int proc_cid = (*it).second;
		        int cache_cid = l1_cids[node_id];

		        //connect proc with L1 cache
		        //!!!!!!!!!!!!!!!!!!! todo: use proper clock!
		        Manifold :: Connect(proc_cid, core_t::PORT0, &core_t::cache_response_handler,
					                cache_cid, MESI_L1_cache::PORT_PROC,
					                &MESI_L1_cache::handle_processor_request<ZestoCacheReq>, Clock::Master(), Clock::Master(), 1, 1);

		    }//for
	        break;
        }
        default: { assert(0); }
    }
}

void Zesto_builder :: print_config(std::ostream& out)
{
    ProcBuilder::print_config(out);
    out << "  type: Zesto" << endl;
    out << "  config file: " << m_CONFIG_FILE << endl;
    if(m_fe_type == QSIMCLIENT) {
        out << "  server= " << m_server << "  port= " << m_port << endl;
    }
}

void Zesto_builder :: print_stats(std::ostream& out)
{
    for(map<int,int>::iterator it = m_proc_id_cid_map.begin(); it != m_proc_id_cid_map.end(); ++it) {
        int cid = (*it).second;
	    core_t* proc = Component :: GetComponent<core_t>(cid);
	    if(proc) {
	        proc->print_stats();
	        proc->print_stats(out);
	    }
    }
}


//####################################################################
//####################################################################
void Simple_builder :: create_qsimclient_procs(std::map<int,int>& id_lp)
{
    assert(m_fe_type == QSIMCLIENT);
    assert(m_proc_id_cid_map.size() == 0);

    manifold::simple_proc::QsimProc_Settings proc_settings(m_server, m_port, m_l1_line_sz);
    int cpuid = 0;
    int i=0;
    for(map<int,int>::iterator it = id_lp.begin(); it != id_lp.end(); ++it) {
        int node_id = (*it).first;
        int lp = (*it).second;
        int cid = manifold::kernel::Component::Create<QsimProcessor>(lp, cpuid, node_id, proc_settings);
	    cpuid++;
        m_proc_id_cid_map[node_id] = cid;
	    QsimProcessor* proc = manifold::kernel::Component :: GetComponent<QsimProcessor>(cid);
	    Clock* clk = 0;
	    if(m_use_default_clock) {
	        clk = m_sysBuilder->get_default_clock();
	    }
	    else {
	        clk = m_clocks[i++];
	    }
	    assert(clk);
	    if(proc) {
	        Clock :: Register(*clk, (SimpleProcessor*)proc, &QsimProcessor::tick, (void(SimpleProcessor::*)(void))0);
	    }
    }
}


void Simple_builder :: create_qsimlib_procs(std::map<int,int>& id_lp)
{
    assert(m_fe_type == QSIMLIB);
    assert(m_proc_id_cid_map.size() == 0);

    manifold::simple_proc::SimpleProc_Settings proc_settings(m_l1_line_sz);
    int cpuid = 0;
    int i=0;
    for(map<int,int>::iterator it = id_lp.begin(); it != id_lp.end(); ++it) {
        int node_id = (*it).first;
        int lp = (*it).second;
	    assert(lp == 0);
        int cid = manifold::kernel::Component::Create<QsimLibProcessor>(lp, m_qsim_osd, cpuid, node_id, proc_settings);
	    cpuid++;
        m_proc_id_cid_map[node_id] = cid;
	    QsimLibProcessor* proc = manifold::kernel::Component :: GetComponent<QsimLibProcessor>(cid);
	    Clock* clk = 0;
	    if(m_use_default_clock) {
	        clk = m_sysBuilder->get_default_clock();
	    }
	    else {
	        clk = m_clocks[i++];
	    }
	    assert(clk);
	    if(proc) {
	        Clock :: Register(*clk, (SimpleProcessor*)proc, &QsimLibProcessor::tick, (void(SimpleProcessor::*)(void))0);
	    }
    }

    QsimLibProcessor :: Start_qsim(m_qsim_osd);
}

void Simple_builder :: create_qsimproxy_procs(std::map<int,int>& id_lp)
{
    cerr << "SimpleProc does not support Qsim Proxy!\n";
    exit(1);
}

void Simple_builder :: create_trace_procs(std::map<int,int>& id_lp)
{
    assert(m_fe_type == TRACE);
    assert(m_proc_id_cid_map.size() == 0);

    manifold::simple_proc::SimpleProc_Settings proc_settings(m_l1_line_sz);
    int cpuid = 0;
    int i=0;
    for(map<int,int>::iterator it = id_lp.begin(); it != id_lp.end(); ++it) {
        char buf[100];
    	sprintf(buf, "%s%d", m_trace, cpuid);
        int node_id = (*it).first;
        int lp = (*it).second;
        int cid = manifold::kernel::Component::Create<TraceProcessor>(lp, cpuid, buf, proc_settings);
	    cpuid++;
        m_proc_id_cid_map[node_id] = cid;
	    TraceProcessor* proc = manifold::kernel::Component :: GetComponent<TraceProcessor>(cid);
	    Clock* clk = 0;
	    if(m_use_default_clock) {
	        clk = m_sysBuilder->get_default_clock();
	    }
	    else {
	        clk = m_clocks[i++];
	    }
	    assert(clk);
	    if(proc) {
	        Clock :: Register(*clk, (SimpleProcessor*)proc, &TraceProcessor::tick, (void(SimpleProcessor::*)(void))0);
	    }
    }
}

void Simple_builder :: connect_proc_qsim_proxy(QsimBuilder* qsim_builder)
{
    assert(get_fe_type() != QSIMPROXY);
    return;

    /*
    cerr << "SimpleProc doesn't support Qsim Proxy\n";
    exit(1);
    */
}

void Simple_builder :: connect_proc_cache(CacheBuilder* cache_builder)
{
    switch(cache_builder->get_type()) {
        case CacheBuilder::MCP_CACHE: {
	        MCP_lp_lls_builder* mcp_builder = dynamic_cast<MCP_lp_lls_builder*>(cache_builder);
		    assert(mcp_builder);

		    map<int, LP_LLS_unit*>& mcp_caches = mcp_builder->get_cache_map();

		    for(map<int,int>::iterator it = m_proc_id_cid_map.begin(); it != m_proc_id_cid_map.end(); ++it) {
		        int node_id = (*it).first;
		        int proc_cid = (*it).second;
		        LP_LLS_unit* unit = mcp_caches[node_id];
		        assert(unit);
		        int cache_cid = unit->get_llp_cid();

		        //connect proc with L1 cache
		        //!!!!!!!!!!!!!!!!!!! todo: use proper clock!
		        Manifold :: Connect(proc_cid, SimpleProcessor::PORT_CACHE, &SimpleProcessor::handle_cache_response,
					                cache_cid, MESI_LLP_cache::PORT_PROC,
					                &MESI_LLP_cache::handle_processor_request<manifold::simple_proc::CacheReq>, Clock::Master(), Clock::Master(), 1, 1);

		    }//for
	        break;
        }
        default: { assert(0); }
    }
}

void Simple_builder :: print_config(std::ostream& out)
{
    ProcBuilder::print_config(out);
    out << "  type: SimpleProc" << endl;
}

void Simple_builder :: print_stats(std::ostream& out)
{
    for(map<int,int>::iterator it = m_proc_id_cid_map.begin(); it != m_proc_id_cid_map.end(); ++it) {
        int cid = (*it).second;
	    SimpleProcessor* proc = Component :: GetComponent<SimpleProcessor>(cid);
	    if(proc) {
	        proc->print_stats(out);
	    }
    }
}
#endif

//####################################################################
//####################################################################
void Spx_builder :: read_config(Config& config)
{
    try {
	    const char* chars = config.lookup("processor.config");
	    m_CONFIG_FILE = chars;
    }
    catch (SettingNotFoundException e) {
	    cout << e.getPath() << " not set." << endl;
	    exit(1);
    }
    catch (SettingTypeException e) {
	    cout << e.getPath() << " has incorrect type." << endl;
	    exit(1);
    }
    m_use_default_clock = true;
}

void Spx_builder :: create_qsimlib_procs(std::map<int,int>& id_lp)
{
    cerr << "SPX does not support QsimLib!\n";
    exit(1);
}

void Spx_builder :: create_qsimproxy_procs(std::map<int,int>& id_lp)
{
    assert(m_fe_type == QSIMPROXY);
    assert(m_proc_id_cid_map.size() == 0);

    int cpuid = 0;
    int i=0;
    for(map<int,int>::iterator it = id_lp.begin(); it != id_lp.end(); ++it) {
        int node_id = (*it).first;
        int lp = (*it).second;
        int cid = manifold::kernel::Component::Create<spx_core_t>(lp, node_id, m_CONFIG_FILE.c_str(), cpuid);
	    cpuid++;
        m_proc_id_cid_map[node_id] = cid;
	    spx_core_t* proc = manifold::kernel::Component :: GetComponent<spx_core_t>(cid);

        if(proc) {
	        Clock* clk = 0;
	        if(m_use_default_clock)
	            clk = m_sysBuilder->get_default_clock();
            else
	            clk = m_clocks[i++];
	        assert(clk);
	        Clock :: Register(*clk, (spx_core_t*)proc, &spx_core_t::tick, (void(spx_core_t::*)(void))0);
        }
    }
}

void Spx_builder :: create_qsimclient_procs(std::map<int,int>& id_lp)
{
    cerr << "SPX does not support Qsim client!\n";
    exit(1);
}

void Spx_builder :: create_trace_procs(std::map<int,int>& id_lp)
{
    cerr << "SPX does not support trace files!\n";
    exit(1);
}

#ifdef LIBKITFOX
void Spx_builder :: connect_proc_kitfox_proxy(KitFoxBuilder* kitfox_builder)
{
    int kitfox_cid = kitfox_builder->get_component_id();

    for(map<int,int>::iterator it = m_proc_id_cid_map.begin(); it != m_proc_id_cid_map.end(); ++it) {
        int proc_cid = (*it).second;

        //connect proc with qsim proxy
        Manifold :: Connect(proc_cid, spx_core_t::OUT_TO_QSIM, &spx_core_t::handle_qsim_response,
                            qsim_cid, proc_cid, &qsim_proxy_t::handle_core_request<manifold::spx::qsim_proxy_request_t>,
                            Clock::Master(), Clock::Master(), 1, 1);
    }
}
#endif

void Spx_builder :: connect_proc_qsim_proxy(QsimBuilder* qsim_builder)
{
    int qsim_cid = qsim_builder->get_component_id();

    for(map<int,int>::iterator it = m_proc_id_cid_map.begin(); it != m_proc_id_cid_map.end(); ++it) {
    	int proc_cid = (*it).second;

	    //connect proc with qsim proxy
        Manifold :: Connect(proc_cid, spx_core_t::OUT_TO_QSIM, &spx_core_t::handle_qsim_response,
                            qsim_cid, proc_cid, &qsim_proxy_t::handle_core_request<manifold::spx::qsim_proxy_request_t>,
                            Clock::Master(), Clock::Master(), 1, 1);
    }
}

void Spx_builder :: connect_proc_cache(CacheBuilder* cache_builder)
{
    switch(cache_builder->get_type()) {
        case CacheBuilder::MCP_CACHE: {
	        MCP_lp_lls_builder* mcp_builder = dynamic_cast<MCP_lp_lls_builder*>(cache_builder);
	    	assert(mcp_builder);

		    map<int, LP_LLS_unit*>& mcp_caches = mcp_builder->get_cache_map();

		    for(map<int,int>::iterator it = m_proc_id_cid_map.begin(); it != m_proc_id_cid_map.end(); ++it) {
		        int node_id = (*it).first;
		        int proc_cid = (*it).second;
		        LP_LLS_unit* unit = mcp_caches[node_id];
		        assert(unit);
		        int cache_cid = unit->get_llp_cid();

		        //connect proc with L1 cache
		        //!!!!!!!!!!!!!!!!!!! todo: use proper clock!
		        Manifold :: Connect(proc_cid, spx_core_t::OUT_TO_DATA_CACHE, &spx_core_t::handle_cache_response,
					                cache_cid, MESI_LLP_cache::PORT_PROC,
					                &MESI_LLP_cache::handle_processor_request<manifold::spx::cache_request_t>,
					                Clock::Master(), Clock::Master(), 1, 1);
		    }//for
	        break;
        }
        case CacheBuilder::MCP_L1L2: {
	        MCP_l1l2_builder* mcp_builder = dynamic_cast<MCP_l1l2_builder*>(cache_builder);
		    assert(mcp_builder);

		    map<int, int>& l1_cids = mcp_builder->get_l1_cids();

		    for(map<int,int>::iterator it = m_proc_id_cid_map.begin(); it != m_proc_id_cid_map.end(); ++it) {
		        int node_id = (*it).first;
		        int proc_cid = (*it).second;
		        int cache_cid = l1_cids[node_id];

		        //connect proc with L1 cache
		        //!!!!!!!!!!!!!!!!!!! todo: use proper clock!
                Manifold :: Connect(proc_cid, spx_core_t::OUT_TO_DATA_CACHE, &spx_core_t::handle_cache_response,
                                    cache_cid, MESI_L1_cache::PORT_PROC,
                                    &MESI_L1_cache::handle_processor_request<manifold::spx::cache_request_t>,
                                    Clock::Master(), Clock::Master(), 1, 1);

		    }//for
	        break;
        }
        default: { assert(0); }
    }
}

void Spx_builder :: print_config(std::ostream& out)
{
    ProcBuilder::print_config(out);
    out << "  type: SPX" << endl;
    out << "  config file: " << m_CONFIG_FILE << endl;
}

void Spx_builder :: print_stats(std::ostream& out)
{
    for(map<int,int>::iterator it = m_proc_id_cid_map.begin(); it != m_proc_id_cid_map.end(); ++it) {
        int cid = (*it).second;
	    spx_core_t* proc = manifold::kernel::Component :: GetComponent<spx_core_t>(cid);
	    if(proc) {
	        proc->print_stats(out);
	    }
    }
}
