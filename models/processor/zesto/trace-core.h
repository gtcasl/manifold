#ifndef MANIFOLD_ZESTO_TRACE_CORE
#define MANIFOLD_ZESTO_TRACE_CORE

#include "zesto-core.h"

namespace manifold {
namespace zesto {

class trace_core_t: public core_t
{
  public:
  trace_core_t(const int core_id, char* config, const char* trace);

  protected:
  virtual bool fetch_next_pc(md_addr_t *nextPC, struct core_t * core);
 
  virtual void fetch_inst(md_inst_t *inst, struct mem_t *mem, const md_addr_t pc, core_t * const core);
 
  virtual void pipe_flush_helper(md_addr_t* pc, core_t* c)
  {
    *pc = this->store_nextPC;
  }

  /* pipeline flush and recovery */
  virtual void emergency_recovery(void);

  bool use_stored_nextPC;
  md_addr_t store_nextPC;
    
};



//! @class loop_trace_core_t  trace-core.h
//! This class is a subclass of trace_core_t. Unlike the base class, this class, after reaches the
//! end of the trace file, it will start from the beginning again. Therefore, users must ensure the
//! last instruction in the trace file is a jump instruction. For example, if the last instruction's
//! address is ADDR, and its length is LEN, then append "ADDR+LEN 2 0 eb 1" to the file, where
//! "ADDR+LEN" is the address for the JMP, and "eb" is the opcode of JMP. The jump offset is set to
//! "1". The specific value of the offset is not important.
class loop_trace_core_t: public trace_core_t
{
public:
    loop_trace_core_t(const int core_id, char* config, const char* trace);

protected:
    //override the base version
    virtual bool fetch_next_pc(md_addr_t *nextPC, struct core_t * core);
    virtual void fetch_inst(md_inst_t *inst, struct mem_t *mem, const md_addr_t pc, core_t * const core);

    const char* tracefile_name; //remember the name
};


//! @class mf_trace_core_t  trace-core.h
class mf_trace_core_t: public trace_core_t
{
public:
    mf_trace_core_t(const int core_id, char* config, const char* trace);

protected:
    //override the base version
    virtual bool fetch_next_pc(md_addr_t *nextPC, struct core_t * core);
    virtual void fetch_inst(md_inst_t *inst, struct mem_t *mem, const md_addr_t pc, core_t * const core);

    bool handle_eof();
    void terminate();

    const string tracefile_base_name;
    int tf_idx; //tracefile index
};






} // namespace zesto
} // namespace manifold

#endif // MANIFOLD_ZESTO_TRACE_CORE
