#ifndef MANIFOLD_MCPCACHE_LLP_CACHE_H
#define MANIFOLD_MCPCACHE_LLP_CACHE_H

#include "L1_cache.h"



namespace manifold {
namespace mcp_cache_namespace {


class MuxDemux;

class LLP_cache : public L1_cache {
public:
    friend class MuxDemux;

    enum {PORT_LOCAL_L2=2};

    enum {LLP_ID=234, LLS_ID};

    LLP_cache (int nid, const cache_settings&, const L1_cache_settings&);
    virtual ~LLP_cache (void);

    void set_mux(MuxDemux* m)
    {
	assert(m_mux == 0);
        m_mux = m;
    }

    void handle_local_LLS_request (int, Coh_msg *request);

    void tick();

    void send_msg_to_peer_or_l2(Coh_msg*);

    void print_stats(std::ostream&);

    bool for_local_lls(paddr_t addr) { return l2_map->lookup(addr) == node_id; }

    manifold::uarch::NetworkPacket* pop_from_output_buffer();

private:
    MuxDemux* m_mux;

    void add_to_output_buffer(manifold::uarch::NetworkPacket* pkt);

#ifdef MCP_CACHE_UTEST
public:
#else
protected:
#endif

    virtual void try_send();
    virtual void send_credit_downstream();

    std::list<Coh_msg*> m_lls_requests;
};


} //namespace mcp_cache
} //namespace manifold


#endif //MANIFOLD_MCPCACHE_LLP_CACHE_H
