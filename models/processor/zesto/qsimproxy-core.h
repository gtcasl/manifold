#ifndef MANIFOLD_ZESTO_QSIMPROXY_CORE
#define MANIFOLD_ZESTO_QSIMPROXY_CORE

#ifdef USE_QSIM

#include "zesto-core.h"
//#include "qsim-client.h"
#include "shm.h"

namespace manifold {
namespace zesto {

struct QsimProxyClient_Settings {
    QsimProxyClient_Settings(char* kf, unsigned sz) : key_file(kf), size(sz) {}

    char* key_file; //file name used to generate key for shared mem
    unsigned size; //size of the shared memory segment
};


class qsimproxy_core_t: public core_t
{
public:
    qsimproxy_core_t(const int core_id, char* config, int cpuid, const QsimProxyClient_Settings&);
    ~qsimproxy_core_t();

protected:
    virtual bool fetch_next_pc(md_addr_t* nextPC, struct core_t* tcore);
 
    virtual void fetch_inst(md_inst_t* inst, struct mem_t* mem, const md_addr_t pc, core_t* const tcore);

    int run(int);
 
private:
    const int m_Qsim_cpuid; //QSim cpu ID. In general this is different from core_id.
    SharedMemBuffer* m_shm;
    std::list<Qsim::QueueItem> m_q; //instruction queue
};


} // namespace zesto
} // namespace manifold

#endif //USE_QSIM

#endif // MANIFOLD_ZESTO_QSIMPROXY_CORE
