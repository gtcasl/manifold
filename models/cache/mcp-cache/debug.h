#ifndef MANIFOLD_MCP_CACHE_DEBUG_H
#define MANIFOLD_MCP_CACHE_DEBUG_H

#ifndef MCP_DBG_FILTER
#define MCP_DBG_FILTER true
#endif

#ifdef DBG_MCP_CACHE_L1_CACHE
    #define DBG_L1_CACHE(ostr, s) \
	if (MCP_DBG_FILTER) { \
	    ostr << s; \
	}

    #define DBG_L1_CACHE_ID(ostr, s) \
	if (MCP_DBG_FILTER) { \
	    ostr << "L1 node= " << node_id << " " << s; \
	}

    #define DBG_L1_CACHE_TICK_ID(ostr, s) \
	if (MCP_DBG_FILTER) { \
	    ostr << "\n@ " << Manifold::NowTicks() << " L1 node= " << node_id << " " << s; \
	}

    #define DBG_LLP_CACHE_ID(ostr, s) \
	if (MCP_DBG_FILTER) { \
	    ostr << "LLP node= " << node_id << " " << s; \
	}

#else
    #define DBG_L1_CACHE(ostr, s)
    #define DBG_L1_CACHE_ID(ostr, s)
    #define DBG_L1_CACHE_TICK_ID(ostr, s)
    #define DBG_LLP_CACHE_ID(ostr, s)
#endif



#ifdef DBG_MCP_CACHE_L2_CACHE
    #define DBG_L2_CACHE(ostr, s) \
	if (MCP_DBG_FILTER) { \
	    ostr << s; \
	}

    #define DBG_L2_CACHE_ID(ostr, s) \
	if (MCP_DBG_FILTER) { \
	    ostr << "L2 node= " << node_id << " " << s; \
	}

    #define DBG_L2_CACHE_TICK_ID(ostr, s) \
	if (MCP_DBG_FILTER) { \
	    ostr << "\n@ " << Manifold::NowTicks() << " L2 node= " << node_id << " " << s; \
	}

    #define DBG_LLS_CACHE_ID(ostr, s) \
	if (MCP_DBG_FILTER) { \
	    ostr << "LLS node= " << node_id << " " << s; \
	}
#else
    #define DBG_L2_CACHE(ostr, s)
    #define DBG_L2_CACHE_ID(ostr, s)
    #define DBG_L2_CACHE_TICK_ID(ostr, s)
    #define DBG_LLS_CACHE_ID(ostr, s)
#endif




namespace manifold {
namespace mcp_cache_namespace {




} // namespace mcp_cache_namespace
} //namespace manifold
#endif
