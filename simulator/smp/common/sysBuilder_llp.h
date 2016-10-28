#ifndef SYSBUILDER_LLP_H
#define SYSBUILDER_LLP_H

#include <libconfig.h++>
#include <vector>
#include <set>
#include "kernel/manifold.h"
#include "kernel/component.h"
#include "uarch/DestMap.h"
#include "proc_builder.h"
#include "cache_builder.h"
#include "network_builder.h"
#include "mc_builder.h"
#include "qsim_builder.h"
#include "qsim.h"

#ifdef LIBKITFOX
#include "kitfox_builder.h"
#endif

// this data structure to hold a node's type and lp
enum {INVALID_NODE=0, EMPTY_NODE, CORE_NODE, MC_NODE, CORE_MC_NODE, L2_NODE};

struct Node_conf_llp {
    Node_conf_llp() : type(INVALID_NODE) {}

    int type;
    int lp;
};

class SysBuilder_llp {
public:
    enum FrontendType {FT_QSIMCLIENT, FT_QSIMLIB, FT_QSIMPROXY, FT_TRACE}; // front-end type: QSimClient, QSimLib, trace

    enum { PART_1, PART_2, PART_Y}; //torus partitioning

    SysBuilder_llp(const char* fname);
    ~SysBuilder_llp();

    void config_system();
    void build_system(FrontendType type, int n_lps, std::vector<std::string>& args, int part); //for QSim client and tracefile
    void build_system(Qsim::OSDomain* osd, std::vector<std::string>& args); //for QSimLib
    void build_system(std::vector<std::string>& args, const char* stateFile, const char* appFile, int n_lps, int part); // for QSimProxy

    void pre_simulation();
    void print_config(std::ostream& out);
    void print_stats(std::ostream& out);

    libconfig::Config m_config;

    ProcBuilder* get_proc_builder() { return m_proc_builder; }
    CacheBuilder* get_cache_builder() { return m_cache_builder; }
    NetworkBuilder* get_network_builder() { return m_network_builder; }
    MemControllerBuilder* get_mc_builder() { return m_mc_builder; }
    QsimBuilder* get_qsim_builder() { return m_qsim_builder; }

    manifold::kernel::Ticks_t get_stop_tick() { return STOP; }
    size_t get_proc_node_size() { return proc_node_idx_vec.size(); }
    size_t get_mc_node_size() { return mc_node_idx_vec.size(); }
    int get_y_dim() { return m_network_builder->get_y_dim(); }

    std::map<int, int>& get_proc_id_lp_map() { return proc_id_lp_map; }

    manifold::kernel::Clock* get_default_clock() { return m_default_clock; }

protected:
    //enum { PROC_ZESTO, PROC_SIMPLE, PROC_SPX }; //processor model type

    virtual void config_components();
    virtual void create_nodes(int type, int n_lps, int part);

    virtual void do_partitioning_1_part(int n_lps);
    virtual void do_partitioning_y_part(int n_lps);

    ProcBuilder* m_proc_builder;
    CacheBuilder* m_cache_builder;
    NetworkBuilder* m_network_builder;
    MemControllerBuilder* m_mc_builder;
    QsimBuilder *m_qsim_builder;

#ifdef LIBKITFOX
    KitFoxBuilder *m_kitfox_builder;
#endif

    Qsim::OSDomain *m_qsim_osd;

    int MAX_NODES;
    manifold::kernel::Ticks_t STOP; //simulation stop time
    uint64_t m_DEFAULT_CLOCK_FREQ; //default clock's frequency

    std::vector<Node_conf_llp> m_node_conf;

    std::vector<int> proc_node_idx_vec;
    std::vector<int> mc_node_idx_vec;

    std::set<int> proc_node_idx_set; //set is used to ensure each index is unique
    std::set<int> mc_node_idx_set; //set is used to ensure each index is unique

    std::map<int, int> proc_id_lp_map; //maps proc's node id to its LP
    std::map<int, int> mc_id_lp_map; //maps mc's node id to its LP

    //int m_processor_type;
private:

    void create_qsimclient_nodes(int n_lps, std::vector<std::string>& argv, int part);
    void create_qsimlib_nodes(Qsim::OSDomain* qsim_osd, vector<string>& args);
    void create_qsimproxy_nodes(vector<string>& args, const char* stateFile, const char* appFile, int n_lps, int part);
    void create_trace_nodes(int n_lps, vector<string>& args, int part);

#ifdef LIBKITFOX
    void create_kitfoxproxy_nodes(const char* config);
#endif

    void connect_components();

    void dep_injection_for_iris();
    void dep_injection_for_mcp();



    bool m_conf_read; //used to ensure config_system is called first

    manifold::kernel::Clock* m_default_clock;
};





#endif // #ifndef SYSBUILDER_LLP_H
