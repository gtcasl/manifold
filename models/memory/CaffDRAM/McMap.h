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

#ifdef CAFFDRAM_TEST
public:
#else
private:
#endif

    std::vector<int> m_nodeIds;
    int m_mc_shift_bits;
    uint64_t  m_mc_selector_mask;
};



} //namespace caffdram
} //namespace manifold

#endif // MANIFOLD_CAFFDRAM_MCMAP_H_
