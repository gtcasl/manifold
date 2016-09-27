#ifndef MANIFOLD_UARCH_DESTMAP_H
#define MANIFOLD_UARCH_DESTMAP_H

#include <stdint.h>
#include <assert.h>
#include <vector>

namespace manifold {
namespace uarch {

class DestMap {
public:
    virtual ~DestMap() {}
    virtual int lookup(uint64_t) = 0;
    virtual uint64_t get_local_addr(uint64_t);
};


class PageBasedMap : public manifold::uarch::DestMap {
public:
    PageBasedMap(std::vector<int>& nodes, int page_offset_bits) : m_page_offset_bits(page_offset_bits)
    {
	assert(nodes.size() > 0);
	for(unsigned i=0; i<nodes.size(); i++)
	    m_nodes.push_back(nodes[i]);

	int bits_for_mask = myLog2(nodes.size());

        m_selector_mask = (0x1 << bits_for_mask) - 1;
    }

    int lookup(uint64_t addr)
    {
        return m_nodes[m_selector_mask & (addr >> m_page_offset_bits)];
    }

    uint64_t get_local_addr(uint64_t addr)
    {
	int bits_for_mask = myLog2(m_nodes.size());

        uint64_t up_addr = addr >> (m_page_offset_bits + bits_for_mask);
        uint64_t low_addr = addr & m_page_offset_bits;
        
        return  (up_addr << m_page_offset_bits) | low_addr;
    }

private:

    static int myLog2(unsigned num)
    {
	assert(num > 0);

	int bits = 0;
	while(((unsigned)0x1 << bits) < num) {
	    bits++;
	}
	return bits;
    }

    std::vector<int> m_nodes; //target node ids
    const int m_page_offset_bits;
    uint64_t m_selector_mask;
};


} //namespace uarch
} //namespace manifold

#endif //MANIFOLD_UARCH_DESTMAP_H
