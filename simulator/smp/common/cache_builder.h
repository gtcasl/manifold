#ifndef CACHE_CONFIG_H
#define CACHE_CONFIG_H

#include <libconfig.h++>
#include <map>
#include "mcp-cache/lp_lls_unit.h"
#include "network_builder.h"

#ifdef LIBKITFOX
#include "kitfox_builder.h"
using namespace manifold::kitfox_proxy;
#endif


class SysBuilder_llp;

class CacheBuilder {
public:
    enum {MCP_CACHE, MCP_L1L2}; //types of caches
    CacheBuilder(SysBuilder_llp* b) : m_sysBuilder(b) {}
    virtual ~CacheBuilder() {}

    virtual void read_cache_config(libconfig::Config& config) = 0;
    virtual unsigned get_l1_block_size() = 0;
    virtual unsigned get_l2_block_size() = 0;
    virtual void create_caches(manifold::kernel::Clock& clk) = 0;
    virtual void connect_cache_network(NetworkBuilder*) = 0;
    virtual int get_type() = 0;
    virtual void set_mc_map_obj(manifold::uarch::DestMap *mc_map) = 0;

    virtual void pre_simulation() {};
    virtual void print_config(std::ostream&) {}
    virtual void print_stats(std::ostream&) = 0;

#ifdef LIBKITFOX
    virtual void connect_cache_kitfox_proxy(KitFoxBuilder* kitfox_builder) = 0;
#endif

protected:
    SysBuilder_llp* m_sysBuilder;
};



class MCP_cache_builder : public CacheBuilder {
public:
    MCP_cache_builder(SysBuilder_llp* b) : CacheBuilder(b)
    {
    }

    unsigned get_l1_block_size() { return l2_cache_parameters.block_size; }
    unsigned get_l2_block_size() { return l2_cache_parameters.block_size; }

    int get_coh_type() { return m_COH_MSG_TYPE; }
    int get_mem_type() { return m_MEM_MSG_TYPE; }
    int get_credit_type() { return m_CREDIT_MSG_TYPE; }

#ifdef LIBKITFOX
    void connect_cache_kitfox_proxy(KitFoxBuilder* kitfox_builder) {}
#endif

protected:

    manifold::mcp_cache_namespace::cache_settings l1_cache_parameters;
    manifold::mcp_cache_namespace::cache_settings l2_cache_parameters;
    manifold::mcp_cache_namespace::L1_cache_settings l1_settings;
    manifold::mcp_cache_namespace::L2_cache_settings l2_settings;

    int m_COH_MSG_TYPE;
    int m_MEM_MSG_TYPE;
    int m_CREDIT_MSG_TYPE;
};


class MCP_lp_lls_builder : public MCP_cache_builder {
public:
  MCP_lp_lls_builder(SysBuilder_llp *b) : MCP_cache_builder(b) {}

  void read_cache_config(libconfig::Config &config);
  void create_caches(manifold::kernel::Clock &clk);
  void connect_cache_network(NetworkBuilder *);

  void set_mc_map_obj(manifold::uarch::DestMap *mc_map);

  int get_type() { return MCP_CACHE; }

  std::map<int, manifold::mcp_cache_namespace::LP_LLS_unit *> &get_cache_map() {
    return m_caches; }

    void pre_simulation();

    void print_config(std::ostream&);
    void print_stats(std::ostream&);

#ifdef LIBKITFOX
    void connect_cache_kitfox_proxy(KitFoxBuilder* kitfox_builder);
#endif

private:

    std::map<int, manifold::mcp_cache_namespace::LP_LLS_unit*> m_caches;
};




class MCP_l1l2_builder : public MCP_cache_builder {
public:
    MCP_l1l2_builder(SysBuilder_llp* b);

    void read_cache_config(libconfig::Config& config);
    void create_caches(manifold::kernel::Clock& clk);
    void connect_cache_network(NetworkBuilder*);

    void set_mc_map_obj(manifold::uarch::DestMap* mc_map);

    int get_type() { return MCP_L1L2; }

    void print_config(std::ostream&);
    void print_stats(std::ostream&);

    std::map<int,int>& get_l1_cids() { return m_l1_cids; }

#ifdef LIBKITFOX
    void connect_cache_kitfox_proxy(KitFoxBuilder* kitfox_builder);
#endif

private:

    std::map<int,int> m_l1_cids; //map node id to cid
    std::map<int,int> m_l2_cids;
};




#endif // #ifndef CACHE_CONFIG_H
