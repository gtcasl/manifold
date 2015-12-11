/*
 * =====================================================================================
 *
 *       Filename:  myFullyVirtualArbiter.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  04/27/2010 01:52:11 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Mitchelle Rasquinha (), mitchelle.rasquinha@gatech.edu
 *        Company:  Georgia Institute of Technology
 *
 * =====================================================================================
 */

#ifndef  MANIFOLD_IRIS_GENERICSWITCHARBITER_H
#define  MANIFOLD_IRIS_GENERICSWITCHARBITER_H

#include	"../interfaces/genericHeader.h"
#include	<vector>
#include	<list>
//#include	<fstream>

namespace manifold {
namespace iris {


struct SA_unit
{
    uint port;
    uint ch;
    uint64_t in_time;
    uint64_t win_cycle;
};

class GenericSwitchArbiter
{
    public:
	#ifdef IRIS_DBG
        GenericSwitchArbiter (unsigned p, unsigned v, unsigned id);       
	#else
        GenericSwitchArbiter (unsigned p, unsigned v);       
	#endif
        ~GenericSwitchArbiter();
        void clear_requestor(uint outp, uint inp, uint ovc);
        virtual void request(uint p, uint op, uint inp, uint iv);
        virtual const SA_unit* pick_winner( uint p) = 0;

        std::string toString() const;

#ifdef IRIS_TEST
    public:
#else
    protected:
#endif

        const unsigned ports;
        const unsigned vcs;
        std::vector < std::vector <bool> > requested; //ports X (ports*vcs)
        std::vector < SA_unit > last_winner;

        std::string name;
	#ifdef IRIS_DBG
        unsigned router_id;
	#endif
};


//! Round-robin switch arbiter
class RRSwitchArbiter : public GenericSwitchArbiter
{
public:
    #ifdef IRIS_DBG
    RRSwitchArbiter (unsigned p, unsigned v, unsigned id);       
    #else
    RRSwitchArbiter (unsigned p, unsigned v);       
    #endif

    virtual const SA_unit* pick_winner( uint p);

#ifdef IRIS_TEST
    public:
#else
    private:
#endif
    //const SA_unit* do_round_robin_arbitration( uint p);
    //SA_unit do_priority_round_robin_arbitration( uint p);
    //SA_unit do_fcfs_arbitration( uint p);

        std::vector < uint> last_port_winner;
};


//! First-come-first-serve switch arbiter
class FCFSSwitchArbiter : public GenericSwitchArbiter
{
public:
    #ifdef IRIS_DBG
    FCFSSwitchArbiter (unsigned p, unsigned v, unsigned id);       
    #else
    FCFSSwitchArbiter (unsigned p, unsigned v);       
    #endif

    virtual void request(uint p, uint op, uint inp, uint iv);
    virtual const SA_unit* pick_winner( uint p);

#ifdef IRIS_TEST
public:
#else
private:
#endif
    std::vector<std::list<unsigned> > m_requesters; //For each output port, a list of requesting VCs.

};


} // namespace iris
} // namespace manifold



#endif //MANIFOLD_IRIS_GENERICSWITCHARBITER_H
