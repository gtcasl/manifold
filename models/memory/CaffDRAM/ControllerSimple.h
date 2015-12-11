#ifndef MANIFOLD_CAFFDRAM_CONTROLLERSIMPLE_H
#define MANIFOLD_CAFFDRAM_CONTROLLERSIMPLE_H

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


//! This is a simpler version of Controller. It doesn't have flow control and allows to be
//! connected directly to, say, a cache.
class ControllerSimple : public manifold::kernel::Component {
public:
    enum {PORT=0};

    ControllerSimple(int nid, const Dsettings&, bool st_resp=false);
    ~ControllerSimple();

    int get_nid() { return m_nid; }

    template<typename T>
    void handle_request(int, T*);

    void print_config(std::ostream&);
    void print_stats(std::ostream&);


#ifdef CAFFDRAM_TEST
public:
#else
private:
#endif
    unsigned long int processRequest (unsigned long int reqAddr, unsigned long int currentTime);

    //Called when a request is complete.
    template<typename T>
    void request_complete(T*);

    int m_nid; //node id
    const bool m_send_st_response; //send response for stores
    Dsettings* dramSetting;
    Channel** myChannel;

    std::list<uarch::NetworkPacket*> m_completed_reqs; //this is the output buffer holding completed requests.



    //stats
    unsigned stats_ld_misses;
    unsigned stats_stores;
    unsigned stats_max_output_buffer_size;

};



//! Event handler for memory requests.
template<typename T>
void ControllerSimple :: handle_request(int, T* req)
{
    if(req->is_read()) {
#ifdef DBG_CAFFDRAM
cout << "mc received LD, addr= " <<hex<< req->get_addr() <<dec<<endl;
#endif
	//stats
	stats_ld_misses++;


	manifold::kernel::Ticks_t latency = processRequest(req->get_addr(), manifold::kernel::Manifold::NowTicks()); //????????????? using default clock here.
	//The return value of processRequest() is the actual (or absolute) time of when the request
	//is completed, but Sendtick requires time relative to now. So we must pass to SendTick
	//the return value - now.

	//SendTick(OUT, pkt, latency - manifold::kernel::Manifold::NowTicks());
	manifold::kernel::Manifold::Schedule(latency - manifold::kernel::Manifold::NowTicks(), &ControllerSimple::request_complete<T>, this, req);
    }
    else { //write request
#ifdef DBG_CAFFDRAM
cout << "mc received ST, addr= " <<hex<< req->get_addr() <<dec<<endl;
#endif
	//stats
	stats_stores++;
	manifold::kernel::Ticks_t latency = processRequest(req->get_addr(), manifold::kernel::Manifold::NowTicks()); //????????????? using default clock here.
        if(m_send_st_response) {
	    manifold::kernel::Manifold::Schedule(latency - manifold::kernel::Manifold::NowTicks(), &ControllerSimple::request_complete<T>, this, req);
	}
	else
	    delete req;
    }
}


template<typename T>
void ControllerSimple :: request_complete(T* req)
{
    Send(PORT, req);
}




} //namespace caffdram
} //namespace manifold

#endif // MANIFOLD_CAFFDRAM_CONTROLLERSIMPLE_H
