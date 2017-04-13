#ifndef MANIFOLD_MCPCACHE_L1_CACHE_H
#define MANIFOLD_MCPCACHE_L1_CACHE_H

#include <assert.h>
#include "cache_types.h"
#include "coherence/ClientInterface.h"
#include "kernel/component.h"
#include "hash_table.h"
#include "cache_req.h"
#include "coh_mem_req.h"
#include "uarch/DestMap.h"
#include "uarch/networkPacket.h"

#ifdef LIBKITFOX
#include "uarch/kitfoxCounter.h"
#endif


namespace manifold {
namespace mcp_cache_namespace {

struct L1_cache_settings {
public:
    //    manifold::uarch::DestMap* l2_map;
    unsigned mshr_sz; //mshr size
    int downstream_credits;
};


class L1_cache : public manifold::kernel::Component {
public:

    enum {PORT_PROC=0, PORT_L2, PORT_KITFOX};

    L1_cache (int nid, const cache_settings&, const L1_cache_settings&);
    virtual ~L1_cache (void);

    int get_node_id() const { return node_id; }

    //void handle_processor_request (int, cache_req *request);
    template<typename T>
    void handle_processor_request (int, T* request);

    void handle_peer_and_manager_request (int, manifold::uarch::NetworkPacket* pkt)
    {
    //may receive multiple credits, or one msg one credit
        //assert(m_net_requests.size() == 0);
    m_net_requests.push_back(pkt);
    }

    void tick(); //called at rising edge of clock.

    virtual void send_msg_to_peer_or_l2(Coh_msg* msg);

    hash_entry* get_hash_entry_by_idx(unsigned idx) { return hash_entries[idx]; }

    void print_stats(std::ostream&);

    void set_l2_map(manifold::uarch::DestMap *m);

    static void Set_msg_types(int coh, int credit)
    {
        COH_MSG = coh;
        CREDIT_MSG = credit;
    }

#ifdef LIBKITFOX
    template<typename T>
    void handle_kitfox_proxy_request(int temp, T *kitfox_proxy_request);
#endif

private:
    //void issue_to_client (cache_req *request);
    //void issue_to_manager (cache_req *request);
    //void issue_to_peer (cache_req *request);
    void process_processor_request (cache_req *request, bool first);

    void process_upper_manager_request(Coh_msg* request);
    //void c_um_notify(ClientInterface* client);
    void c_evict_notify(ClientInterface* client);

    void read_reply (cache_req *request);
    void write_reply (cache_req *request);

    void L1_algorithm (hash_entry *e, cache_req *request, bool first);

    void stall (cache_req *req, stall_type_t stall);
    bool stall_buffer_has_match(paddr_t addr);

    void start_eviction (hash_entry*, cache_req *request);
    void wakeup(hash_entry* mshr_entry, cache_req* req);

    void release_mshr_entry(hash_entry*);

    //! called when client enters stable state while processing a request from processor.
    void c_notify(hash_entry* mshr_entry, ClientInterface* client);


    //debug
    void print_mshr();
    void print_stall_buffer();


#ifdef MCP_CACHE_UTEST
public:
#else
protected:
#endif

    void internal_handle_peer_and_manager_request (manifold::uarch::NetworkPacket* pkt);
    void process_peer_and_manager_request(Coh_msg* request);

    virtual void respond_with_default (Coh_msg *request) = 0; //subclass will likely override

    static int COH_MSG; //coherence message type.
    static int CREDIT_MSG; //credit message type.

    int node_id;
    manifold::uarch::DestMap* l2_map;
    hash_table *my_table;
    hash_table *mshr; /** Can view MSHRs as temporary blocks for use while an eviction is pending. Thus, a fully associative hash_table works great. */
    std::map<int, hash_entry*> mshr_map; //map an mshr entry id to an entry in the hash table

    std::vector<ClientInterface*> clients;
    std::vector<hash_entry*> hash_entries; //allows mapping from client to hash_entry
    std::vector<cache_req*> mcp_stalled_req; //stalled request for each manager-client pairing, or in this case, each client;
                                             //with this, after a client finishes a request, we can
                                             //easily identify the stalled request in the stall buffer.
    std::vector<Coh_msg*> mcp_stalled_um_req; //stalled upper manager request for each manager-client pairing, or in this case, each client;

    enum MCP_State {PROCESSING_LOWER, EVICTING}; //this is used in the case of request race when a
                                                 //client in the middle of handling processor request
                         //gets a request from upper manager. The upper manager
                         //request takes priority and after it's finished it may
                         //have affected the previous request, so we need to
                         //remember what we were doing.
    std::vector<MCP_State> mcp_state;

    struct Stall_buffer_entry {
        cache_req* req;
    stall_type_t type;
    manifold::kernel::Ticks_t time; //when it was stalled.
    };

    //std::list<std::pair <cache_req *, stall_type_t> > stalled_client_req_buffer; //holds requests waiting for client to finish.
    std::list<Stall_buffer_entry> stalled_client_req_buffer; //holds requests waiting for client to finish.

    //TODO: Reimplement with hierarchies
    //LIST<pair <cache_req *, stall_type_t> > stalled_peer_message_buffer;


    std::list<cache_req*> m_proc_requests; //store requests from processor
    std::list<manifold::uarch::NetworkPacket*> m_net_requests; //store requests from network


    //flow control
    int m_downstream_credits;
    const int DOWNSTREAM_FULL_CREDITS; //for debug purpose only
    std::list<manifold::uarch::NetworkPacket*> m_downstream_output_buffer; //buffer holding output msg to peer or manager
    void send_msg_after_lookup_time(manifold::uarch::NetworkPacket* pkt);
    virtual void try_send();
    virtual void send_credit_downstream();

    //output prediction
    std::list<manifold::kernel::Ticks_t> m_credit_out_ticks;
    std::list<manifold::kernel::Ticks_t> m_msg_out_ticks;

    //stats
    unsigned long stats_cycles;
    unsigned stats_processor_read_requests;
    unsigned stats_processor_write_requests;
    unsigned stats_hits;
    unsigned stats_misses;
    unsigned stats_MSHR_PEND_STALLs;
    unsigned stats_MSHR_FULL_STALLs;
    unsigned stats_PREV_PEND_STALLs;
    unsigned stats_LRU_BUSY_STALLs;
    unsigned stats_TRANS_STALLs;
    unsigned stats_stall_buffer_max_size;
    unsigned long stats_table_occupancy; //accumulated hash table occupancy
    unsigned stats_table_empty_cycles;

#ifdef LIBKITFOX
    manifold::uarch::cache_counter_t counter;
#endif
};


template<typename T>
void L1_cache :: handle_processor_request(int, T* request)
{
    //stats
    if(stalled_client_req_buffer.size() > stats_stall_buffer_max_size)
    stats_stall_buffer_max_size = stalled_client_req_buffer.size();

    if(request->is_read())
    stats_processor_read_requests++;
    else
    stats_processor_write_requests++;

    cache_req* req = new cache_req;
    req->addr = request->get_addr();
    req->op_type = (request->is_read()) ? OpMemLd : OpMemSt;
    req->preqWrapper = new GenericPreqWrapper<T>(request);

    //might receive multiple processor requests at the same tick.
    //assert(m_proc_requests.size() == 0);
    m_proc_requests.push_back(req);

    //process_processor_request(req, true);
}


#ifdef LIBKITFOX
template <typename T>
void L1_cache :: handle_kitfox_proxy_request(int temp, T *kitfox_proxy_request)
{
    assert(kitfox_proxy_request->get_type() == manifold::uarch::KitFoxType::l1cache_type);
    assert(kitfox_proxy_request->get_id() == node_id);

    kitfox_proxy_request->set_counter(counter);
    counter.clear();

    Send(PORT_KITFOX, kitfox_proxy_request);
}
#endif // LIBKITFOX



} //namespace mcp_cache
} //namespace manifold


#endif //MANIFOLD_MCPCACHE_L1_CACHE_H
