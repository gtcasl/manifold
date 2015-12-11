/*
 * =====================================================================================
 *
 *       Filename:  genericVcAllocator.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  06/21/2010 03:22:20 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Mitchelle Rasquinha (), mitchelle.rasquinha@gatech.edu
 *        Company:  Georgia Institute of Technology
 *
 * =====================================================================================
 */

#ifndef  MANIFOLD_IRIS_GENERICVCALLOCATOR_H
#define  MANIFOLD_IRIS_GENERICVCALLOCATOR_H

#include	"../interfaces/genericHeader.h"
#include	<algorithm>

namespace manifold {
namespace iris {


struct VCA_unit
{
    VCA_unit(): in_port(-1), in_vc(-1), out_port(-1), out_vc(-1), is_valid(false){}
    uint in_port;
    uint in_vc;
    uint out_port;
    uint out_vc;
    bool is_valid;
};

/*!
 * =====================================================================================
 *        Class:  GenericVcAllocator
 *  Description:  \brief This class allocates an output virtual channel to all
 *  requesting input messages. Only head flits pass through VCA.
 * =====================================================================================
 */
class SimpleRouter;

class GenericVcAllocator
{
    public:
        /* ====================  LIFECYCLE     ======================================= */
        GenericVcAllocator (const SimpleRouter* r, unsigned p, unsigned v);
        virtual ~GenericVcAllocator ();
        virtual void request(uint out_port, uint out_vc, uint in_port, uint in_vc);
        virtual std::vector<VCA_unit>& pick_winner() = 0;
	void release_output_vc(unsigned port, unsigned ovc);
        std::string toString() const;        

#ifdef IRIS_TEST
    public:
#else
    protected:
#endif
	const SimpleRouter* router;
        const unsigned PORTS;
        const unsigned VCS;
        std::string name;
        std::vector < std::vector <VCA_unit> > requested; //ports X (ports*vcs)
        std::vector<std::vector<bool> > ovc_taken; //whether or not an output vc is taken: ports X vcs
        std::vector<VCA_unit> current_winners; //winners of the current call of pick_winner; could be more than 1,
	                                       //so use a vector
        //unsigned router_id;
};


//! Round-robin VC allocator
class RRVcAllocator : public GenericVcAllocator {
public:
    RRVcAllocator (const SimpleRouter* r, unsigned p, unsigned v);

    virtual std::vector<VCA_unit>& pick_winner();

#ifdef IRIS_TEST
public:
#else
private:
#endif
    std::vector < std::vector <uint> >  last_winner; // ID of input vc that win a vc allocation last time: per output vc basis: ports X vcs

};


//! First-come-first-serve allocator
class FCFSVcAllocator : public GenericVcAllocator {
public:
    FCFSVcAllocator (const SimpleRouter* r, unsigned p, unsigned v);

    virtual void request(uint out_port, uint out_vc, uint in_port, uint in_vc);
    virtual std::vector<VCA_unit>& pick_winner();

#ifdef IRIS_TEST
public:
#else
private:
#endif
    std::vector<std::list<unsigned> > m_requesters; // For each output vc, a list of input VCs that are requesting output VC.

};


} // namespace iris
} // namespace manifold



#endif //MANIFOLD_IRIS_GENERICVCALLOCATOR_H

