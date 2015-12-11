#ifndef MANIFOLD_MCPCACHE_MESI_L2_CACHE_H
#define MANIFOLD_MCPCACHE_MESI_L2_CACHE_H

#include "L2_cache.h"

namespace manifold {
namespace mcp_cache_namespace {


class MESI_L2_cache : public L2_cache {
public:
    MESI_L2_cache (int nid, const cache_settings&, const L2_cache_settings&);

    void print_stats(std::ostream&);

};


} //namespace mcp_cache
} //namespace manifold


#endif //MANIFOLD_MCPCACHE_MESI_L2_CACHE_H
