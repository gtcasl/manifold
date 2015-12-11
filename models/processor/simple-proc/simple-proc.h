/*
 * =====================================================================================
 *
 *       Filename:  simple-proc.h
 *
 *    Description:  Implements a simple interface to qsim for injecting memory
 *    traffic into the cache. The model assumes that all compute instructions
 *    take a single cycle and inserts a bubble into the timeline of the core.
 *    All memory operations [Loads or Stores] are sent to the cache.
 *    Outstanding requests are held in a MSHR till the response comes back from
 *    the cache.
 *
 *        Version:  1.0
 *        Created:  06/26/2011 09:10:38 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Mitchelle Rasquinha
 *        Company: Georgia Institute of Technology 
 *
 * =====================================================================================
 */

#ifndef  MANIFOLD_SIMPLE_PROC_H
#define  MANIFOLD_SIMPLE_PROC_H

#include	"instruction.h"
#include	"kernel/component-decl.h"
#include	<iostream>
#include	<list>


namespace manifold {
namespace simple_proc {

typedef enum {
    SP_THREAD_READY,
    SP_THREAD_EXITED,    /*  Used to determine when the guest system has 
                            reached its termination time */
    SP_THREAD_STALLED     
} sp_thread_state;

struct SimpleProc_Settings {
    SimpleProc_Settings(unsigned cl_size, int msize=16);

    unsigned cache_line_size;
    int mshrsize; //MSHR holds outstanding loads
};


class CacheReq
{
public:
    CacheReq() {} //for deserialization
    CacheReq(uint64_t addr, bool read) : m_addr(addr), m_read(read) {}

    uint64_t get_addr() { return m_addr; }
    bool is_read() { return m_read; }
private:
    uint64_t m_addr;
    bool m_read; //true for read; false for write
};



//! @brief Base class. Subclasses differ in how instructions are fetched.
//!
class SimpleProcessor: public manifold::kernel::Component
{
    public:
        enum {PORT_CACHE=0};

        SimpleProcessor (int id, const SimpleProc_Settings&);

        virtual ~SimpleProcessor ();       

	int get_nid() { return PROCESSOR_ID; } //assuming processor_id is node id

        void tick();    // function registered to clock invoked every cycle
        void handle_cache_response(int, CacheReq* ); // invoked when the cache responds to an outstanding request

        bool is_suspended();
        bool is_exited();

	unsigned long get_cur_cycle() { return m_cur_cycle; }
        
	void print_stats(std::ostream&);

    protected:
        unsigned long m_cur_cycle;

#ifdef SIMPLE_PROC_UTEST
    public:
#else
    private:
#endif

        virtual Instruction* fetch() = 0;

        void dispatch( Instruction* );
	bool same_cache_line(paddr_t a1, paddr_t a2);

        const int PROCESSOR_ID;
        const int MAX_OUTSTANDING_CACHE_LINES; //max number of outstanding LOAD requests for different cache lines.
	                                 //once this number is reached, processor is stalled.
        sp_thread_state thread_state;

        Instruction* pending_insn;

	int cache_offset_bits;

        // for MSHR modelling
        int num_outstanding_requests;
        std::list <CacheReq*> outstanding_requests;

	//stats
        uint64_t stats_stalled_cycles;
        uint64_t stats_num_loads;
        uint64_t stats_num_stores;
        uint64_t stats_total_issued_insn;

};



} //namespace simple_proc
} //namespace manifold






#endif
