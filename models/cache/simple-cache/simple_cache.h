#ifndef MANIFOLD_SIMPLE_CACHE_SIMPLE_CACHE_H
#define MANIFOLD_SIMPLE_CACHE_SIMPLE_CACHE_H

#include "kernel/component-decl.h"
#include "cache_req.h"
#include "hash_table.h"
#include "uarch/DestMap.h"

namespace manifold {
namespace simple_cache {

struct Simple_cache_parameters {
    int size;
    int assoc;
    int block_size;
    int hit_time;
    int lookup_time;
};

struct Simple_cache_init {
    manifold::uarch::DestMap* dmap;
    bool first_level;
    bool last_level;
};


class simple_cache : public manifold::kernel::Component {
public:
    enum {PORT_LOWER=0, PORT_UPPER}; //the closer to the processor the lower: L1 is lower than L2

    simple_cache (int nid, const char* name, const Simple_cache_parameters&, const Simple_cache_init& init);
    ~simple_cache (void);

    int get_node_id() { return m_node_id; }

    template<typename T>
    void handle_processor_request(int, T* request);

    virtual void handle_request (int port, Cache_req *request);
    virtual void handle_response (int port, Mem_req *request); // for last level: handle response from mem
    void handle_cache_response (int port, Cache_req *request); //upper level cache responds with Cache_req

    void print_stats(std::ostream&);

#ifdef SIMPLE_CACHE_UTEST
public:
#else
protected:
#endif
    hash_table *my_table;
    std::list<mshr_entry*> m_mshr;

    const int m_node_id;
    const char* m_name;
    const bool m_IS_FIRST_LEVEL;
    const bool m_IS_LAST_LEVEL;

    manifold::uarch::DestMap* m_dest_map;


    //stats
    uint64_t stats_n_loads; //number of loads
    uint64_t stats_n_stores; //number of stores
    uint64_t stats_n_load_hits; //load hits
    uint64_t stats_n_store_hits; //load hits

private:
    void write_hit(Cache_req*);
    bool insert_mshr_entry(Cache_req*);
    void free_mshr_entries(paddr_t);
    void reply_to_lower(Cache_req* creq, int);
};



template<typename T>
void simple_cache :: handle_processor_request(int port, T* request)
{
    assert(m_IS_FIRST_LEVEL);
    assert(port == PORT_LOWER);

    Cache_req* req = new Cache_req;
    req->addr = request->get_addr();
    req->op_type = (request->is_read()) ? OpMemLd : OpMemSt;
    req->preqWrapper = new GenericPreqWrapper<T>(request);

    handle_request(port, req);
}


} //namespace simple_cache
} //namespace manifold


#endif // MANIFOLD_SIMPLE_CACHE_SIMPLE_CACHE_H
