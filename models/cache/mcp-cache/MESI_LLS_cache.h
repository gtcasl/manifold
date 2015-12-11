#ifndef MANIFOLD_MCPCACHE_MESI_LLS_CACHE_H
#define MANIFOLD_MCPCACHE_MESI_LLS_CACHE_H

#include "LLS_cache.h"

namespace manifold {
namespace mcp_cache_namespace {


class MESI_LLS_cache : public LLS_cache {
public:
    MESI_LLS_cache (int nid, const cache_settings&, const L2_cache_settings&);
    void print_stats(std::ostream&);
};


} //namespace mcp_cache
} //namespace manifold


#endif //MANIFOLD_MCPCACHE_MESI_LLS_CACHE_H
