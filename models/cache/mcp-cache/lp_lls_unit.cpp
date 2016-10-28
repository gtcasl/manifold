#include "lp_lls_unit.h"
#include "kernel/manifold.h"
#include "kernel/clock.h"

namespace manifold {
namespace mcp_cache_namespace {

using namespace manifold::kernel;


LP_LLS_unit :: LP_LLS_unit(LpId_t lp, int nodeId, cache_settings& l1_parameters, cache_settings& l2_parameters,
                           L1_cache_settings& l1_settings, L2_cache_settings& l2_settings,
                           Clock& lp_clk, Clock& lls_clk, Clock& mux_clk, int credit_type)
{
    m_llp_cid = Component :: Create<MESI_LLP_cache>(lp, nodeId, l1_parameters, l1_settings);
    m_lls_cid = Component :: Create<MESI_LLS_cache>(lp, nodeId, l2_parameters, l2_settings);
    m_mux_cid = Component :: Create<MuxDemux>(lp, mux_clk, credit_type);

    m_llp = Component :: GetComponent<MESI_LLP_cache>(m_llp_cid);
    m_lls = Component :: GetComponent<MESI_LLS_cache>(m_lls_cid);
    m_mux = Component :: GetComponent<MuxDemux>(m_mux_cid);

    if(m_llp) {
        assert(m_lls != 0 && m_mux != 0);
        m_llp->set_mux(m_mux);
        m_lls->set_mux(m_mux);
        m_mux->set_llp_lls(m_llp, m_lls);
    }

    //connect LLP and LLS
    Manifold :: Connect(m_llp_cid, MESI_LLP_cache::PORT_LOCAL_L2, &MESI_LLP_cache::handle_local_LLS_request,
            m_lls_cid, MESI_LLS_cache::PORT_LOCAL_L1,
            &MESI_LLS_cache::handle_llp_incoming, lp_clk, lls_clk, 1, 1);

    if(m_mux) {
    Clock :: Register(lp_clk, m_llp, &LLP_cache::tick, (void(LLP_cache::*)(void))0);
    Clock :: Register(lls_clk, m_lls, &LLS_cache::tick, (void(LLS_cache::*)(void))0);
    Clock :: Register(mux_clk, m_mux, &MuxDemux::tick, (void(MuxDemux::*)(void))0);
    }
}


} // namespace mcp_cache_namespace
} //namespace manifold
