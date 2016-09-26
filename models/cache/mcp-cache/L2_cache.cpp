#include <assert.h>
#include "L2_cache.h"
#include "kernel/manifold.h"
#include "debug.h"

using namespace manifold::uarch;
using namespace manifold::kernel;


namespace manifold { 
namespace mcp_cache_namespace { 


int L2_cache :: COH_MSG = -1;
int L2_cache :: MEM_MSG = -1;;
int L2_cache :: CREDIT_MSG = -1;

L2_cache :: L2_cache (int nid, const cache_settings& parameters, const L2_cache_settings& settings) :
      DOWNSTREAM_FULL_CREDITS(settings.downstream_credits)
{
    assert(COH_MSG != -1 && MEM_MSG != -1 && CREDIT_MSG != -1);

    node_id = nid;
    this->mc_map = settings.mc_map;
    this->m_downstream_credits = settings.downstream_credits;


    stalled_client_req_buffer.clear();    
    //TODO: reimplement with hierarchies
    //stalled_peer_req_buffer.clear();    

    this->my_table = new hash_table (parameters);

    //mshr is created as a fully associated hash table.
    cache_settings mshr_settings = parameters;
    mshr_settings.assoc = settings.mshr_sz;
    mshr_settings.size = mshr_settings.assoc * mshr_settings.block_size; //1 set.

    this->mshr = new hash_table (mshr_settings);

    mcp_stalled_req.resize(my_table->get_num_entries());
    for(unsigned i=0; i<mcp_stalled_req.size(); i++)
        mcp_stalled_req[i] = 0;

    //stats
    stats_num_reqs = 0;
    stats_miss = 0;
    stats_MSHR_STALLs = 0;
    stats_PREV_PEND_STALLs = 0;
    stats_LRU_BUSY_STALLs = 0;
    stats_TRANS_STALLs = 0;
    stats_stall_buffer_max_size = 0;
    stats_table_occupancy = 0;
    stats_table_empty_cycles = 0;
    stats_mshr_occupancy = 0;
    stats_mshr_empty_cycles = 0;
    stats_read_mem = 0;
    stats_dirty_to_mem = 0;
}


L2_cache::~L2_cache (void)
{
   delete my_table;
   delete mshr;
}

    /** 
     Sequence is as follows:
       Seperate lower requests from upper coherence-specific peer messages
       For lower client requests
         Check to see if block exists in the hashtable already with appropriate permission
         If so, reply with appropriate response by calling manager protocol grant methods
         else, start the client-get-permissions sequence
       For Upper Manager requests
         Check to see if block exists
           if not, do 'default' logic
         Check to see if we can process (we're nontransient)
         Do as requested by manager
         execute demand_response method
       For everything else, must be protocol specific, so send to client
    */



//====================================================================
//====================================================================
#if 0
void L2_cache :: handle_incoming(int, Coh_mem_req* request)
{
#ifdef DBG_MCP_CACHE_L2_CACHE
cout << "\n######## L2_cache node= " << node_id << " @ " << manifold::kernel::Manifold::NowTicks() << " handle_incoming(), srcID= " << request->get_src() << " type= " << request->type << " addr=(" <<hex<< request->u.mem.addr << " / " << request->u.coh.addr <<dec<< ")" << endl;
#endif
    if(request->type == Coh_mem_req :: MEM_RPLY) {
        process_mem_resp(request);
    }
    else if(request->type == Coh_mem_req :: COH_REQ) {
	//stats
	if(stalled_client_req_buffer.size() > stats_stall_buffer_max_size)
	    stats_stall_buffer_max_size = stalled_client_req_buffer.size();

        process_client_request(request, true);
    }
    else {
        assert(request->type == Coh_mem_req :: COH_RPLY);
        process_client_reply(request);
    }
}
#endif


//====================================================================
//====================================================================
void L2_cache :: process_incoming_coh(Coh_msg* request)
{
    DBG_L2_CACHE_TICK_ID(cout, "######  handle_incoming()_coh, srcID= " << request->src_id << " type= " << int(request->type) << " addr=(" <<hex<< request->addr <<dec<< ")" << endl);

    if(request->type == Coh_msg :: COH_REQ) {
        stats_num_reqs++;
	//stats
	if(stalled_client_req_buffer.size() > stats_stall_buffer_max_size)
	    stats_stall_buffer_max_size = stalled_client_req_buffer.size();

        process_client_request(request, true);
    }
    else {
        assert(request->type == Coh_msg :: COH_RPLY);
        process_client_reply(request);
    }
}



//====================================================================
//====================================================================
//! process response from memory
void L2_cache :: process_mem_resp (Mem_msg *request)
{
    DBG_L2_CACHE_TICK_ID(cout,  "######## process_mem_resp(), addr= " << hex << request->addr << dec << endl);


    if(request->op_type == OpMemLd) {
	hash_entry* mshr_entry = mshr->get_entry(request->addr);
	assert(mshr_entry);

	ManagerInterface* manager = managers[mshr_map[mshr_entry->get_idx()]->get_idx()];
	Coh_msg* stalled_req = mcp_stalled_req[manager->getManagerID()];
	mcp_stalled_req[manager->getManagerID()] = 0;

	mshr_entry->set_have_data(true);
	L2_algorithm(mshr_entry, stalled_req);
    }
    else {
	assert(request->op_type == OpMemSt);
	//do nothing for now.
    }

    delete request;
}


//====================================================================
//====================================================================
void L2_cache::process_client_request (Coh_msg* request, bool first)
{
    DBG_L2_CACHE_ID(cout, "process_client_request(), addr= " << hex << request->addr << dec << endl);


    /** This code may be modified in the future to enable parallel events while transient (read while waiting for previous read-unblock).  For the time being, just assume no parallel events for a single address */
    if (mshr->has_match(request->addr)) {
	DBG_L2_CACHE_ID(cout, "    PREV_PEND_STALL, addr= " << hex << request->addr << dec << endl);

	assert(first);
        stall (request, C_PREV_PEND_STALL);
	stats_PREV_PEND_STALLs++;
        return;
    }


    // IMPORTANT: it's important to also check the stall buffer for requests for the same line.
    if(first) {
	if(stall_buffer_has_match(request->addr)) {
	    stall (request, C_PREV_PEND_STALL);
	    stats_PREV_PEND_STALLs++;
	    return;
	}
    }



    /** Assumption; all requests require an MSHR, even those that have enough permissions to hit (because of potential for transient while waiting for unblock messages)*/
    hash_entry* mshr_entry = mshr->reserve_block_for(request->addr);

    if (mshr_entry == 0)
    {
	DBG_L2_CACHE_ID(cout, "  MSHR_STALL, addr= " << hex << request->addr << dec << endl);

	assert(first);
        stall (request, C_MSHR_STALL);
	stats_MSHR_STALLs++;
        return;
    }
   

    mshr_map[mshr_entry->get_idx()] = 0;
    hash_entry* hash_table_entry = 0;



    /** Miss logic. */
    if (!my_table->has_match (request->addr))
    {
	DBG_L2_CACHE(cout, "    L2_cache: request is miss.\n");

	//first check if request is an invalidation request; a missed invalidation request should
	//be ignored.
	if(managers[0]->is_invalidation_request(request)) {
	    DBG_L2_CACHE(cout, "    L2_cache: a missed invalidation request; ignore it.\n");

	    mshr_entry->invalidate();
	    delete request;
	    return;
	}


        /** Check if an invalid block exists already or the LRU block can begin eviction. */
	hash_table_entry = my_table->reserve_block_for (request->addr);

        if (hash_table_entry != 0)
        {
	    mshr_map[mshr_entry->get_idx()] = hash_table_entry;
            L2_algorithm (mshr_entry, request);
        } 
        else { //no hash entry available, eviction required.
	    hash_entry* victim = my_table->get_replacement_entry(request->addr);
	    ManagerInterface* victim_manager = managers[victim->get_idx()];


	    if (victim_manager->req_pending() == false && mcp_stalled_req[victim_manager->getManagerID()] == 0)
	    {
	    //The 2nd part of the if statement is necessary because a manager could be in I state and waiting for
	    //data from memory.

	    DBG_L2_CACHE(cout, "    L2_cache start replacement, victim line addr= " <<hex<< victim->get_line_addr() <<dec<< "\n");

		assert(mshr->has_match(victim->get_line_addr()) == false); //victim shouldn't have an mshr entry.

		start_eviction(victim_manager, request);
	    }
	    else
	    {
		//the client for the victim is in transient, so it must have a stalled request in the
		//stall buffer; we cannot overwrite with this to-be-stalled request. The easiest solution is
		//for the request to give up the MSHR, and wait for wakeup when the victim's request is
		//finished and it releases its mshr.
		DBG_L2_CACHE(cout, "    LRU BUSY_STALL waiting for " <<hex<< my_table->get_replacement_entry(request->addr)->get_line_addr() <<dec<< "\n");

		assert(first);
		mshr_entry->invalidate();
		stall (request, C_LRU_BUSY_STALL);
		stats_LRU_BUSY_STALLs++;
		return;
	    }
        }//eviction
    }
    /** Hit and Partial-hit logic (block exists, but coherence miss). */
    else
    {
	DBG_L2_CACHE(cout, "    L2_cache :: process_client_request(), request hits in hash table.\n");

	hash_table_entry = my_table->get_entry(request->addr);


        /** entry for this address exists, now we need to consider what state it's in */ 
        if(managers[hash_table_entry->get_idx()]->req_pending() == true)
        {
	    //One possibility is the entry is being evicted.

	    //Treat TRANS_STALL the same way as LRU_BUSY_STALL. That is, we free the mshr and
	    //wait for wakeup when the victim finishes.
	    //##### NOTE #####: here we must give up the MSHR entry; otherwise, when response for the
	    //eviction comes back, it will get this MSHR entry because it's a hit on the victim.
	    //The request waiting for the eviction will not be processed.
	    DBG_L2_CACHE(cout, "    L2_cache: TRANS_STALL\n");

	    assert(first);
	    mshr_entry->invalidate();
            stall (request, C_TRANS_STALL);
	    stats_TRANS_STALLs++;
            return;
        }
    
	mshr_map[mshr_entry->get_idx()] = hash_table_entry;

        /** We need to bring the MSHR up to speed w.r.t. state so the req can be processed. */
        /** So we copy the block over */
	update_hash_entry(mshr_entry, hash_table_entry);

        L2_algorithm (mshr_entry, request);
    }
}



//====================================================================
//! This is called the first time a request from lower client is processed.
//====================================================================
void L2_cache::L2_algorithm (hash_entry* mshr_entry, Coh_msg* request)
{
    DBG_L2_CACHE(cout, "    L2_cache :: L2_algorithm(), addr= " << hex << request->addr << dec << endl);

    ManagerInterface* manager = managers[mshr_map[mshr_entry->get_idx()]->get_idx()];
    assert(mcp_stalled_req[manager->getManagerID()] == 0);

    if(mshr_entry->get_have_data())
    {
        if(manager->process_lower_client_request(request, true)) {
	    if(my_table->get_entry(request->addr)) {
		//my_table->update_lru(request->u.coh.addr);
	    }
	    delete request;
	    update_hash_entry(mshr_map[mshr_entry->get_idx()], mshr_entry); //write hash_entry back.
	    release_mshr_entry(mshr_entry);
	}
	else {
	    mcp_stalled_req[manager->getManagerID()] = request;
	}
    }
    else
    {
        stats_miss++;
        get_from_memory(request);
	mcp_stalled_req[manager->getManagerID()] = request; //necessary for wakeup when response comes back.
    }
}



//====================================================================
//! @param \c manager  Victim's manager.
//====================================================================
void L2_cache :: start_eviction(ManagerInterface* manager, Coh_msg* request)
{
    DBG_L2_CACHE(cout, "    start_eviction().\n");

    manager->Evict();
    //is it possible the line is evicted immediately?????? Assuming that's not possible.
    mcp_stalled_req[manager->getManagerID()] = request;
}





//====================================================================
//====================================================================
void L2_cache :: process_client_reply (Coh_msg* reply)
{
    DBG_L2_CACHE_ID(cout,  "process_client_reply(), addr= " << hex << reply->addr << dec << endl);

    hash_entry* mshr_entry = mshr->get_entry(reply->addr);

    if(mshr_entry) { //this is a response to a request

	DBG_L2_CACHE(cout,  "    mshr found\n");

	ManagerInterface* manager = managers[mshr_map[mshr_entry->get_idx()]->get_idx()];
	manager->process_lower_client_reply(reply);

	if(manager->req_pending() == false) {
	    m_notify(manager);
	}
    }
    else { //no mshr exists for this reply; this could happen, for example, when a line is being evicted.
        //There must be a hash table entry.
	DBG_L2_CACHE(cout,  "    no mshr\n");

	hash_entry* hash_table_entry = my_table->get_entry(reply->addr);
	assert(hash_table_entry);

	ManagerInterface* manager = managers[hash_table_entry->get_idx()];
	manager->process_lower_client_reply(reply);

	if(manager->req_pending() == false) {
	    m_evict_notify(manager);
	}
    }
    delete reply;
}




//====================================================================
//! This is called when a reply from lower client brings the manager to a stable state,
//! while processing a request from lower client. This allows us to check if the transaction
//! is complete.
//====================================================================
void L2_cache :: m_notify(ManagerInterface* manager)
{
    DBG_L2_CACHE(cout,  "    m_notify\n");

    Coh_msg* req = mcp_stalled_req[manager->getManagerID()];
    assert(req);

    if(manager->process_lower_client_request(req, false)) { //process the request again.
	if(my_table->get_entry(req->addr)) {
	    //my_table->update_lru(req->u.coh.addr);
        }

	delete mcp_stalled_req[manager->getManagerID()];
	mcp_stalled_req[manager->getManagerID()] = 0;

	hash_entry* mshr_entry = mshr->get_entry(req->addr);

	update_hash_entry(mshr_map[mshr_entry->get_idx()], mshr_entry); //write hash_entry back.
	//mshr_map[mshr_entry->get_idx()] = 0; //cannot do this because the mapping is still used
					       //inside release_mshr_entry().
	release_mshr_entry(mshr_entry);
    }
    else {
        //do nothing since the request is already saved in mcp_stalled_req[].
    }
}



//====================================================================
//! This is called when a reply from lower client brings the manager to a stable state,
//! while processing an eviction of an L2 line. This allows us to check if the eviction
//! is complete.
//====================================================================
void L2_cache :: m_evict_notify(ManagerInterface* manager)
{
    DBG_L2_CACHE(cout,  "    m_evict_notify\n");

    Coh_msg* req = mcp_stalled_req[manager->getManagerID()];
    assert(req);
    DBG_L2_CACHE(cout, "    There is a request waiting for eviction to finish, req= " << req << " src= " << req->src_id << " msg= " << req->msg << " addr= " <<hex<< req->addr <<dec<< "\n");

    mcp_stalled_req[manager->getManagerID()] = 0;
    //now we should be able to occupy the evicted entry.
    hash_entry* mshr_entry = mshr->get_entry(req->addr);
    assert(mshr_entry);

    hash_entry* hash_table_entry = my_table->reserve_block_for (req->addr);
    assert(hash_table_entry);

    mshr_map[mshr_entry->get_idx()] = hash_table_entry;
    L2_algorithm (mshr_entry, req);
}





void L2_cache::stall (Coh_msg *request, stall_type_t stall_msg)
{
    //stalled_client_req_buffer.push_back (std::make_pair (request, stall_msg));
    Stall_buffer_entry e;
    e.req = request;
    e.type = stall_msg;
    e.time = manifold::kernel::Manifold::NowTicks();
    stalled_client_req_buffer.push_back (e);
}


bool L2_cache :: stall_buffer_has_match(paddr_t addr)
{
    paddr_t line_addr = my_table->get_line_addr(addr);

    for(std::list<Stall_buffer_entry>::iterator it= stalled_client_req_buffer.begin(); it != stalled_client_req_buffer.end(); ++it) {
	if( my_table->get_line_addr((*it).req->addr) == line_addr)
	    return true;
    }
    return false;
}



void L2_cache::get_from_memory (Coh_msg *request)
{
    Mem_msg req;
    req.type = Mem_msg :: MEM_REQ;
    req.addr = request->addr;
    req.op_type = OpMemLd;
    req.src_id = node_id;
    req.dst_id = mc_map->lookup(request->addr);

    NetworkPacket* pkt = new NetworkPacket;
    pkt->type = MEM_MSG;
    pkt->src = node_id;
    pkt->dst = req.dst_id;
    pkt->dst_port = manifold::uarch::MEM_ID;
    *((Mem_msg*)(pkt->data)) = req;
    pkt->data_size = sizeof(Mem_msg);

    DBG_L2_CACHE_ID(cout,  " get from memory node " << req.dst_id << " for 0x" << hex << req.addr << dec << endl);

    assert(my_table->get_lookup_time() > 0);

    Clock* clk = m_clk;
    if(m_clk == 0)
        clk = &Clock::Master();

    manifold::kernel::Manifold::ScheduleClock(my_table->get_lookup_time(), *clk, &L2_cache::send_msg_after_lookup_time, this, pkt);

    stats_read_mem++;
}


void L2_cache::dirty_to_memory (paddr_t addr)
{
    DBG_L2_CACHE_ID(cout,  " dirty write to memory for 0x" << hex << addr << dec << endl);

    Mem_msg req;
    req.type = Mem_msg :: MEM_REQ;
    req.addr = addr;
    req.op_type = OpMemSt;
    req.src_id = node_id;
    req.dst_id = mc_map->lookup(addr);

    NetworkPacket* pkt = new NetworkPacket;
    pkt->type = MEM_MSG;
    pkt->src = node_id;
    pkt->dst = req.dst_id;
    pkt->dst_port = manifold::uarch::MEM_ID;
    *((Mem_msg*)(pkt->data)) = req;
    pkt->data_size = sizeof(Mem_msg);

    Clock* clk = m_clk;
    if(m_clk == 0)
        clk = &Clock::Master();

    manifold::kernel::Manifold::ScheduleClock(my_table->get_lookup_time(), *clk, &L2_cache::send_msg_after_lookup_time, this, pkt);

    stats_dirty_to_mem++;
}



void L2_cache :: update_hash_entry(hash_entry* e1, hash_entry* e2)
{
    e1->set_have_data(e2->get_have_data());
}


void L2_cache :: send_msg_to_l1(Coh_msg* msg)
{
    msg->src_id = node_id;

    NetworkPacket* pkt = new NetworkPacket;
    pkt->type = COH_MSG;
    pkt->src = node_id;
    pkt->dst = msg->dst_id;
    *((Coh_msg*)(pkt->data)) = *msg;
    pkt->data_size = sizeof(Coh_msg);

    DBG_L2_CACHE_ID(cout,  " sending msg= " << msg->msg << " to L1 node= " << msg->dst_id << " fwd= " << msg->forward_id << endl);

    delete msg;

    Clock* clk = m_clk;
    if(m_clk == 0)
        clk = &Clock::Master();

    manifold::kernel::Manifold::ScheduleClock(my_table->get_lookup_time(), *clk, &L2_cache::send_msg_after_lookup_time, this, pkt);
}


//! This function is scheduled by send_msg_to_l1(), get_from_memory(), and dirty_to_memory()
//! to send the message after a delay of lookup_time
void L2_cache :: send_msg_after_lookup_time(NetworkPacket* pkt)
{
    m_downstream_output_buffer.push_back(pkt);
    try_send();
}


void L2_cache :: try_send()
{
    if(!m_downstream_output_buffer.empty() && m_downstream_credits > 0) {
	NetworkPacket* pkt = m_downstream_output_buffer.front();
	m_downstream_output_buffer.pop_front();
        Send(PORT_L1, pkt);
        m_downstream_credits--;
    }
}


void L2_cache :: schedule_send_credit()
{
    Clock* clk = m_clk;
    if(m_clk == 0)
        clk = &Clock::Master();

    manifold::kernel::Manifold::ScheduleClock(1, *clk, &L2_cache::send_credit_downstream, this);
    #ifdef FORECAST_NULL
    m_credit_out_ticks.push_back(clk->NowTicks() + 1);
    #endif

}


void L2_cache :: send_credit_downstream()
{
    DBG_L2_CACHE_ID(cout, " sending credit.\n");

    NetworkPacket* pkt = new NetworkPacket;
    pkt->type = CREDIT_MSG;
    Send(PORT_L1, pkt);
}


void L2_cache :: client_writeback(ManagerInterface* mgr)
{
    DBG_L2_CACHE_ID(cout, " client write back.\n");

    hash_entries[mgr->getManagerID()]->set_dirty(true);
}


void L2_cache :: invalidate (ManagerInterface* mgr)
{
    DBG_L2_CACHE_ID(cout,  " invalidating " << hash_entries[mgr->getManagerID()] << " id= " << hash_entries[mgr->getManagerID()]->get_idx() << endl);

    hash_entry* he = hash_entries[mgr->getManagerID()];
    if(he->is_dirty())
        dirty_to_memory(he->get_line_addr());
    he->invalidate();

}


//! The function release_mshr_entry() is only called when a reply from a client
//! comes back and the transaction is complete. In some situations, however, no message
//! needs to be sent to the client, but MSHR entry is already allocated. In these cases,
//! manager would call ignore(), allowing us to call release_mshr_entry().
// One such case is with MESI, as follows:
//
//    C0              M               C1
//     |------1------>|                |
//     |              |---2---  ---3---|  
//     |              |       \/       |
//     |              |       /\       |
//     |              |<------  ------>|
//
// 1. Client C0 sends I_to_S to LOAD a missed line.
// 2. The line is in E state, so manager M sends FWD_S to owner C1.
// 3. Before 2 reaches C1, the line is being evicted in C1, so it
//    sends CM_E_to_I to M. 
// 4. When E_to_I from C1 reaches M, it PREV_PEND_STALLs since an
//    mshr entry for the same line already exists.
// 5. C1 receives FWD_S in the EI state. It processes it as if it were
//    in the E state: it sends CC_S_DATA to C0, and CM_CLEAN to M.
//    Then it silently enters I state.
// 6. M gets CM_CLEAN from C1, and UNBLOCK_S from C0 and enters S state.
// 7. Now M, in S state, processes E_to_I in step 3.
//    Since C1 is already in I state, there's nothing to be done, so M
//    calls ignore() in case the owning object needs cleanup.

void L2_cache :: ignore(ManagerInterface* mgr)
{
    DBG_L2_CACHE_ID(cout,  " ignore a request.\n");
}




void L2_cache :: release_mshr_entry(hash_entry* mshr_entry)
{
    mshr_map[mshr_entry->get_idx()] = 0;
    mshr_entry->invalidate();


    //check the stall buffer and see if any request waiting for mshr
    //std::list<std::pair <Coh_mem_req *, stall_type_t> >::iterator it = stalled_client_req_buffer.begin();
    std::list<Stall_buffer_entry>::iterator it = stalled_client_req_buffer.begin();

    Coh_msg* req = 0;
    manifold::kernel::Ticks_t stall_time;
    stall_type_t stallType; //for debug

    bool found = false; //if there's a stalled request to wake up
    while(it != stalled_client_req_buffer.end()) {
        //Wake up a stalled request only if it will not PREV_PEND_STALL, LRU_BUSY_STALL, or TRANS_STALL.
	//It canno MSHR_STALL because we are releasing an MSHR entry only a few lines ago.
	if (!mshr->has_match((*it).req->addr)) { //won't PREV_PEND
	    if(!my_table->has_match ((*it).req->addr)) { //going to miss
		ManagerInterface* victim_manager = managers[my_table->get_replacement_entry ((*it).req->addr)->get_idx()];
		if (victim_manager->req_pending() == false && mcp_stalled_req[victim_manager->getManagerID()] == 0) {
		    found = true;
		}
	    }
	    else { //hit
		if (managers[my_table->get_entry((*it).req->addr)->get_idx()]->req_pending() == false) { //won't TRANS
		    found = true;
		}
	    }
	}

	if(found) {
	    req = (*it).req;
	    stall_time = (*it).time;
	    stalled_client_req_buffer.erase(it);
	    break;
	}

	++it;
    }

    if(req != 0) {
        //do a sanity check
	paddr_t line_addr = my_table->get_line_addr(req->addr);
	for(std::list<Stall_buffer_entry>::iterator it = stalled_client_req_buffer.begin(); it != stalled_client_req_buffer.end(); ++it) {
	    if(my_table->get_line_addr((*it).req->addr) == line_addr) {
	        assert(stall_time <= (*it).time); //if a stalled request is for the same line, then it must be stalled later than the one
		                                  //being released.
	    }
	}

	DBG_L2_CACHE_ID(cout,  " wakeup req, stall type= " << stallType << " req= " << req << " msg= " << req->msg << " addr= " <<hex<< req->addr <<dec << " src= " << req->src_id << "\n");

	process_client_request(req, false);
    }
}


void L2_cache :: print_stats(std::ostream& out)
{
    out << "L2 node " << node_id << endl
        << "    Num of reqs= " << stats_num_reqs << " misses= " << stats_miss << "  miss rate= " << (double)stats_miss/stats_num_reqs << endl
        << "    Stall buffer max size= " << stats_stall_buffer_max_size << endl
        << "    MSHR_STALL = " << stats_MSHR_STALLs << endl
        << "    PREV_PEND_STALL = " << stats_PREV_PEND_STALLs << endl
        << "    PREV_LRU_BUSY_STALL = " << stats_LRU_BUSY_STALLs << endl
        << "    PREV_TRANS_STALL = " << stats_TRANS_STALLs << endl
	<< "    mshr occupancy = " << stats_mshr_occupancy << endl
	<< "    mshr empty cycles= " << stats_mshr_empty_cycles << endl;
    if(stats_cycles == 0) {
        out << "    hash table occupancy = " << stats_table_occupancy << endl;
    }
    else {
	double avg_occup = (double)stats_table_occupancy / stats_cycles;

	out << "    cycles = " << stats_cycles << endl
	    << "    hash table avg occupancy = " << avg_occup << " (" << avg_occup / (my_table->get_size() / my_table->get_block_size()) * 100 << "%)" << endl
	    << "    hash table empty cycles= " << stats_table_empty_cycles << endl;
    }

    out << "    read mem= " << stats_read_mem << endl
        << "    dirty to mem= " << stats_dirty_to_mem << endl;
}


//####################################################################
// for debug
//####################################################################
void L2_cache :: print_mshr()
{
    mshr->dbg_print(cerr);
}

void L2_cache :: print_stall_buffer()
{
    cerr << "Stall buffer:\n";
    for(std::list<Stall_buffer_entry>::iterator it= stalled_client_req_buffer.begin(); it != stalled_client_req_buffer.end(); ++it) {
    cerr <<  " (" <<hex<< (*it).req->addr <<dec<< ") (" << (*it).type << ")" << "\n";
    }
    cerr << endl;
}





} // namespace mcp_cache_namespace
} // namespace manifold

