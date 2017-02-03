#ifndef MANIFOLD_UARCH_DESTMAP_H
#define MANIFOLD_UARCH_DESTMAP_H

#include <stdint.h>
#include <assert.h>
#include <vector>
#include <iostream>

namespace manifold {
namespace uarch {

class DestMap {
public:
    virtual ~DestMap() {}
    virtual int lookup(uint64_t) = 0;
    virtual uint64_t get_local_addr(uint64_t) = 0;
    virtual uint64_t get_global_addr(uint64_t, uint64_t) = 0;
    virtual int get_page_offset_bits (void) = 0;
};


class PageBasedMap : public manifold::uarch::DestMap {
public:
    PageBasedMap(std::vector<int>& nodes, int page_offset_bits) : m_page_offset_bits(page_offset_bits)
    {
        assert(nodes.size() > 0);
        for(unsigned i=0; i<nodes.size(); i++)
            m_nodes.push_back(nodes[i]);

        m_selector_bits = myLog2(nodes.size());

        m_selector_mask = (0x1 << m_selector_bits) - 1;
        m_page_offset_mask = (0x1 << m_page_offset_bits) - 1;
    }

    int lookup(uint64_t addr)
    {
        int idx = m_selector_mask & (addr >> m_page_offset_bits);
        int m_id = m_nodes[idx];

        return m_id;
    }

    int get_idx(int m_id)
    {
        for (unsigned int i = 0; i < m_nodes.size(); i++) {
            if (m_nodes[i] == m_id)
                return i;
        }

        assert(0);
    }

    uint64_t get_local_addr(uint64_t addr)
    {
        uint64_t up_addr = addr >> (m_page_offset_bits + m_selector_bits);
        uint64_t low_addr = addr & m_page_offset_mask;
        uint64_t laddr = (up_addr << m_page_offset_bits) | low_addr;

        return laddr;
    }

    //fixme: assume the cache offset field is less than a page
    uint64_t get_global_addr(uint64_t addr, uint64_t m_id)
    {
        int idx = get_idx(m_id);

        uint64_t up_addr = addr >> m_page_offset_bits;
        uint64_t low_addr = (addr & m_page_offset_mask) | idx << (m_page_offset_bits);
        uint64_t gaddr = (up_addr << (m_page_offset_bits + m_selector_bits)) | low_addr;

        return  gaddr;
    }

    int get_page_offset_bits()
    {
        return m_page_offset_bits;
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
    int m_selector_bits;
    uint64_t m_selector_mask;
    uint64_t m_page_offset_mask;
};


} //namespace uarch
} //namespace manifold

#endif //MANIFOLD_UARCH_DESTMAP_H
