#include "McMap.h"
#include <assert.h>


namespace manifold {
namespace caffdram {


//! @param \c nodeIds  A vector of the node IDs of the memory controllers.
//! @param \c sett  The CaffDRAM settings.
CaffDramMcMap :: CaffDramMcMap(std::vector<int>& nodeIds, const Dsettings& sett) : m_nodeIds(nodeIds)
{
    assert(nodeIds.size() > 0);

    if(nodeIds.size() > 1) {
	//Determine the number of bits required to select the MC nodes.
	int mc_selector_bits = 0;
	while((0x1 << mc_selector_bits) < (int)nodeIds.size()) {
	    mc_selector_bits++;
	}

	assert((0x1 << mc_selector_bits) == nodeIds.size()); //number of MCs must be a power of 2

	m_mc_selector_mask = (0x1 << mc_selector_bits) - 1;

	//Determine the number of bits required to select channel.
	int ch_bits = 0;
	while((0x1 << ch_bits) < (int)sett.numChannels) {
	    ch_bits++;
	}

	m_mc_shift_bits = sett.channelShiftBits + ch_bits;
    }

}


//! Return the node ID of the MC for a given address.
int CaffDramMcMap :: lookup(uint64_t addr)
{
    if(m_nodeIds.size() == 1) { //if there is only one MC
	return m_nodeIds[0];
    }
    else {
	int idx = ((addr >> m_mc_shift_bits) & m_mc_selector_mask);
	assert(idx < m_nodeIds.size());
	return m_nodeIds[idx];
    }
}




} //namespace caffdram
} //namespace manifold

