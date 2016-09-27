#ifndef MANIDOLD_DRAM_SIM_H
#define MANIDOLD_DRAM_SIM_H

#include "kernel/component-decl.h"
#include "uarch/DestMap.h"
#include "uarch/networkPacket.h"
#include "uarch/memMsg.h"
#include "DRAMSim2-2.2.2/SystemConfiguration.h"
//#include "DRAMSim2-2.2.2/MemorySystem.h"
#include "DRAMSim2-2.2.2/MultiChannelMemorySystem.h"
#include "DRAMSim2-2.2.2/Transaction.h"
#include <list>
#include <map>
#include <iostream>
//#include <fstream>
#include <assert.h>
#include "kernel/clock.h"


namespace manifold {
namespace dramsim {


struct Dram_sim_settings {
    Dram_sim_settings(const char* dev, const char* sys, int sz, bool st_resp, int credits) :
        dev_filename(dev), mem_sys_filename(sys), size(sz), send_st_resp(st_resp),
	downstream_credits(credits)
	{}
    const char* dev_filename;           // contains settings of the DRAM device
    const char* mem_sys_filename;       // Contains settings of the whole memory       
    int size;			        // Memory size in MB
    //unsigned local_map;	                // Type of local address mapping
    bool send_st_resp;                  // whether responses are sent for stores
    int downstream_credits;            
    manifold::uarch::DestMap* mc_map;
};



class Dram_sim : public manifold::kernel::Component {
public:
    enum {PORT0=0};

    Dram_sim(int nid, const Dram_sim_settings& dram_settings, manifold::kernel::Clock&);
    ~Dram_sim() {};	 

    int get_nid() { return m_nid; }

    template<typename T> 
    void handle_incoming(int, manifold::uarch::NetworkPacket* req); // interface with NI

    void tick();        // registered with clock

    static void Set_msg_types(int mem, int credit) // Set some interface parameters
    {
        assert(Msg_type_set == false);
        MEM_MSG_TYPE = mem;
        CREDIT_MSG_TYPE = credit;
        Msg_type_set = true;
    }

    void print_stats(std::ostream&);
    void print_config(std::ostream&) {};	//TODO

#ifdef DRAMSIM_UTEST
public:
#else
private:
#endif
    struct Request {
        Request(uint64_t c, uint64_t a, bool r, void* e) : r_cycle(c), addr(a), read(r), extra(e) {}
	uint64_t r_cycle; //cycle when the request is first received
	uint64_t addr;
	bool read;  //true for read; false for write
	void* extra; //this is used to store the NetworkPacket for reuse when sending response.
	             //use void* so it can be used for other purposes.
    };


    static int MEM_MSG_TYPE;
    static int CREDIT_MSG_TYPE;
    static bool Msg_type_set;

    int m_nid;  // node id
    //manifold::kernel::Clock& m_clk;
    bool m_send_st_response;  // send response for stores
    int downstream_credits;     // NI credits`

    MultiChannelMemorySystem *mem;  // the original DRAMSim object        

    std::list<Request> m_incoming_reqs;	        // input buffer 
    std::list<Request> m_completed_reqs;  // output buffer
    std::map<uint64_t, std::list<Request> > m_pending_reqs;    // buffer holding active requests,
                                                               // i.e., requests being processed

    /* create and register our callback functions */
    Callback_t *read_cb; 
    Callback_t *write_cb;
    Callback_t *power_cb;

    /* callbacks for read and write */
    void read_complete(unsigned id, uint64_t address, uint64_t done_cycle);
    void write_complete(unsigned id, uint64_t address, uint64_t done_cycle);

    bool limitExceeds();        // check if input has to be stopped because the output is full
    void try_send_reply();     // send reply if there's any and there's credit
    void send_credit();

    //stat
    unsigned stats_n_reads;
    unsigned stats_n_writes;
    unsigned stats_n_reads_sent;
    uint64_t stats_totalMemLat;
    std::map<int, unsigned> stats_n_reads_per_source;
    std::map<int, unsigned> stats_n_writes_per_source;

#ifdef DRAMSIM_UTEST
    unsigned completed_writes; 
#endif
};


//! Event handler for incoming memory request.
template<typename T>
void Dram_sim :: handle_incoming(int, manifold::uarch::NetworkPacket* pkt)
{
    if (pkt->type == CREDIT_MSG_TYPE) {
	downstream_credits++;
	delete pkt;
	return;
    }
        
    assert (pkt->dst == m_nid);

    T* req = (T*)(pkt->data);

    if (req->is_read()) {
	stats_n_reads++;
	stats_n_reads_per_source[pkt->get_src()]++;
#ifdef DBG_DRAMSIM
cout << "@ " << m_clk->NowTicks() << " >>> mc " << m_nid << " received LD, src= " << pkt->get_src() << " addr= " <<hex<< req->get_addr() <<dec<<endl;
#endif
    }
    else {
	stats_n_writes++;
	stats_n_writes_per_source[pkt->get_src()]++;
#ifdef DBG_DRAMSIM
cout << "@ " << m_clk->NowTicks() << " >>> mc " << m_nid << " received ST, src= " << pkt->get_src() << " addr= " <<hex<< req->get_addr() <<dec<<endl;
#endif
    }

	//paddr_t newAddr = m_mc_map->ripAddress(req->addr);

    pkt->set_dst(pkt->get_src());
    pkt->set_dst_port(pkt->get_src_port());
    pkt->set_src(m_nid);
    pkt->set_src_port(0);
    pkt->type = 9;
	
    //put the request in the input buffer
    m_incoming_reqs.push_back(Request(m_clk->NowTicks(), req->get_addr(), req->is_read(), pkt));

}




} // namespace dramsim
} // namespace manifold

#endif

