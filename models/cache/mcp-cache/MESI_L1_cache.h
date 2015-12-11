#ifndef MANIFOLD_MCPCACHE_MESI_L1_CACHE_H
#define MANIFOLD_MCPCACHE_MESI_L1_CACHE_H

#include "L1_cache.h"

namespace manifold {
namespace mcp_cache_namespace {


class MESI_L1_cache : public L1_cache {
public:
    MESI_L1_cache (int nid, cache_settings my_settings, const L1_cache_settings&);
    void invalidate(ClientInterface*);

    void print_stats(std::ostream& out);

protected:
    void respond_with_default(Coh_msg*);

};


} //namespace mcp_cache
} //namespace manifold


#endif //MANIFOLD_MCPCACHE_MESI_L1_CACHE_H
