/*
 * Controller.h
 *
 *  Created on: Jun 14, 2011
 *      Author: rishirajbheda
 */

#ifndef MANIFOLD_CAFFDRAM_CONTROLLER_H_
#define MANIFOLD_CAFFDRAM_CONTROLLER_H_

#include "Channel.h"
#include "Dsettings.h"
#include "Dreq.h"

#include "kernel/component-decl.h"
#include "kernel/manifold-decl.h"
#include "uarch/networkPacket.h"

#include <map>
#include <list>

namespace manifold {
namespace caffdram {


class Controller : public manifold::kernel::Component {
public:
	enum {PORT0=0};

	//! @arg \c st_resp whether responses are sent for stores
	Controller(int nid, const Dsettings&, int out_credits, bool st_resp=false);
	~Controller();

	int get_nid() { return m_nid; }

	template<typename T>
	void handle_request(int, uarch::NetworkPacket* pkt);

        void print_config(std::ostream&);
        void print_stats(std::ostream&);

	static void Set_msg_types(int mem, int credit)
	{
	    assert(Msg_type_set == false);
	    MEM_MSG_TYPE = mem;
	    CREDIT_MSG_TYPE = credit;
	    Msg_type_set = true;
	}

#ifdef CAFFDRAM_TEST
public:
#else
private:
#endif
	unsigned long int processRequest (unsigned long int reqAddr, unsigned long int currentTime);

	//Called when a request is complete.
	void request_complete(uarch::NetworkPacket*, bool read);
	void credit_received(uarch::NetworkPacket*);
	void try_send();

	int m_nid; //node id
	const bool m_send_st_response; //send response for stores
	Dsettings* dramSetting;
	Channel** myChannel;

        static int MEM_MSG_TYPE;
        static int CREDIT_MSG_TYPE;
	static bool Msg_type_set; //this is used to ensure message types are only set once

	int m_downstream_credits;
	const int m_DOWNSTREAM_FULL_CREDITS; //for debug
	std::list<uarch::NetworkPacket*> m_completed_reqs; //this is the output buffer holding completed requests.


	//for stats
	struct Req_info {
	    Req_info(int t, int oid, int sid, uint64_t a) : type(t), org_id(oid), src_id(sid), addr(a) {}
	    int type; //ld or st: 0 for load, 1 for store
	    int org_id; //originator id
	    int src_id; //source id; could be different from originator id, e.g., when it's from an LLS cache
	    uint64_t addr;
	};

	std::map<int, int> m_ld_misses; //number of ld misses per source
	std::map<int, int> m_stores;
	std::multimap<manifold::kernel::Ticks_t, Req_info> m_req_info; //request info in time order

	//for calculating average accepted requests
	unsigned stats_requests_count; //number of accepted requests currently
	uint64_t stats_last_requests_count_change_tick; //when the count changed last time
	uint64_t stats_requests_count_integration; // sigma(c * t): c: requests count; t: ticks the count lasted
	                                           // integration/total_ticks = average requests at any moment
        uint64_t stats_non_zero_requests_ticks; //num of ticks during which there is 1 or more request

	//stats
	unsigned stats_max_output_buffer_size;

};



//! Event handler for memory requests.
template<typename T>
void Controller :: handle_request(int, uarch::NetworkPacket* pkt)
{
    if (pkt->type == CREDIT_MSG_TYPE) {
        m_downstream_credits++;
	assert(m_downstream_credits <= m_DOWNSTREAM_FULL_CREDITS);

	credit_received(pkt);
        return;
    }

    assert(pkt->dst == m_nid);

    //stats
    uint64_t duration = (manifold::kernel::Manifold::NowTicks() - stats_last_requests_count_change_tick);
    stats_requests_count_integration += duration * stats_requests_count;
    if(stats_requests_count > 0) {
        stats_non_zero_requests_ticks += duration;
    }

    stats_requests_count++;
    stats_last_requests_count_change_tick = manifold::kernel::Manifold::NowTicks();


    T* req = (T*)(pkt->data);

    if(req->is_read()) {
#ifdef DBG_CAFFDRAM
cerr << "@ " << dec << manifold::kernel::Manifold::NowTicks() << " mc [" << m_nid << "] received LD, src= " << pkt->get_src() << " port= " << pkt->get_src_port() << "dst= " << pkt->get_dst() << " port= " << pkt->get_dst_port() << " addr= " <<hex<< req->get_addr() <<dec<<endl;
#endif
	//stats
	m_ld_misses[pkt->get_src()]++;
	/*
	m_req_info.insert(pair<Ticks_t, Req_info>(Manifold::NowTicks(),
	                                          Req_info(OpMemLd, req->u.mem.originator_id, req->u.mem.source_id, req->u.mem.addr)));
						  */

	//req->u.mem.msg = LD_RESPONSE;

	req->set_mem_response(); //make it explicit the reply is a memory response. This is a temp solution!!!!!!!!!!!!!!!!!!
	                         //The MC should send its own type.

	req->set_dst(pkt->get_src());
	req->set_dst_port(pkt->get_src_port());
	req->set_src(m_nid);
	req->set_src_port(0);
	manifold::kernel::Ticks_t latency = processRequest(req->get_addr(), manifold::kernel::Manifold::NowTicks()); //????????????? using default clock here.
	//The return value of processRequest() is the actual (or absolute) time of when the request
	//is completed, but Schedule requires time relative to now. So we must pass to Schedule
	//the return value - now.

	//reuse the network packet object.
	pkt->set_dst(pkt->get_src());
	pkt->set_dst_port(pkt->get_src_port());
	pkt->set_src(m_nid);
	pkt->set_src_port(0);
	manifold::kernel::Manifold::Schedule(latency - manifold::kernel::Manifold::NowTicks(), &Controller::request_complete, this, pkt, true);
    }
    else { //write request
#ifdef DBG_CAFFDRAM
cerr << "@ " << dec << manifold::kernel::Manifold::NowTicks() << " mc [" << m_nid << "] received ST, src= " << pkt->get_src() << " port= " << pkt->get_src_port() << "dst= " << pkt->get_dst() << " port= " << pkt->get_dst_port() << " addr= " <<hex<< req->get_addr() <<dec<<endl;
#endif
	//stats
	m_stores[req->get_src()]++;
	/*
	m_req_info.insert(pair<Ticks_t, Req_info>(Manifold::NowTicks(),
	                                          Req_info(OpMemSt, req->u.mem.originator_id, req->u.mem.source_id, req->u.mem.addr)));
						  */
	manifold::kernel::Ticks_t latency = processRequest(req->get_addr(), manifold::kernel::Manifold::NowTicks()); //????????????? using default clock here.
        if(m_send_st_response) {
	    req->set_dst(pkt->get_src());
	    req->set_dst_port(pkt->get_src_port());
	    req->set_src(m_nid);
	    req->set_src_port(0);

	    pkt->set_dst(pkt->get_src());
	    pkt->set_dst_port(pkt->get_src_port());
	    pkt->set_src(m_nid);
	    pkt->set_src_port(0);
	}
	manifold::kernel::Manifold::Schedule(latency - manifold::kernel::Manifold::NowTicks(), &Controller::request_complete, this, pkt, false);
    }
}



} //namespace caffdram
} //namespace manifold

#endif // MANIFOLD_CAFFDRAM_CONTROLLER_H_
