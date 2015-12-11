#include "L1_cache.h"
#include "kernel/manifold.h"
#include "debug.h"


#include <assert.h>
#include <iostream>


using namespace std;
using namespace manifold :: kernel;
using namespace manifold :: uarch;


namespace manifold { 
namespace mcp_cache_namespace {


int L1_cache :: COH_MSG = -1;
int L1_cache :: CREDIT_MSG = -1;


//! @param \c nid  Node ID of the cache.
L1_cache::L1_cache (int nid, const cache_settings& parameters, const L1_cache_settings& settings) :
    DOWNSTREAM_FULL_CREDITS(settings.downstream_credits)
{
    assert(COH_MSG != -1 && CREDIT_MSG != -1);

    node_id = nid;
    this->l2_map = settings.l2_map;

    stalled_client_req_buffer.clear();    
    //TODO: reimplement with hierarchies
    //stalled_peer_req_buffer.clear();    

    this->my_table = new hash_table (parameters);

    //mshr is created as a fully associated hash table.
    cache_settings mshr_parameters = parameters;
    mshr_parameters.assoc = settings.mshr_sz;
    mshr_parameters.size = mshr_parameters.assoc * mshr_parameters.block_size; //1 set.

    this->mshr = new hash_table (mshr_parameters);

    mcp_stalled_req.resize(my_table->get_num_entries());
    //mcp_stalled_um_req.resize(my_table->get_num_entries());
    mcp_state.resize(my_table->get_num_entries());

    for(unsigned i=0; i<mcp_stalled_req.size(); i++) {
        mcp_stalled_req[i] = 0;
        //mcp_stalled_um_req[i] = 0;
    }

    //flow control
    m_downstream_credits = settings.downstream_credits;

    //stats
    stats_cycles = 0;
    stats_processor_read_requests = 0;
    stats_processor_write_requests = 0;
    stats_hits = 0;
    stats_misses = 0;
    stats_MSHR_STALLs = 0;
    stats_PREV_PEND_STALLs = 0;
    stats_LRU_BUSY_STALLs = 0;
    stats_TRANS_STALLs = 0;
    stats_stall_buffer_max_size = 0;
    stats_table_occupancy = 0;
    stats_table_empty_cycles = 0;
}


L1_cache::~L1_cache (void)
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





/** TODO: Need to add LRU hook somewhere*/

/*
void L1_cache::handle_processor_request (int, cache_req *request)
{
    //stats
    if(stalled_client_req_buffer.size() > stats_stall_buffer_max_size)
	stats_stall_buffer_max_size = stalled_client_req_buffer.size();

    if(request->op_type == OpMemLd)
	stats_processor_read_requests++;
    else
	stats_processor_write_requests++;

    process_processor_request(request, true);
}
*/


//! Since L1_cache has 2 input ports, we may have simultaneous outputs. To ensure determinism,
//! tick() is used to handle simultaneous inputs in a determinstic way.
void L1_cache :: tick()
{
    while(m_net_requests.size() > 0) {
        NetworkPacket* pkt = m_net_requests.front();
	m_net_requests.pop_front();
	internal_handle_peer_and_manager_request (pkt);
    }
    while(m_proc_requests.size() > 0) {
        cache_req* req = m_proc_requests.front();
	m_proc_requests.pop_front();
	process_processor_request(req, true);
    }

    //stats
    stats_cycles++;
    unsigned tab_occupancy = my_table->get_occupancy();
    stats_table_occupancy += tab_occupancy;
    if(tab_occupancy == 0)
        stats_table_empty_cycles++;

}



void L1_cache :: process_processor_request (cache_req *request, bool first)
{

    DBG_L1_CACHE_TICK_ID( cout,  "###### " << " process_processor_request():  addr= " <<hex<< request->addr <<dec<< ((request->op_type==OpMemLd) ? " LD" : " ST") << "\n" );

    /** This code may be modified in the future to enable parallel events while transient (read while waiting for previous read-unblock).  For the time being, just assume no parallel events for a single address */
    if (mshr->has_match(request->addr))
    {
	DBG_L1_CACHE(cout, "  There is already an MSHR for the same line. Enter PREV_PEND_STALL.\n");

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
    if(mshr_entry == 0) {
	DBG_L1_CACHE(cout, "  No MSHR available. Enter MSHR_STALL.\n");

	assert(first);
	stall (request, C_MSHR_STALL);
	stats_MSHR_STALLs++;
        return;
    }


    mshr_map[mshr_entry->get_idx()] = 0;
    hash_entry* hash_table_entry = 0;

   
    /** Miss logic. */
    if (!my_table->has_match (request->addr)) {
	DBG_L1_CACHE(cout, "    miss.\n");

        /** Check if an invalid block exists already or the LRU block can begin eviction. */
	hash_table_entry = my_table->reserve_block_for (request->addr);

        if (hash_table_entry != 0 ) {
	    mshr_map[mshr_entry->get_idx()] = hash_table_entry;
            L1_algorithm (mshr_entry, request, true);
        } 
        else if (clients[my_table->get_replacement_entry (request->addr)->get_idx()]->req_pending() == false) {
	    DBG_L1_CACHE(cout, "  start eviction line= " <<hex<< my_table->get_replacement_entry(request->addr)->get_line_addr() <<dec<< "\n");

	    start_eviction (mshr_entry, request);
        }
        else {
	    //the client for the victim is in transient, so it must have a stalled request in the
	    //stall buffer; we cannot overwrite with this to-be-stalled request. The easiest solution is
	    //for the request to give up the MSHR, and wait for wakeup when the victim's request is
	    //finished and it releases its mshr.
	    DBG_L1_CACHE(cout, "  LRU_BUSY_STALL.\n");

	    assert(first);
	    mshr_entry->invalidate();
	    stall (request, C_LRU_BUSY_STALL);
	    stats_LRU_BUSY_STALLs++;
            return;
        }
    }
    /** Hit and Partial-hit logic (block exists, but coherence miss). */
    else {
	hash_table_entry = my_table->get_entry(request->addr);

        /** entry for this address exists, now we need to consider what state it's in */ 
        //if (hash_table_entry->being_replaced)
        if(clients[hash_table_entry->get_idx()]->req_pending() == true) {
	    DBG_L1_CACHE(cout, "  TRANS_STALL.\n");

	    //One possibility is the entry is being evicted.

	    //Treat TRANS_STALL the same way as LRU_BUSY_STALL. That is, we free the mshr and
	    //wait for wakeup when the victim finishes.
	    assert(first);
	    mshr_entry->invalidate();
	    stall (request, C_TRANS_STALL);
	    stats_TRANS_STALLs++;
            return;
        }
    
	mshr_map[mshr_entry->get_idx()] = hash_table_entry;

        /** We need to bring the MSHR up to speed w.r.t. state so the req can be processed. */
        /** So we copy the block over */
	//do I need to copy the hash_table_entry to mshr_entry ???
        L1_algorithm (mshr_entry, request, true);
    }
}



//====================================================================
//====================================================================
/** For each request, check if we have enough permissions.  If we do, process by the manager protocol.  Otherwise, have the client-side acquire the needed permissions so we can. */
//This should be part of the manager's logic, but it's here since L1 does not have manager.
//! @param \c first  True if this is the first time the function is called for the request. This is for stats only.
void L1_cache::L1_algorithm (hash_entry *e, cache_req *request, bool first)
{
    DBG_L1_CACHE(cout, "    L1_cache :: L1_algorithm()\n");

    ClientInterface* client = clients[mshr_map[e->get_idx()]->get_idx()];

    assert(mcp_stalled_req[client->getClientID()] == 0);

    mcp_state[client->getClientID()] = PROCESSING_LOWER;

    if (request->op_type == OpMemLd) {
        if(client->HaveReadP()) {
	    //my_table->update_lru(request->addr);
            read_reply(request);
	    release_mshr_entry(e);
	    if(first)
	        stats_hits++;
	}
        else {
            client->GetReadD();

	    mcp_stalled_req[client->getClientID()] = request;
        }
    }
    else if (request->op_type == OpMemSt) {
        if(client->HaveWriteP()) {
	    //my_table->update_lru(request->addr);
            write_reply (request);
	    release_mshr_entry(e);
	    if(first)
	        stats_hits++;
	}
        else {
            // TODO: May be worth re-defining the interface to avoid this.
            if(client->HaveReadP()) {
                client->GetWrite();  //upgrade
            }
            else 
                client->GetWriteD();
            // Check if got insta-permissions. If in state E, can go to M without doing anything else.
            if(client->HaveWriteP()) {
		//my_table->update_lru(request->addr);
                write_reply (request);
		release_mshr_entry(e);
		if(first)
		    stats_hits++;
	    }
            else {
		mcp_stalled_req[client->getClientID()] = request;
	    }
        }
    }
    else {
        assert(0 && "not a read or write?");
    }
}




//====================================================================
//! @return true if evicted right away; false if not evicted yet.
//====================================================================
void L1_cache :: start_eviction (hash_entry* mshr_entry, cache_req *request)
{ 
    DBG_L1_CACHE_ID(cout, " start eviction(), for addr= " <<hex<< request->addr <<dec<< "\n");

    hash_entry* victim = my_table->get_replacement_entry(request->addr);

    assert(mshr->has_match(victim->get_line_addr()) == false); //victim shouldn't have an mshr entry.

    ClientInterface* client = clients[victim->get_idx()];
    
    assert(client->HaveEvictP() == false);

    client->GetEvict();

    // Check if got insta-permissions.
    if(client->HaveEvictP()) { //this could happen, for example, when client in S state in MESI.
	DBG_L1_CACHE(cout, "  evicted immediately\n");

	hash_entry* hash_table_entry = my_table->reserve_block_for (request->addr);
	mshr_map[mshr_entry->get_idx()] = hash_table_entry;
	L1_algorithm (mshr_entry, request, true);
    }
    else {
        DBG_L1_CACHE(cout, "  stall for eviction to complete\n");

	assert(mcp_stalled_req[client->getClientID()] == 0); //could this assert fail, i.e, could
								//the victim be in the middle of something?
	mcp_stalled_req[client->getClientID()] = request;

	mcp_state[client->getClientID()] = EVICTING;
    }
}




//====================================================================
//! for both request and reply from upper manager or peer (when forwarding is supported)
//====================================================================
void L1_cache :: internal_handle_peer_and_manager_request (NetworkPacket* pkt)
{
    if(pkt->type == CREDIT_MSG) {
        delete pkt;
        m_downstream_credits++;
	assert(m_downstream_credits <= DOWNSTREAM_FULL_CREDITS);
	try_send();
	return;
    }

    assert(pkt->type == COH_MSG);

    Coh_msg* request = new Coh_msg;

    *request = *((Coh_msg*)(pkt->data));

    DBG_L1_CACHE_TICK_ID(cout, "@@@@@@  internal_handle_peer_and_manager_request(),  src= " << pkt->src << " addr= " <<hex<< request->addr <<dec<< " fwd= " << request->forward_id << "\n");

    delete pkt;
    process_peer_and_manager_request(request);
    //If the msg is a reply, we can certainly send a credit back.
    //If it's a request, the assumption is the request is finished after calling process_upper_manager_request(),
    //(see the function process_upper_manager_request() for comments), then a credit can be sent.
    //Based on the above, we always send a credit back at this point. 
    //credit is sent 1 tick after receiving the request.
    manifold::kernel::Manifold::ScheduleClock(1, *m_clk, &L1_cache::send_credit_downstream, this);

    #ifdef FORECAST_NULL
    m_credit_out_ticks.push_back(m_clk->NowTicks() + 1);
    #endif
}



void L1_cache :: process_peer_and_manager_request(Coh_msg* request)
{

/** Check to see if block exists in the hashtable
    if not exist
        do 'default client' behavior (handles invalidate to a block you've already evicted, etc.)
    if exist
        if is demand/downgrade
            if can downgrade
                stall request and issue appropriate demand to paired manager
            else 
                stall request
*/

    if(request->type == Coh_msg::COH_RPLY) {
        DBG_L1_CACHE(cout, "    it is a reply.\n");

	if (mshr->has_match(request->addr)) {
	    /** This is either a reply to an earlier request or a req to something transient. */
	    hash_entry* mshr_entry = mshr->get_entry(request->addr);
	    assert(my_table->has_match(request->addr));

	    ClientInterface* client = clients[mshr_map[mshr_entry->get_idx()]->get_idx()];

	    client->process(request->msg, request->forward_id);

	    if(client->req_pending() == false) { //request is complete
		c_notify(mshr_entry, client);
	    }
	}
	else {
	    //a reply to something with no mshr entry; must be for an eviction.
	    //A request from upper manager doesn't have mshr entry either. So, it's
	    //possible the reply is part of the upper manager request.
	    hash_entry* hash_table_entry = my_table->get_entry(request->addr);
	    assert(hash_table_entry);

	    ClientInterface* client = clients[hash_table_entry->get_idx()];
	    client->process(request->msg, request->forward_id);

	    if(client->req_pending() == false) { //request is complete
		assert(mcp_state[client->getClientID()] == EVICTING);
		c_evict_notify(client);
	    }
	}
	delete request;
    }
    else if(request->type == Coh_msg::COH_REQ)
    {
	/*
        //request from upper manager: downgrade or invalidation requests.
	if (!my_table->has_match (request->u.coh.addr) && !mshr->has_match(request->u.coh.addr))
	{
	    respond_with_default(request);
	    return;
	}
	*/
	if(!my_table->has_match(request->addr)) {
	    //There may or may not be an MSHR entry for the address.
	    //The case where there is an MSHR could happen in MESI like this:
	    //C0 evicts line X which is in S state; since it's in S,
	    //directory is not notified. Later on, C0 accesses the same line and gets a MSHR. At the same
	    //time C1 writes to X causing directory to send invalidation of X to C0 since directory still
	    //thinks C0 is in S state. Now C0 gets a request which has an MSHR but not in the hash table.
	    respond_with_default(request);
	    return;
	}
	process_upper_manager_request(request);
    }
    else {
        assert(0);
    }

}



void L1_cache :: process_upper_manager_request(Coh_msg* request)
{
    //request from upper manager: downgrade or invalidation requests.
    hash_entry* hash_table_entry = my_table->get_entry(request->addr);
    assert(hash_table_entry);

    ClientInterface* client = clients[hash_table_entry->get_idx()];

    client->process(request->msg, request->forward_id);

    //NOTE: here we assume upper manager requests can be finished in one step by calling
    //process() as we did above, which is most likely true for L1. With this assumption,
    //we don't need to deal with the messy issue of determining whether the upper manager's
    //request has been completed.
    //Since such requests are completed in one step, we don't need mcp_stalled_um_req.

    //NOTE that client->req_pending() == false cannot be used to determine whether
    //the upper manager's request has completed. For example, with MESI, when client
    //is in SE state and gets a DEMAND_I, it replies with UNBLOCK_I and stays in SE,
    //which is not a steady state, but the DEMAND_I has been completed. Note that
    //in this case the client cannot go to I state because it has already allocated
    //a hash entry and has sent I_to_E to upper manager.

    delete request;

    //check if any request stalled on this.
    if(client->req_pending() == false) { //request is complete
	cache_req* creq = mcp_stalled_req[client->getClientID()];
	if(creq) {
	    if(mcp_state[client->getClientID()] == EVICTING) {
		hash_entry* mshr_entry = mshr->get_entry(creq->addr);
		assert(mshr_entry); //if a request is stalled on a hash entry, it must already have an mshr entry.
		//the request must be waiting for eviction; in other words, it must miss in the cache.
		assert(my_table->get_entry(creq->addr) == 0);
		c_evict_notify(client);
	    }
	    else {
	        //If client is not processing eviction, that means it's processing a processor request,
		//which would be an upgrade request. This request cannot be finised as a request of
		//getting an upper manager request.
	        assert(0);
	    }
	}
    }
}


//! called when client enters stable state, while processing an upper manager request.
//void L1_cache :: c_um_notify(ClientInterface* client)
//{
//    Coh_mem_req* umreq = mcp_stalled_um_req[client->getClientID()];
//    mcp_stalled_um_req[client->getClientID()] = 0;
//    assert(umreq);
//    process_upper_manager_request(umreq, false);
//}



//! called when client enters stable state, while processing a processor request.
void L1_cache :: c_notify(hash_entry* mshr_entry, ClientInterface* client)
{
    DBG_L1_CACHE(cout, "    c_notify, line addr= "  <<hex<< mshr_map[mshr_entry->get_idx()]->get_line_addr() <<dec<< "\n");

    cache_req* creq = mcp_stalled_req[client->getClientID()];
    if(creq) {
	mcp_stalled_req[client->getClientID()] = 0;

        DBG_L1_CACHE(cout, "    reprocessing request, addr= "  <<hex<< creq->addr <<dec<< "\n");

	L1_algorithm (mshr_entry, creq, false);
    }
}







//! called when client enters stable state, while being evicted.
void L1_cache :: c_evict_notify(ClientInterface* client)
{
    DBG_L1_CACHE(cout, " c_evict_notify()\n");

    cache_req* creq = mcp_stalled_req[client->getClientID()];
    assert(creq);

    mcp_stalled_req[client->getClientID()] = 0;

    hash_entry* mshr_entry = mshr->get_entry(creq->addr);
    assert(mshr_entry); //if a request is talled on a hash entry, it must already have an mshr entry.

    DBG_L1_CACHE(cout, "    L1_cache: wakeup request waiting for eviction, addr= "  <<hex<< creq->addr <<dec<< "\n");

    hash_entry* hash_table_entry = my_table->reserve_block_for (creq->addr);
    assert(hash_table_entry);
    mshr_map[mshr_entry->get_idx()] = hash_table_entry;
    L1_algorithm (mshr_entry, creq, true);
}



void L1_cache::stall (cache_req *request, stall_type_t stall_msg)
{
    Stall_buffer_entry e;
    e.req = request;
    e.type = stall_msg;
    e.time = m_clk->NowTicks();
    //stalled_client_req_buffer.push_back (std::make_pair (request, stall_msg));
    stalled_client_req_buffer.push_back (e);
}

bool L1_cache :: stall_buffer_has_match(paddr_t addr)
{
    paddr_t line_addr = my_table->get_line_addr(addr);

    for(std::list<Stall_buffer_entry>::iterator it= stalled_client_req_buffer.begin(); it != stalled_client_req_buffer.end(); ++it) {
	if( my_table->get_line_addr((*it).req->addr) == line_addr)
	    return true;
    }
    return false;
}



//! Reply to processor for LOAD.
void L1_cache::read_reply (cache_req *request)
{
    DBG_L1_CACHE_ID(cout, " read reply to processor,  addr= " <<hex<< request->addr <<dec<< "\n");

    request->preqWrapper->SendTick(this, PORT_PROC, my_table->get_hit_time());
    delete request->preqWrapper;
    delete request;
}

//! Reply to processor for STORE.
void L1_cache::write_reply (cache_req *request)
{
    DBG_L1_CACHE_ID(cout,  " write reply to processor,  addr= " <<hex<< request->addr <<dec<< "\n");

    request->preqWrapper->SendTick(this, PORT_PROC, my_table->get_hit_time());
    delete request->preqWrapper;
    delete request;
}


void L1_cache :: send_msg_to_peer_or_l2(Coh_msg* msg)
{
    msg->src_id = node_id;
    if(msg->dst_id == -1) //destination is L2
	msg->dst_id = l2_map->lookup(msg->addr);

    DBG_L1_CACHE_ID(cout, "L1_cache node " << node_id << " sending msg= " << msg->msg << " to node " << msg->dst_id << " addr= " <<hex<< msg->addr <<dec<< endl);

    NetworkPacket* pkt = new NetworkPacket;
    pkt->type = COH_MSG;
    pkt->src = node_id;
    pkt->dst = msg->dst_id;
    *((Coh_msg*)(pkt->data)) = *msg;
    pkt->data_size = sizeof(Coh_msg);
    delete msg;

    assert(my_table->get_lookup_time() > 0);
    manifold::kernel::Manifold::ScheduleClock(my_table->get_lookup_time(), *m_clk, &L1_cache::send_msg_after_lookup_time, this, pkt);
}

//! This function is scheduled by send_msg_to_peer_or_l2() to send the message after a delay of lookup_time
void L1_cache :: send_msg_after_lookup_time(NetworkPacket* pkt)
{
    m_downstream_output_buffer.push_back(pkt);
    try_send();
}


void L1_cache :: try_send()
{
    if(!m_downstream_output_buffer.empty() && m_downstream_credits > 0) {
	NetworkPacket* pkt = m_downstream_output_buffer.front();
	m_downstream_output_buffer.pop_front();
        Send(PORT_L2, pkt);
        m_downstream_credits--;
    }
}


void L1_cache :: send_credit_downstream()
{
    DBG_L1_CACHE_ID(cout, " sending credit.\n");

    NetworkPacket* pkt = new NetworkPacket;
    pkt->type = CREDIT_MSG;
    Send(PORT_L2, pkt);
}


void L1_cache :: release_mshr_entry(hash_entry* mshr_entry)
{
    DBG_L1_CACHE(cout, "    release_mshr_entry, entry= " << mshr_entry << "\n");

    mshr_map[mshr_entry->get_idx()] = 0;
    mshr_entry->invalidate();

    //check the stall buffer and see if any request waiting for mshr
    std::list<Stall_buffer_entry>::iterator it = stalled_client_req_buffer.begin();

    cache_req* creq = 0;
    manifold::kernel::Ticks_t stall_time;
    stall_type_t stallType; //for debug

    bool found = false; //if there's a stalled request to wake up
    while(it != stalled_client_req_buffer.end()) {
        //Wake up a stalled request only if it will not PREV_PEND_STALL, LRU_BUSY_STALL, or TRANS_STALL.
	//It canno MSHR_STALL because we are releasing an MSHR entry only a few lines ago.
	if (!mshr->has_match((*it).req->addr)) { //won't PREV_PEND
	    if(!my_table->has_match ((*it).req->addr)) { //going to miss
		if(clients[my_table->get_replacement_entry ((*it).req->addr)->get_idx()]->req_pending() == false) { //won't LRU_BUSY
		    found = true;
		}
	    }
	    else { //hit
		if (clients[my_table->get_entry((*it).req->addr)->get_idx()]->req_pending() == false) { //won't TRANS
		    found = true;
		}
	    }
	}

	if(found) {
	    creq = (*it).req;
	    stall_time = (*it).time;
	    stalled_client_req_buffer.erase(it);
	    break;
	}

	++it;
    }

    if(creq != 0) {
        //do a sanity check
	paddr_t line_addr = my_table->get_line_addr(creq->addr);
	for(std::list<Stall_buffer_entry>::iterator it = stalled_client_req_buffer.begin(); it != stalled_client_req_buffer.end(); ++it) {
	    if(my_table->get_line_addr((*it).req->addr) == line_addr) {
	        assert(stall_time <= (*it).time); //if a stalled request is for the same line, then it must be stalled later than the one
		                                  //being released.
	    }
	}
        DBG_L1_CACHE_ID(cout, " release mshr entry wakes up req= " << creq << " addr= " <<hex<< creq->addr <<dec<< "\n");
	process_processor_request(creq, false);
    }
}



void L1_cache :: print_stats(std::ostream& out)
{
    double avg_occup = (double)stats_table_occupancy / stats_cycles;

    out << "L1 node " << node_id << endl
        << "    Processor requests = " << stats_processor_read_requests << " (R) + " << stats_processor_write_requests
	                               << " (W) = " << stats_processor_read_requests + stats_processor_write_requests << endl
	<< "    Hits = " << stats_hits << endl
	<< "    Hit rate = " << (double)stats_hits / (stats_processor_read_requests + stats_processor_write_requests) << endl
        << "    Stall buffer max size= " << stats_stall_buffer_max_size << endl
        << "    MSHR_STALL = " << stats_MSHR_STALLs << endl
        << "    PREV_PEND_STALL = " << stats_PREV_PEND_STALLs << endl
        << "    PREV_LRU_BUSY_STALL = " << stats_LRU_BUSY_STALLs << endl
        << "    PREV_TRANS_STALL = " << stats_TRANS_STALLs << endl
	<< "    cycles = " << stats_cycles << endl
	<< "    hash table avg occupancy = " << avg_occup << " (" << avg_occup / (my_table->get_size() / my_table->get_block_size()) * 100 << "%)" << endl
	<< "    hash table empty cycles= " << stats_table_empty_cycles << endl;
}


//####################################################################
// for debug
//####################################################################
void L1_cache :: print_mshr()
{
    mshr->dbg_print(cout);
}


void L1_cache :: print_stall_buffer()
{
    cout << "Stall buffer:\n";
    for(std::list<Stall_buffer_entry>::iterator it= stalled_client_req_buffer.begin(); it != stalled_client_req_buffer.end(); ++it) {
	cout << " (" <<hex<< (*it).req->addr <<dec<< ") (" << (*it).type << ")" << "\n";
    }
    cout << endl;
}







} // namespace mcp_cache_namespace
} // namespace manifold

