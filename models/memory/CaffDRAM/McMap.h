#ifndef MANIFOLD_CAFFDRAM_MCMAP_H_
#define MANIFOLD_CAFFDRAM_MCMAP_H_

#include "uarch/DestMap.h"
#include "Dsettings.h"
#include <vector>

namespace manifold {
namespace caffdram {


//! This memory controller map
class CaffDramMcMap : public manifold::uarch::DestMap {
public:
    CaffDramMcMap(std::vector<int>& nodeIds, const Dsettings&);

    int lookup(uint64_t addr);

    uint64_t get_local_addr(uint64_t);
    uint64_t get_global_addr(uint64_t addr, uint64_t idx) { return addr; }
    int get_page_offset_bits(void) {return 0; }

#ifdef CAFFDRAM_TEST
public:
#else
private:
#endif

    std::vector<int> m_nodeIds;
    int m_mc_shift_bits;
    int mc_selector_bits;
    uint64_t  m_mc_selector_mask;
};



} //namespace caffdram
} //namespace manifold

#endif // MANIFOLD_CAFFDRAM_MCMAP_H_
