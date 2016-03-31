/*
 * Controller.cpp
 *
 *  Created on: Jun 14, 2011
 *      Author: rishirajbheda
 */

#include "Controller.h"
#include "kernel/component.h"
#include "kernel/manifold.h"

#include <iostream>

using namespace std;
using namespace manifold::kernel;
using namespace manifold::uarch;


namespace manifold {
namespace caffdram {

int Controller :: MEM_MSG_TYPE = -1;
int Controller :: CREDIT_MSG_TYPE = -1;
bool Controller :: Msg_type_set = false;


Controller::Controller(int nid, const Dsettings& s, int credits, bool resp) :
	m_send_st_response(resp), m_DOWNSTREAM_FULL_CREDITS(credits)
{
        assert(Msg_type_set);
        assert(MEM_MSG_TYPE != CREDIT_MSG_TYPE);

	m_nid = nid;
	m_downstream_credits = credits;

	this->dramSetting = new Dsettings (s);
	this->dramSetting->update(); //call update in case fileds of s were changed after construction.

	this->myChannel = new Channel*[this->dramSetting->numChannels];
	for (int i = 0; i < this->dramSetting->numChannels; i++)
	{
		this->myChannel[i] = new Channel (this->dramSetting);
	}

	//stats
	stats_max_output_buffer_size = 0;

	stats_requests_count = 0;
	stats_last_requests_count_change_tick = 0;
	stats_requests_count_integration = 0;
	stats_non_zero_requests_ticks = 0;
}

Controller::~Controller() {
	// TODO Auto-generated destructor stub

	for (int i = 0; i < this->dramSetting->numChannels; i++)
	{
		delete this->myChannel[i];
	}
	delete [] this->myChannel;
	delete this->dramSetting;
}

unsigned long int Controller::processRequest(unsigned long int reqAddr, unsigned long int currentTime)
{
	Dreq* dRequest = new Dreq (reqAddr, currentTime, this->dramSetting);
#ifdef DBG_CAFFDRAM
cout << "ch-rank-bank = " << dRequest->get_chId() << "-" << dRequest->get_rankId() << "-" << dRequest->get_bankId() << endl;
#endif
	unsigned long int returnLatency = this->myChannel[dRequest->get_chId()]->processRequest(dRequest);
	delete dRequest;
	return (returnLatency);
}


/*
//! Event handler for memory requests.
void Controller :: handle_request(int, mem_req* req)
{
    if(req->op_type == OpMemLd) {
#ifdef DBG_CAFFDRAM
cout << "mc received LD, src= " << req->source_id << " port= " <<req->source_port << " addr= " <<hex<< req->addr <<dec<<endl;
#endif
	//stats
	m_ld_misses[req->source_id]++;
	m_req_info.insert(pair<Ticks_t, Req_info>(Manifold::NowTicks(),
	                                          Req_info(OpMemLd, req->originator_id, req->source_id, req->addr)));

	req->msg = LD_RESPONSE;
	req->req_id = req->req_id;
	req->dest_id = req->source_id;
	req->dest_port = req->source_port;
	req->source_id = m_nid;
	req->source_port = 0;
	Ticks_t latency = processRequest(req->addr, Manifold::NowTicks()); //????????????? using default clock here.
	//The return value of processRequest() is the actual (or absolute) time of when the request
	//is completed, but Sendtick requires time relative to now. So we must pass to SendTick
	//the return value - now.
	SendTick(PORT0, req, latency - Manifold::NowTicks());
    }
    else {
#ifdef DBG_CAFFDRAM
cout << "mc received ST, src= " << req->source_id << " port= " <<req->source_port << " addr= " <<hex<< req->addr <<dec<<endl;
#endif
	//stats
	m_stores[req->source_id]++;
	m_req_info.insert(pair<Ticks_t, Req_info>(Manifold::NowTicks(),
	                                          Req_info(OpMemSt, req->originator_id, req->source_id, req->addr)));
	Ticks_t latency = processRequest(req->addr, Manifold::NowTicks()); //????????????? using default clock here.
        if(m_send_st_response) {
	    req->msg = ST_COMPLETE;
	    req->req_id = req->req_id;
	    req->dest_id = req->source_id;
	    req->dest_port = req->source_port;
	    req->source_id = m_nid;
	    req->source_port = 0;
	    //The return value of processRequest() is the actual (or absolute) time of when the request
	    //is completed, but Sendtick requires time relative to now. So we must pass to SendTick
	    //the return value - now.
	    SendTick(PORT0, req, latency - Manifold::NowTicks());
	}
    }

}
*/


#if 0
// moved to header file.

//! Event handler for memory requests.
void Controller :: handle_request(int, Coh_mem_req* req)
{
    if(req->u.mem.op_type == OpMemLd) {
#ifdef DBG_CAFFDRAM
cout << "mc received LD, src= " << req->u.mem.source_id << " port= " <<req->u.mem.source_port << " addr= " <<hex<< req->u.mem.addr <<dec<<endl;
#endif
	//stats
	m_ld_misses[req->u.mem.source_id]++;
	/*
	m_req_info.insert(pair<Ticks_t, Req_info>(Manifold::NowTicks(),
	                                          Req_info(OpMemLd, req->u.mem.originator_id, req->u.mem.source_id, req->u.mem.addr)));
						  */

	req->u.mem.msg = LD_RESPONSE;
	//req->u.mem.req_id = req->u.mem.req_id;
	req->u.mem.dest_id = req->u.mem.source_id;
	req->u.mem.dest_port = req->u.mem.source_port;
	req->u.mem.source_id = m_nid;
	req->u.mem.source_port = 0;
	Ticks_t latency = processRequest(req->u.mem.addr, Manifold::NowTicks()); //????????????? using default clock here.
	//The return value of processRequest() is the actual (or absolute) time of when the request
	//is completed, but Sendtick requires time relative to now. So we must pass to SendTick
	//the return value - now.
	SendTick(PORT0, req, latency - Manifold::NowTicks());
    }
    else {
#ifdef DBG_CAFFDRAM
cout << "mc received ST, src= " << req->u.mem.source_id << " port= " <<req->u.mem.source_port << " addr= " <<hex<< req->u.mem.addr <<dec<<endl;
#endif
	//stats
	m_stores[req->u.mem.source_id]++;
	/*
	m_req_info.insert(pair<Ticks_t, Req_info>(Manifold::NowTicks(),
	                                          Req_info(OpMemSt, req->u.mem.originator_id, req->u.mem.source_id, req->u.mem.addr)));
						  */
	Ticks_t latency = processRequest(req->u.mem.addr, Manifold::NowTicks()); //????????????? using default clock here.
        if(m_send_st_response) {
	    req->u.mem.msg = ST_COMPLETE;
	    //req->u.mem.req_id = req->u.mem.req_id;
	    req->u.mem.dest_id = req->u.mem.source_id;
	    req->u.mem.dest_port = req->u.mem.source_port;
	    req->u.mem.source_id = m_nid;
	    req->u.mem.source_port = 0;
	    //The return value of processRequest() is the actual (or absolute) time of when the request
	    //is completed, but Sendtick requires time relative to now. So we must pass to SendTick
	    //the return value - now.
	    SendTick(PORT0, req, latency - Manifold::NowTicks());
	}
    }

}
#endif


void Controller :: print_config(ostream& out)
{
    out << "***** CaffDRAM " << m_nid << " config *****" << endl;
    out << "  send store response: " << (m_send_st_response ? "yes" : "no") << endl;
    out << "  num of channels = " << dramSetting->numChannels << endl
        << "  num of ranks = " << dramSetting->numRanks << endl
        << "  num of banks = " << dramSetting->numBanks << endl
        << "  num of rows = " << dramSetting->numRows << endl
        << "  num of columns = " << dramSetting->numColumns << endl
        << "  num of columns = " << dramSetting->numColumns << endl
        << "  page policy = " << ((dramSetting->memPagePolicy == OPEN_PAGE) ? "Open page" : "Closed page") << endl
	<< "  t_RTRS (rank to rank switching time) = " << dramSetting->t_RTRS << endl
	<< "  t_OST (ODT switching time) = " << dramSetting->t_OST << endl
	<< "  t_BURST (burst length on mem bus) = " << dramSetting->t_BURST << endl
	<< "  t_CWD (column write delay) = " << dramSetting->t_CWD << endl
	<< "  t_RAS (row activation time) = " << dramSetting->t_RAS << endl
	<< "  t_RP (row precharge time) = " << dramSetting->t_RP << endl
	<< "  t_RTP (read to precharge delay) = " << dramSetting->t_RTP << endl
	<< "  t_CCD (column to column delay) = " << dramSetting->t_CCD << endl
	<< "  t_CAS (column access latency) = " << dramSetting->t_CAS << endl
	<< "  t_RCD (row to column delay) = " << dramSetting->t_RCD << endl
	<< "  t_CMD (command delay) = " << dramSetting->t_CMD << endl
	<< "  t_RFC (refresh cycle time) = " << dramSetting->t_RFC << endl
	<< "  t_WR (write recovery time) = " << dramSetting->t_WR << endl
	<< "  t_WTR (write to read delay) = " << dramSetting->t_WTR << endl
	<< "  t_FAW (four bank activation window) = " << dramSetting->t_FAW << endl
	<< "  t_RRD (row to row activation delay) = " << dramSetting->t_RRD << endl
	<< "  t_RC (row cycle time) = " << dramSetting->t_RC << endl
	<< "  t_REF_INT (refresh interval) = " << dramSetting->t_REF_INT << endl;
}

void Controller :: print_stats(ostream& out)
{
    out << "********** CaffDRAM " << m_nid << " stats **********" << endl;
    out << "max output buffer size= " << stats_max_output_buffer_size << endl;
    out << "=== Load misses ===" << endl;
    for(map<int, int>::iterator it = m_ld_misses.begin(); it != m_ld_misses.end(); ++it) {
        out << "    " << (*it).first << ": " << (*it).second << endl;
    }
    out << "=== stores ===" << endl;
    for(map<int, int>::iterator it = m_stores.begin(); it != m_stores.end(); ++it) {
        out << "    " << (*it).first << ": " << (*it).second << endl;
    }

    for(multimap<Ticks_t, Req_info>::iterator it = m_req_info.begin(); it != m_req_info.end(); ++it) {
        Req_info& req = (*it).second;
        out << "MC received: " << (*it).first << " " << ((req.type == 0) ? "LD" : "ST")
	    << " N_" << req.org_id << "(" << req.src_id << ") " << hex << "0x" << req.addr << dec << endl;
    }
    out << "Avg requests: " << (double)stats_requests_count_integration / manifold::kernel::Manifold::NowTicks() << endl;
    out << "Avg requests(excluding 0-requests periods): " << (double)stats_requests_count_integration / stats_non_zero_requests_ticks << endl; //average requests calculated over the periods when there was 1 or more request
    
    uint64_t non_idle_cycles_at_end = 0;
    if(stats_requests_count != 0) { //from last change to termination there are some requests
        non_idle_cycles_at_end = manifold::kernel::Manifold::NowTicks() - stats_last_requests_count_change_tick;
    }
    out << "Idle cycles: " << (manifold::kernel::Manifold::NowTicks() - stats_non_zero_requests_ticks) - non_idle_cycles_at_end << endl;

    for (int i = 0; i < this->dramSetting->numChannels; i++) {
	out << "Channel " << i << ":" << endl;
	myChannel[i]->print_stats(out);
    }
}



void Controller :: credit_received(NetworkPacket* pkt)
{
    delete pkt;
    try_send();
}

void Controller :: request_complete(NetworkPacket* pkt, bool read)
{
    if(read || m_send_st_response) {
	m_completed_reqs.push_back(pkt);
	if(m_completed_reqs.size() > stats_max_output_buffer_size)
	    stats_max_output_buffer_size = m_completed_reqs.size();
	try_send();
    }
    else {
	uint64_t duration = (manifold::kernel::Manifold::NowTicks() - stats_last_requests_count_change_tick);
	stats_requests_count_integration += duration * stats_requests_count;
	if(stats_requests_count > 0) {
	    stats_non_zero_requests_ticks += duration;
	}

	stats_requests_count--;
	stats_last_requests_count_change_tick = manifold::kernel::Manifold::NowTicks();

	delete pkt;

	//Send a credit downstream.
	NetworkPacket* credit_pkt = new NetworkPacket();
	credit_pkt->type = CREDIT_MSG_TYPE;   
	Send(PORT0, credit_pkt);
    }
}


void Controller :: try_send()
{
    if (!m_completed_reqs.empty() && m_downstream_credits > 0) {
        //stats
	uint64_t duration = (manifold::kernel::Manifold::NowTicks() - stats_last_requests_count_change_tick);
	stats_requests_count_integration += duration * stats_requests_count;
	if(stats_requests_count > 0) {
	    stats_non_zero_requests_ticks += duration;
	}

	stats_requests_count--;
	stats_last_requests_count_change_tick = manifold::kernel::Manifold::NowTicks();

	NetworkPacket* req = m_completed_reqs.front();
	m_completed_reqs.pop_front();

#ifdef DBG_CAFFDRAM
cerr << "@ " << dec << manifold::kernel::Manifold::NowTicks() << " mc completes transaction dst= " << req->get_dst() << " port= " << req->get_dst_port() <<dec<<endl;
#endif
        Send(PORT0, req);
        m_downstream_credits--;

	//Send a credit downstream.
        NetworkPacket* credit_pkt = new NetworkPacket();
        credit_pkt->type = CREDIT_MSG_TYPE;   
        Send(PORT0, credit_pkt);
   }
}



} //namespace caffdram
} //namespace manifold

