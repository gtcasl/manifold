// Mirko Stoffers, (and others) Georgia Tech, 2012

#ifndef NO_MPI

#include <iostream>
#include <assert.h>

#include "lookahead.h"

namespace manifold {
namespace kernel {

Lookahead* Lookahead::Create(Lookahead::LookaheadType_t laType)
{
  switch(laType)
  {
    case LA_GLOBAL:
      return new GlobalLookahead();
    case LA_PAIRWISE:
      return new PairwiseLookahead();
  }
}

GlobalLookahead::GlobalLookahead()
{
  // We initiliaze the lookahead with inf and decrease it for every connection.
  m_lookahead = INFINITY;
}

void GlobalLookahead::UpdateLookahead(const Time_t delay,const LpId_t src,const LpId_t dst)
{
  m_lookahead = (delay<m_lookahead)? delay : m_lookahead;
}

Time_t GlobalLookahead::GetLookahead(const LpId_t src,const LpId_t dst)
{
  return m_lookahead;
}

void GlobalLookahead :: print()
{
    std::cout << "Global lookahead: " << m_lookahead << std::endl;
}







void PairwiseLookahead::UpdateLookahead(const Time_t lookahead, const LpId_t src, const LpId_t dst)
{
    assert(src >= 0 && dst >= 0 && src != dst);

    while(m_lookaheads.size() <= src) //expand vector if necessary
        m_lookaheads.push_back(std::map<int, Time_t>());

    m_lookaheads[src][dst] = lookahead;
}


Time_t PairwiseLookahead::GetLookahead(const LpId_t src, const LpId_t dst)
{
    assert(src >= 0 && dst >= 0 && src != dst);

    return m_lookaheads[src][dst];
}


void PairwiseLookahead :: print()
{
    std::cout << "Pairwise lookahead:\n";
    for(int i=0; i<m_lookaheads.size(); i++) {
        std::cout << i << ": ";
	for(std::map<int, Time_t>::iterator it=m_lookaheads[i].begin(); it != m_lookaheads[i].end(); ++it) {
	    std::cout << it->first << "-" << it->second << "  ";
	}
	std::cout << std::endl;
    }
}



} //kernel
} //manifold



#endif //!NO_MPI
