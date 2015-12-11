#ifndef MANIFOLD_MCP_CACHE_LP_LLS_UNIT_H
#define MANIFOLD_MCP_CACHE_LP_LLS_UNIT_H

#include "MESI_LLP_cache.h"
#include "MESI_LLS_cache.h"
#include "mux_demux.h"

namespace manifold {
namespace mcp_cache_namespace {


//This class represents a unit that contains an LLP_cache, an LLS_cache and a mux.
class LP_LLS_unit {
public:
    LP_LLS_unit(manifold::kernel::LpId_t lp, int nodeId, cache_settings& l1_parameters, cache_settings& l2_parameters,
                           L1_cache_settings& l1_settings, L2_cache_settings& l2_settings,
                           manifold::kernel::Clock& lp_clk, manifold::kernel::Clock& lls_clk,
			   manifold::kernel::Clock& mux_clock, int credit_type);

    manifold::kernel::CompId_t get_llp_cid() { return m_llp_cid; }
    manifold::kernel::CompId_t get_lls_cid() { return m_lls_cid; }
    manifold::kernel::CompId_t get_mux_cid() { return m_mux_cid; }
    LLP_cache* get_llp() { return m_llp; }
    LLS_cache* get_lls() { return m_lls; }
    MuxDemux* get_mux() { return m_mux; }


private:
    LLP_cache* m_llp;
    LLS_cache* m_lls;
    MuxDemux* m_mux;

    manifold::kernel::CompId_t m_llp_cid;
    manifold::kernel::CompId_t m_lls_cid;
    manifold::kernel::CompId_t m_mux_cid;
};




} // namespace mcp_cache_namespace
} //namespace manifold

#endif
