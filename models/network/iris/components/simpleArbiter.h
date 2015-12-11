#ifndef MANIFOLD_IRIS_SIMPLEARBITER_H
#define MANIFOLD_IRIS_SIMPLEARBITER_H

#include        "../interfaces/genericHeader.h"

namespace manifold {
namespace iris {

class SimpleArbiter
{
    public:
        SimpleArbiter (unsigned nch);
        virtual ~SimpleArbiter();
        bool is_requested(unsigned ch);
        bool is_empty();
        virtual void request( unsigned ch);
        virtual void clear_request( unsigned ch);
        virtual unsigned pick_winner() = 0;

#ifndef IRIS_TEST
    protected:
#endif 
        const unsigned no_channels;
        std::vector < bool > requested;
};



//! Round-robin arbiter
class RRSimpleArbiter : public SimpleArbiter {
public:
    RRSimpleArbiter(unsigned nch);

    virtual unsigned pick_winner();

private:
    unsigned last_winner;
};



//! First-come-first-serve arbiter
class FCFSSimpleArbiter : public SimpleArbiter {
public:
    FCFSSimpleArbiter(unsigned nch);

    virtual void request(unsigned ch);
    virtual unsigned pick_winner();

private:
    std::list<unsigned> m_requesters; //vcs making request
};




} //iris
} //Manifold

#endif  /*MANIFOLD_IRIS_SIMPLEARBITER_C*/
