/** @file lookahead.h
 *  Contains classes and subclasses to compute lookahead
 */
 
// Mirko Stoffers, (and others) Georgia Tech, 2012

#ifndef NO_MPI

#pragma once

#include "common-defs.h"
#include <vector>
#include <map>

namespace manifold {
namespace kernel {

class Lookahead
{
  public:
    typedef enum{LA_GLOBAL, LA_PAIRWISE} LookaheadType_t;

    /**
     * Creates an object of the given subclass type
     * @param laType type of lookahead (either LA_GLOBAL or LA_PAIRWISE)
     * @return Pointer to new object
     */
    static Lookahead* Create(LookaheadType_t laType);

    /**
     * Updates the static lookahead. You can either specify src and dst for
     * pairwise lookahead or waive those parameters for global lookahead.
     * @param delay The lookahead will be decreased down to delay if not lower.
     * @param src For pairwise lookahead: source LP
     * @param dst For pairwise lookahead: destination LP
     */
    virtual void UpdateLookahead(const Time_t delay, const LpId_t src=-1, const LpId_t dst=-1)=0;

    /**
     * Returns the static lookahead. You can either specify src and dst for
     * pairwise lookahead or waive those parameters for global lookahead.
     * @param src For pairwise lookahead: source LP
     * @param dst For pairwise lookahead: destination LP
     * @return The lookahead.
     */
    virtual Time_t GetLookahead(const LpId_t src=-1, const LpId_t dst=-1)=0;

    virtual void print() {}
};

class GlobalLookahead : public Lookahead
{
  public:
    GlobalLookahead();

    /**
     * Updates the static lookahead. Parameters src and dst are ignored for
     * global lookahead.
     * @param delay The lookahead will be decreased down to delay if not lower.
     * @param src ignored
     * @param dst ignored
     */
    virtual void UpdateLookahead(const Time_t delay,const LpId_t src=-1,const LpId_t dst=-1);


    /**
     * Returns the static lookahead. Parameters src and dst are ignored for
     * global lookahead.
     * @param delay The lookahead will be decreased down to delay if not lower.
     * @param src ignored
     * @param dst ignored
     */
    virtual Time_t GetLookahead(const LpId_t src=-1, const LpId_t dst=-1);

    void print();

  private:
    // The global lookahead.
    Time_t m_lookahead;
};





//! @class PairwiseLookahead lookahead.h
class PairwiseLookahead : public Lookahead
{
public:
    virtual void UpdateLookahead(const Time_t delay, const LpId_t src=-1, const LpId_t dst=-1);
    virtual Time_t GetLookahead(const LpId_t src=-1, const LpId_t dst=-1);
    void print();
private:
    std::vector<std::map<int, Time_t> > m_lookaheads;
};




} //kernel
} //manifold

#endif //!NO_MPI
