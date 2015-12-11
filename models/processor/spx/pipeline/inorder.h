#ifdef USE_QSIM

#ifndef __PIPELINE_INORDER_H__
#define __PIPELINE_INORDER_H__

#include "component.h"
#include "pipeline.h"

namespace manifold {
namespace spx {

class inorder_t : public pipeline_t
{
public:
    inorder_t(spx_core_t *spx_core, libconfig::Config *parser);
    ~inorder_t();
    
    void avoid_memory_deadlock();
    void commit();
    void memory();
    void execute();
    void allocate();
    void frontend();
    void fetch(inst_t *inst);
    void handle_cache_response(int temp, cache_request_t *cache_request);
    void set_config(libconfig::Config *config);    
    
private:
    instQ_t *instQ;
    RS_t *threadQ; // single-entry queue
    STQ_t *STQ;
    LDQ_t *LDQ;
    RF_t *RF;
    std::vector<EX_t*> EX;
};
  
} // namespace spx
} // namespace manifold

#endif

#endif // USE_QSIM
