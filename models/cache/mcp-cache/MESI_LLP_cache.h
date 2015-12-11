#ifndef MANIFOLD_MCPCACHE_MESI_LLP_CACHE_H
#define MANIFOLD_MCPCACHE_MESI_LLP_CACHE_H

#include "LLP_cache.h"

namespace manifold {
namespace mcp_cache_namespace {


class MESI_LLP_cache : public LLP_cache {
public:
    MESI_LLP_cache (int nid, cache_settings my_settings, const L1_cache_settings&);
    void invalidate(ClientInterface*);

    void print_stats(std::ostream&);

protected:
    void respond_with_default(Coh_msg*);

};


} //namespace mcp_cache
} //namespace manifold


#endif //MANIFOLD_MCPCACHE_MESI_LLP_CACHE_H
