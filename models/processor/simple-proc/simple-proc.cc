#include	"simple-proc.h"
#include	"kernel/component.h"
#include	<stdlib.h>
#include	<assert.h>

#ifdef DBG_SIMPLE_PROC
#include "kernel/manifold.h"
using namespace manifold::kernel;
#endif

using namespace std;

namespace manifold {
namespace simple_proc {

//! @param \c cl_size  cache line size; must be a power of 2
//! @param \c sack    whether or not the cache model sends back ACK for Stores.
//! @param \c msize   MSHR size; once the number of LOAD requests for different cache lines reaches this number, the processor is stalled.
SimpleProc_Settings :: SimpleProc_Settings(unsigned cl_size, int msize) :
cache_line_size(cl_size), mshrsize(msize)
{
}


SimpleProcessor::SimpleProcessor (int id, const SimpleProc_Settings& settings) :
	PROCESSOR_ID(id),
	MAX_OUTSTANDING_CACHE_LINES(settings.mshrsize)
{

	thread_state = SP_THREAD_READY;
	num_outstanding_requests = 0;

	pending_insn = 0;

	cache_offset_bits = 0;
	while((0x1 << cache_offset_bits) < (int)settings.cache_line_size) {
	    cache_offset_bits++;
	}

	//verify cache line size is a power of 2
	assert((int)settings.cache_line_size == (0x1 << cache_offset_bits));

	m_cur_cycle = 0;

	//stats
	stats_stalled_cycles = 0;
	stats_num_loads = 0;
	stats_num_stores = 0;
	stats_total_issued_insn = 0;
}


SimpleProcessor::~SimpleProcessor ()
{
	list<CacheReq*>::iterator it;

	for ( it = outstanding_requests.begin(); it != outstanding_requests.end() ; ++it ) {
		delete (*it);
	}
}




//! This is called every tick. It checks if there are spaces in the MSHRs. If so,
//! fetch the next instruction. If it is a memory instruction, sends a request to cache.
//! Otherwise do nothing.
void SimpleProcessor::tick ( void )
{
#ifdef DBG_SIMPLE_PROC
cout << "####### " << this->get_clock()->NowTicks() << " tick() ... " << endl;
#endif

	if(m_cur_cycle % 10000 == 0) {
	    if(PROCESSOR_ID == 0) {
	        cerr << "Cycle= " << m_cur_cycle << endl;
	    }
	}
        m_cur_cycle++;

        // stalled means (num_outstanding_request == max) AND there's a pending mem instruction

	//if stalled or exited, return right away
	if ( is_suspended() || is_exited()) {
	        if(is_suspended())
		    stats_stalled_cycles++;
		return;
	}


	//If there's a pending instruction, we process the pending instruction first.
	if ( pending_insn )
	{
		//If we get here, it means we're not stalled, but there is a pending instruction.
		//So num_outstanding_requests must be < max
		//This could happen like this: after being stalled (out standing requests maxed out, and
		//there's a pending instruciton), a response from cache comes back and decrements no. of
		//outstanding requests. It's not stalled anymore, but there's a pending instruction.

		assert(num_outstanding_requests < MAX_OUTSTANDING_CACHE_LINES);
		assert(pending_insn->is_memop());

		dispatch ( pending_insn );
		delete pending_insn;
		pending_insn = 0;
		stats_total_issued_insn++;
		return;
	}

	//normal operation
	Instruction* insn = fetch();
	if ( insn )
	{
		if ( insn->is_memop () )
		{
			if(num_outstanding_requests >= MAX_OUTSTANDING_CACHE_LINES)
			{
				//there mustn't be a pending_insn.
				assert(pending_insn == 0);

				pending_insn = insn;
				thread_state = SP_THREAD_STALLED;
				return; //must return here, otherwise insn will be deleted twice.
			}
			else
				dispatch ( insn );

		}
		stats_total_issued_insn++;
		delete insn;
	}
	else { //no more instruction
	    //probably should set status to exited
	}

}


//! Event handler for cache response.
void SimpleProcessor::handle_cache_response (int, CacheReq* request )
{
#ifdef DBG_SIMPLE_PROC
cout << "@@@ " << this->get_clock()->NowTicks() << " proc: handle_cache_response " << ", req= " << request
     << " addr= " << request->addr;
#endif
	if(request->is_read() == false) { //store
	    delete request; //for store response, simply delete the request.
	    return;
	}

        //remove all requests for the same cache line
	bool found = false;
	list<CacheReq*>::iterator it = outstanding_requests.begin();
	while(it != outstanding_requests.end()) {
                CacheReq* req = *it;

		if ( same_cache_line(req->get_addr(), request->get_addr()) ) {
			assert(req != request);
			delete req;
			it = outstanding_requests.erase(it);
			found = true;
		}
		else {
		        ++it;
		}
	}

	assert( found );

	//all requests for the same cache line are counted as one outstanding request
	num_outstanding_requests--; // this variable is the number of different lines that are requested
	if(thread_state == SP_THREAD_STALLED)
	    thread_state = SP_THREAD_READY;
	delete request;

}


void SimpleProcessor::dispatch ( Instruction* insn)
{
	//stats first
	if(insn->opcode == Instruction :: OpMemLd)
	{
	    stats_num_loads++;
	}
	else
	{
	    assert(insn->opcode == Instruction :: OpMemSt);
	    stats_num_stores++;
	}

        //Store always dispatch
	if(insn->opcode == Instruction :: OpMemSt) {
		CacheReq* request = new CacheReq (insn->addr, false);
		Send(PORT_CACHE, request);
	}
	else 
	{
		assert(insn->opcode == Instruction :: OpMemLd);
		assert ( num_outstanding_requests < MAX_OUTSTANDING_CACHE_LINES);

		bool mshr_hit = false;

		// Mitch: We dont want to send out dublicate requests for the same cache line. Hence store 
		// them in the outstanding_requests queue but count it as a single outstanding_request.
		for (list<CacheReq*>::iterator it = outstanding_requests.begin(); it != outstanding_requests.end(); ++it)
		{
			if ( same_cache_line((*it)->get_addr(), insn->addr) )
			{
				mshr_hit = true;
				break;
			}
		}

		CacheReq* request = new CacheReq (insn->addr, true);
		outstanding_requests.push_back(request);


		if ( !mshr_hit )
		{
			num_outstanding_requests++;
			Send( PORT_CACHE, new CacheReq(*request) ); //must create a new one since the request is to be deleted by
			//the event handler in the cache.
		}
	}//Load

}



bool SimpleProcessor::is_suspended ( void )
{
    return ( thread_state == SP_THREAD_STALLED );
}

bool SimpleProcessor::is_exited ( void )
{
    return ( thread_state == SP_THREAD_EXITED );
}


//! Check if two addresses are in the same cache line.
bool SimpleProcessor :: same_cache_line(paddr_t a1, paddr_t a2)
{
    return (a1 >> cache_offset_bits) == (a2 >> cache_offset_bits);
}





void SimpleProcessor :: print_stats(ostream& out)
{
    out << "********** SimpleProcessor " << PROCESSOR_ID << " stats **********" << endl;
    out << std::dec << "Total stalled cycles: " << stats_stalled_cycles << endl;
    out << std::dec << "Total issued instructions: " << stats_total_issued_insn << endl;
    out << "    total issued LOADs: " << stats_num_loads << endl;
    out << "    total issued STOREs: " << stats_num_stores << endl;
}





} //namespace simple_proc
} //namespace manifold


