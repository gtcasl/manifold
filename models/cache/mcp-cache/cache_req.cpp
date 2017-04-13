#include "cache_req.h"

using namespace std;

namespace manifold {
namespace mcp_cache_namespace {


cache_req::cache_req()
{}

const char* toString(stall_type_t type) {
  switch (type) {
  case C_INVALID_STALL:   return "INVALID_STALL";
  case C_LRU_BUSY_STALL:  return "LRU_BUSY_STALL";
  case C_TRANS_STALL:     return "TRANS_STALL";
  case C_PREV_PEND_STALL: return "PREV_PEND_STALL";
  case C_MSHR_PEND_STALL: return "MSHR_PEND_STALL";
  case C_MSHR_FULL_STALL: return "MSHR_FULL_STALL";
  case C_PREV_PART_STALL: return "PREV_PART_STALL";
  }
  abort();
  return "";
}

} //namespace mcp_cache_namespace
} //namespace manifold

