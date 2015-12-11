#include <assert.h>
#include "kernel/component.h"
#include "simple_cache.h"

#ifdef DBG_SIMPLE_CACHE
using namespace manifold::kernel;
#include "kernel/manifold.h"
#endif

using namespace std;
using namespace manifold::uarch;



// - Write-through
// - No write-allocate
// - No response for writes, unless it's L1.

// Requests are handled as follows:
// "Higher level" means a higher level cache or memory.
// "Lower level" means a lower level cache or processor.
//
// Read hit: send response to lower level immediately.
// Read miss: a request is sent to higher level. Upon response, eviction if necessary, and
//            a reply is sent to lower level.
// Write hit: if write-through, a write request is sent to higher level. If L1, a reply is sent to processor.
// Write miss: request is sent to higher level. If L1, a reply is sent to processor.
//
namespace manifold {
namespace simple_cache {

simple_cache::simple_cache(int nid, const char* name, const Simple_cache_parameters& parameters, const Simple_cache_init& init) :
    m_node_id(nid),
    m_name(name),
    m_IS_FIRST_LEVEL(init.first_level), m_IS_LAST_LEVEL(init.last_level)
{
    if(init.last_level)
        m_dest_map = init.dmap;
    else
        m_dest_map = 0;

    my_table = new hash_table(parameters.size, parameters.assoc, parameters.block_size,
                              parameters.hit_time, parameters.lookup_time);

    stats_n_loads = 0;
    stats_n_stores = 0;
    stats_n_load_hits = 0;
    stats_n_store_hits = 0;
}


simple_cache::~simple_cache (void)
{
   delete my_table;
}


void simple_cache::handle_request (int port, Cache_req *request)
{
    assert(port == PORT_LOWER);

#ifdef DBG_SIMPLE_CACHE
cout << "@@@ " << Manifold::NowTicks() << " simple cache " << m_node_id << " " << m_name << ": handle_request, " << *request << endl;
#endif
    if(request->op_type == OpMemLd) {
	stats_n_loads++;
    }
    else {
	stats_n_stores++;
    }

    bool hit = my_table->process_request (request->addr);

    if(hit) {
	if(request->op_type == OpMemSt) { //store hit
	    stats_n_store_hits++;
            #ifdef DBG_SIMPLE_CACHE
	    cout << "    cache " << m_node_id << " " << m_name << " write hit." << endl;
            #endif
	    write_hit(request);
	}
	else { //load hit
            stats_n_load_hits++;
            #ifdef DBG_SIMPLE_CACHE
	    cout << "    cache " << m_node_id << " " << m_name << " load hit." << endl;
            #endif
            reply_to_lower(request, my_table->get_hit_time());
        }
    }
    else { //miss
	#ifdef DBG_SIMPLE_CACHE
	if(request->op_type == OpMemSt)
	    cout << "    cache " << m_node_id << " " << m_name << " write miss." << endl;
	else
	    cout << "    cache " << m_node_id << " " << m_name << " load miss." << endl;
	#endif

	if(request->op_type == OpMemLd) {
	    bool prev = insert_mshr_entry(request);
	    if(prev) {
		#ifdef DBG_SIMPLE_CACHE
		cout << "    There is an entry for the same block in the MSHR." << endl;
		#endif
		return;
	    }
	}


	if(request->op_type == OpMemLd) { //load miss
	    //for load miss, the request is put in MSHR.

	    //pass request to upper level
	    if(m_IS_LAST_LEVEL) {
		Mem_req* mreq = new Mem_req (m_node_id, request->addr, request->op_type);
		mreq->dest_id = m_dest_map->lookup (request->addr);
		SendTick (PORT_UPPER, mreq, my_table->get_lookup_time());
	    }
	    else {
		Cache_req* req = new Cache_req(*request); //make a copy
		SendTick (PORT_UPPER, req, my_table->get_lookup_time()); //send to higher-level cache
	    }
	}
	else { //store miss
	    //pass request to upper level
	    if(m_IS_LAST_LEVEL) {
		Mem_req* mreq = new Mem_req (m_node_id, request->addr, request->op_type);
		mreq->dest_id = m_dest_map->lookup (request->addr);
		SendTick (PORT_UPPER, mreq, my_table->get_lookup_time());
		if(!m_IS_FIRST_LEVEL)
		    delete request;
	    }
	    else {
	        //reuse request
		Cache_req* req = request; //reuse request if not first level;
		if(m_IS_FIRST_LEVEL)
		    req = new Cache_req(*request); //make a copy

		SendTick (PORT_UPPER, req, my_table->get_lookup_time()); //send to higher-level cache
	    }

	    if(m_IS_FIRST_LEVEL) {
		reply_to_lower(request, my_table->get_lookup_time());
	    }
	}

    }
}



void simple_cache :: write_hit(Cache_req* creq)
{
    if(m_IS_LAST_LEVEL) {
	Mem_req* mreq = new Mem_req (m_node_id, creq->addr, OpMemSt);
	mreq->dest_id = m_dest_map->lookup (creq->addr);
	if(!m_IS_FIRST_LEVEL)
	    delete creq;
	SendTick (PORT_UPPER, mreq, my_table->get_hit_time());
    }
    else {
	Cache_req* req = creq;
	if(m_IS_FIRST_LEVEL)
	    req = new Cache_req(*creq); //copy if L1
	SendTick (PORT_UPPER, req, my_table->get_hit_time()); //pass to upper level
    }

    if(m_IS_FIRST_LEVEL) { //reply to processor
	reply_to_lower(creq, my_table->get_hit_time());
    }
}



//! Create an MSHR entry for the request.
//! @return True if there's already an entry for the same line in the MSHR.
bool simple_cache::insert_mshr_entry (Cache_req *request)
{
    assert(request->op_type == OpMemLd);

    bool found = false;
    list<mshr_entry *>::iterator it;
    for (it = m_mshr.begin(); it != m_mshr.end(); it++)
    {
	if (my_table->get_line_addr ((*it)->creq->addr) == my_table->get_line_addr (request->addr)) {
	    found = true;
	    break;
        }
    }

    m_mshr.push_back (new mshr_entry(request));
    return found;
}




//! Handle response from memory controller.
void simple_cache::handle_response (int port, Mem_req *mreq)
{
    assert(m_IS_LAST_LEVEL);

    #ifdef DBG_SIMPLE_CACHE
    cout << "@@@ " << Manifold::NowTicks() << " simple cache " << m_node_id << " " << m_name << ": handle_response from memory, " << *mreq << endl;
    #endif

    if(mreq->op_type == OpMemSt) //write response is ignored
        return;

    my_table->process_response (mreq->addr);

    free_mshr_entries(mreq->addr);

    delete mreq; //delete the Mem_req

}


void simple_cache::free_mshr_entries (paddr_t addr)
{
    list<mshr_entry *>::iterator it = m_mshr.begin();

    while ( it != m_mshr.end() ) {
        if (my_table->get_line_addr ((*it)->creq->addr) == my_table->get_line_addr (addr)) {
	    //For load response, we need to process replacement, MSHR, etc. Therefore, there should be a delay.
	    reply_to_lower((*it)->creq, my_table->get_hit_time());
            delete (*it);
            it = m_mshr.erase (it); //erase() returns the iterator pointing to the next element
        }
        else
            ++it;
    }
}




void simple_cache :: reply_to_lower(Cache_req* creq, int delay)
{
    #ifdef DBG_SIMPLE_CACHE
    cout << "@@@ " << Manifold::NowTicks() << " simple cache " << m_node_id << " " << m_name << ": reply to lower level " << *creq << endl;
    #endif
    if(m_IS_FIRST_LEVEL) {
        assert(creq->preqWrapper);
	creq->preqWrapper->SendTick(this, PORT_LOWER, delay);
	delete creq->preqWrapper;
	delete creq;
    }
    else {
	SendTick(PORT_LOWER, creq, delay);
    }
}



//! Handle response from upper level cache
void simple_cache::handle_cache_response (int port, Cache_req *request)
{
    assert(m_IS_LAST_LEVEL == false);

    #ifdef DBG_SIMPLE_CACHE
    cout << "@@@ " << Manifold::NowTicks() << " simple cache " << m_node_id << " " << m_name << ": handle_response from upper level cache, " << *request << endl;
    #endif

    assert(request->op_type == OpMemLd);


    my_table->process_response (request->addr);

    free_mshr_entries(request->addr);

    delete request;
}


void simple_cache :: print_stats(ostream& out)
{
    out << "********** simple-cache " << m_node_id << " stats **********" << endl;
    out << "    Total loads: " << stats_n_loads << endl;
    out << "    Total stores: " << stats_n_stores << endl;
    out << "    Load hits: " << stats_n_load_hits << "  store hits: " << stats_n_store_hits << "  hit rate: "
        << (double)(stats_n_load_hits+stats_n_store_hits)/(stats_n_loads+stats_n_stores)*100 << "%" << endl;
}



} //namespace simple_cache
} //namespace manifold

