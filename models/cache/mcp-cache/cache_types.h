#ifndef MANIFOLD_MCPCACHE_CACHE_TYPES_H
#define MANIFOLD_MCPCACHE_CACHE_TYPES_H

#include <stdint.h>

namespace manifold {
namespace mcp_cache_namespace {

typedef enum {
   RP_LRU = 1
} replacement_policy_t;

typedef struct {
   const char *name;
   int size;
   int assoc;
   int block_size;
   int hit_time;
   int lookup_time;
   replacement_policy_t replacement_policy;
} cache_settings;


} //namespace mcp_cache_namespace
} //namespace manifold

#endif //MANIFOLD_MCPCACHE_CACHE_TYPES_H
