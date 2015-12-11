#ifdef USE_QSIM

#include "pipeline.h"
#include "core.h"
#include "qsim-proxy.h"

using namespace std;
using namespace manifold;
using namespace manifold::spx;

#ifdef LIBKITFOX
using namespace libKitFox;
#endif

pipeline_t::pipeline_t() :
    next_inst(NULL),
    qsim_proxy(NULL),
    Mop_count(0),
    uop_count(0),
    Qsim_osd_state(QSIM_OSD_IDLE),
    inst_cache(NULL),
    inst_tlb(NULL),
    data_tlb(NULL),
    l2_tlb(NULL)
{
}

pipeline_t::~pipeline_t() {}

void pipeline_t::debug_deadlock_inst(inst_t *inst) {
    fprintf(stdout,"SPX_DEADLOCK_DEBUG (core %d) | %lu: uop %lu (Mop %lu)  | ",inst->core->core_id,inst->core->clock_cycle,inst->uop_sequence,inst->Mop_sequence);
    switch(inst->opcode) {
        case QSIM_INST_NULL: fprintf(stdout,"QSIM_INST_NULL"); break;
        case QSIM_INST_INTBASIC: fprintf(stdout,"QSIM_INST_INTBASIC"); break;
        case QSIM_INST_INTMUL: fprintf(stdout,"QSIM_INST_INTMUL"); break;
        case QSIM_INST_INTDIV: fprintf(stdout,"QSIM_INST_INTDIV"); break;
        case QSIM_INST_STACK: fprintf(stdout,"QSIM_INST_STACK"); break;
        case QSIM_INST_BR: fprintf(stdout,"QSIM_INST_BR"); break;
        case QSIM_INST_CALL: fprintf(stdout,"QSIM_INST_CALL"); break;
        case QSIM_INST_RET: fprintf(stdout,"QSIM_INST_RET"); break;
        case QSIM_INST_TRAP: fprintf(stdout,"QSIM_INST_TRAP"); break;
        case QSIM_INST_FPBASIC: fprintf(stdout,"QSIM_INST_FPBASIC"); break;
        case QSIM_INST_FPMUL: fprintf(stdout,"QSIM_INST_FPMUL"); break;
        case QSIM_INST_FPDIV: fprintf(stdout,"QSIM_INST_FPDIV"); break;
        default: break;
    }
    fprintf(stdout,"\n");
    uint64_t src_reg_mask = inst->src_reg;
    for(int i = 0; src_reg_mask > 0; src_reg_mask = src_reg_mask>>1, i++) {
        if(src_reg_mask&0x01)
            fprintf(stdout,"SPX_DEADLOCK_DEBUG (core %d) | %lu:       src reg %d\n",inst->core->core_id,inst->core->clock_cycle,i);
    }
    uint8_t src_flag_mask = inst->src_flag;
    for(int i = 0; src_flag_mask >0; src_flag_mask = src_flag_mask>>1, i++) {
        if(src_flag_mask&0x01)
            fprintf(stdout,"SPX_DEADLOCK_DEBUG (core %d) | %lu:       src flag %d\n",inst->core->core_id,inst->core->clock_cycle,i);
    }
    for(map<uint64_t,inst_t*>::iterator it = inst->src_dep.begin(); it != inst->src_dep.end(); it++) {
        fprintf(stdout,"SPX_DEADLOCK_DEBUG (core %d) | %lu:       mem dep 0x%lx\n",inst->core->core_id,inst->core->clock_cycle,it->second->data.paddr);
    }
    if(inst->memcode != SPX_MEM_NONE) {
        fprintf(stdout,"SPX_DEADLOCK_DEBUG (core %d) | %lu:       mem %s 0x%lx (request was sent at clk=%lu)\n",inst->core->core_id,inst->core->clock_cycle,(inst->memcode==SPX_MEM_ST)?"STORE":"LOAD",inst->data.paddr,inst->memory_request_time_stamp);
    }
    uint64_t dest_reg_mask = inst->dest_reg;
    for(int i = 0; dest_reg_mask > 0; dest_reg_mask = dest_reg_mask>>1, i++) {
        if(dest_reg_mask&0x01)
            fprintf(stdout,"SPX_DEADLOCK_DEBUG (core %d) | %lu:       dest reg %d\n",inst->core->core_id,inst->core->clock_cycle,i);
    }
    uint8_t dest_flag_mask = inst->dest_flag;
    for(int i = 0; dest_flag_mask > 0; dest_flag_mask = dest_flag_mask>>1, i++) {
        if(dest_flag_mask&0x01)
            fprintf(stdout,"SPX_DEADLOCK_DEBUG (core %d) | %lu:       src flag %d\n",inst->core->core_id,inst->core->clock_cycle,i);
    }
    if(inst->src_dep.size()||inst->mem_disamb.size())
        fprintf(stdout,"SPX_DEADLOCK_DEBUG (core %d) | %lu:       depends on ",inst->core->core_id,inst->core->clock_cycle);
    for(map<uint64_t,inst_t*>::iterator it = inst->src_dep.begin(); it != inst->src_dep.end(); it++) {
        fprintf(stdout," uop %lu (Mop %lu),",it->second->uop_sequence,it->second->Mop_sequence);
    }
    for(vector<inst_t*>::iterator it = inst->mem_disamb.begin(); it != inst->mem_disamb.end(); it++) {
        fprintf(stdout," uop %lu (Mop %lu),",(*it)->uop_sequence,(*it)->Mop_sequence);
    }
    if(inst->src_dep.size()||inst->mem_disamb.size())
        fprintf(stdout,"\n");
    fprintf(stdout,"%d incomplete uops of the same Mop",inst->Mop_head->Mop_length);
    fprintf(stdout,"\n");
}

void pipeline_t::Qsim_inst_cb(int core_id, uint64_t vaddr, uint64_t paddr, uint8_t len, const uint8_t *bytes, enum inst_type type) {
    //if(!next_inst||(Qsim_osd_state < QSIM_OSD_ACTIVE)) return;
    if(!next_inst) return;
  
#ifdef SPX_QSIM_DEBUG
    fprintf(stdout,"SPX_QSIM_DEBUG (core %d): uop %lu (Mop %lu) | ",core_id,next_inst->uop_sequence,next_inst->Mop_sequence);
    switch(type) {
        case QSIM_INST_NULL: fprintf(stdout,"QSIM_INST_NULL"); break;
        case QSIM_INST_INTBASIC: fprintf(stdout,"QSIM_INST_INTBASIC"); break;
        case QSIM_INST_INTMUL: fprintf(stdout,"QSIM_INST_INTMUL"); break;
        case QSIM_INST_INTDIV: fprintf(stdout,"QSIM_INST_INTDIV"); break;
        case QSIM_INST_STACK: fprintf(stdout,"QSIM_INST_STACK"); break;
        case QSIM_INST_BR: fprintf(stdout,"QSIM_INST_BR"); break;
        case QSIM_INST_CALL: fprintf(stdout,"QSIM_INST_CALL"); break;
        case QSIM_INST_RET: fprintf(stdout,"QSIM_INST_RET"); break;
        case QSIM_INST_TRAP: fprintf(stdout,"QSIM_INST_TRAP"); break;
        case QSIM_INST_FPBASIC: fprintf(stdout,"QSIM_INST_FPBASIC"); break;
        case QSIM_INST_FPMUL: fprintf(stdout,"QSIM_INST_FPMUL"); break;
        case QSIM_INST_FPDIV: fprintf(stdout,"QSIM_INST_FPDIV"); break;
        default: break;
    }
    fprintf(stdout,"\n");
#endif
  
    next_inst->opcode = type;
    next_inst->op.vaddr = vaddr&config.mem_addr_mask;
    next_inst->op.paddr = paddr&config.mem_addr_mask;
    Qsim_cb_status = SPX_QSIM_INST_CB;
}

void pipeline_t::Qsim_mem_cb(int core_id, uint64_t vaddr, uint64_t paddr, uint8_t size, int type) {
    //if(!next_inst||(Qsim_osd_state < QSIM_OSD_ACTIVE)) return;
    if(!next_inst) return;
  
    if(type) { // store
        // there can't be mem_cb after mem_cb or dest_reg/flag_cb
        if(Qsim_cb_status > SPX_QSIM_MEM_CB) {
            Qsim_post_cb(next_inst);
            fetch(next_inst);
      
            stats.uop_count++;
            stats.interval.uop_count++;
            next_inst = new inst_t(next_inst,core,Mop_count,++uop_count,SPX_MEM_ST);
      
/*#ifdef SPX_QSIM_DEBUG
            fprintf(stdout,"SPX_QSIM_DEBUG (core %d): uop %lu (Mop %lu) | ",core_id,next_inst->uop_sequence,next_inst->Mop_sequence);
            switch(next_inst->opcode) {
                case QSIM_INST_NULL: fprintf(stdout,"QSIM_INST_NULL"); break;
                case QSIM_INST_INTBASIC: fprintf(stdout,"QSIM_INST_INTBASIC"); break;
                case QSIM_INST_INTMUL: fprintf(stdout,"QSIM_INST_INTMUL"); break;
                case QSIM_INST_INTDIV: fprintf(stdout,"QSIM_INST_INTDIV"); break;
                case QSIM_INST_STACK: fprintf(stdout,"QSIM_INST_STACK"); break;
                case QSIM_INST_BR: fprintf(stdout,"QSIM_INST_BR"); break;
                case QSIM_INST_CALL: fprintf(stdout,"QSIM_INST_CALL"); break;
                case QSIM_INST_RET: fprintf(stdout,"QSIM_INST_RET"); break;
                case QSIM_INST_TRAP: fprintf(stdout,"QSIM_INST_TRAP"); break;
                case QSIM_INST_FPBASIC: fprintf(stdout,"QSIM_INST_FPBASIC"); break;
                case QSIM_INST_FPMUL: fprintf(stdout,"QSIM_INST_FPMUL"); break;
                case QSIM_INST_FPDIV: fprintf(stdout,"QSIM_INST_FPDIV"); break;
                default: break;
            }
            fprintf(stdout,"\n");
#endif*/
        }
        else
            next_inst->memcode = SPX_MEM_ST;
    
#ifdef SPX_QSIM_DEBUG
        fprintf(stdout,"SPX_QSIM_DEBUG (core %d):      mem STORE | 0x%lx\n",core_id,paddr);
#endif
    
        next_inst->data.vaddr = vaddr&config.mem_addr_mask;
        next_inst->data.paddr = paddr&config.mem_addr_mask;
        Qsim_cb_status = SPX_QSIM_ST_CB;
    }
    else { // load
        // there can't be mem_cb after mem_cb or dest_reg/flag_cb
        if(Qsim_cb_status > SPX_QSIM_MEM_CB) {
            Qsim_post_cb(next_inst);
            fetch(next_inst);
      
            stats.uop_count++;
            stats.interval.uop_count++;
            next_inst = new inst_t(next_inst,core,Mop_count,++uop_count,SPX_MEM_LD);
      
/*#ifdef SPX_QSIM_DEBUG
            fprintf(stdout,"SPX_QSIM_DEBUG (core %d): uop %lu (Mop %lu) | ",core_id,next_inst->uop_sequence,next_inst->Mop_sequence);
            switch(next_inst->opcode) {
                case QSIM_INST_NULL: fprintf(stdout,"QSIM_INST_NULL"); break;
                case QSIM_INST_INTBASIC: fprintf(stdout,"QSIM_INST_INTBASIC"); break;
                case QSIM_INST_INTMUL: fprintf(stdout,"QSIM_INST_INTMUL"); break;
                case QSIM_INST_INTDIV: fprintf(stdout,"QSIM_INST_INTDIV"); break;
                case QSIM_INST_STACK: fprintf(stdout,"QSIM_INST_STACK"); break;
                case QSIM_INST_BR: fprintf(stdout,"QSIM_INST_BR"); break;
                case QSIM_INST_CALL: fprintf(stdout,"QSIM_INST_CALL"); break;
                case QSIM_INST_RET: fprintf(stdout,"QSIM_INST_RET"); break;
                case QSIM_INST_TRAP: fprintf(stdout,"QSIM_INST_TRAP"); break;
                case QSIM_INST_FPBASIC: fprintf(stdout,"QSIM_INST_FPBASIC"); break;
                case QSIM_INST_FPMUL: fprintf(stdout,"QSIM_INST_FPMUL"); break;
                case QSIM_INST_FPDIV: fprintf(stdout,"QSIM_INST_FPDIV"); break;
                default: break;
            }
            fprintf(stdout,"\n");
#endif*/
        }
        else
            next_inst->memcode = SPX_MEM_LD;
    
#ifdef SPX_QSIM_DEBUG
        fprintf(stdout,"SPX_QSIM_DEBUG (core %d):      mem LOAD | 0x%lx\n",core_id,paddr);
#endif
    
        next_inst->data.vaddr = vaddr&config.mem_addr_mask;
        next_inst->data.paddr = paddr&config.mem_addr_mask;
        Qsim_cb_status = SPX_QSIM_LD_CB;
    }
}

void pipeline_t::Qsim_reg_cb(int core_id, int reg, uint8_t size, int type) {
    //if(!next_inst||(Qsim_osd_state < QSIM_OSD_ACTIVE)) return;
    if(!next_inst) return;
  
    if(type) { // dest
        if(size > 0) { // regs
            // there can be multiple dest regs (e.g., div/mul) - don't use >= sign
            if(Qsim_cb_status > SPX_QSIM_DEST_REG_CB) {
                Qsim_post_cb(next_inst);
                fetch(next_inst);
        
                stats.uop_count++;
                stats.interval.uop_count++;
                next_inst = new inst_t(next_inst,core,Mop_count,++uop_count,SPX_MEM_NONE);
        
/*#ifdef SPX_QSIM_DEBUG
                fprintf(stdout,"SPX_QSIM_DEBUG (core %d): uop %lu (Mop %lu) | ",core_id,next_inst->uop_sequence,next_inst->Mop_sequence);
                switch(next_inst->opcode) {
                    case QSIM_INST_NULL: fprintf(stdout,"QSIM_INST_NULL"); break;
                    case QSIM_INST_INTBASIC: fprintf(stdout,"QSIM_INST_INTBASIC"); break;
                    case QSIM_INST_INTMUL: fprintf(stdout,"QSIM_INST_INTMUL"); break;
                    case QSIM_INST_INTDIV: fprintf(stdout,"QSIM_INST_INTDIV"); break;
                    case QSIM_INST_STACK: fprintf(stdout,"QSIM_INST_STACK"); break;
                    case QSIM_INST_BR: fprintf(stdout,"QSIM_INST_BR"); break;
                    case QSIM_INST_CALL: fprintf(stdout,"QSIM_INST_CALL"); break;
                    case QSIM_INST_RET: fprintf(stdout,"QSIM_INST_RET"); break;
                    case QSIM_INST_TRAP: fprintf(stdout,"QSIM_INST_TRAP"); break;
                    case QSIM_INST_FPBASIC: fprintf(stdout,"QSIM_INST_FPBASIC"); break;
                    case QSIM_INST_FPMUL: fprintf(stdout,"QSIM_INST_FPMUL"); break;
                    case QSIM_INST_FPDIV: fprintf(stdout,"QSIM_INST_FPDIV"); break;
                    default: break;
                }
                fprintf(stdout,"\n");
#endif*/
            }
            else if(Qsim_cb_status == SPX_MEM_ST)
                next_inst->split_store = true;
            else if(Qsim_cb_status == SPX_MEM_LD)
                next_inst->split_load = true;
      
#ifdef SPX_QSIM_DEBUG
            fprintf(stdout,"SPX_QSIM_DEBUG (core %d):      reg dest %d\n",core_id,reg);
#endif

            next_inst->dest_reg |= (0x01<<reg); // bit mask the dest reg position
            Qsim_cb_status = SPX_QSIM_DEST_REG_CB;
        }
        else { // flags
            if(Qsim_cb_status >= SPX_QSIM_DEST_FLAG_CB) {
                Qsim_post_cb(next_inst);
                fetch(next_inst);
        
                stats.uop_count++;
                stats.interval.uop_count++;
                next_inst = new inst_t(next_inst,core,Mop_count,++uop_count,SPX_MEM_NONE);
        
/*#ifdef SPX_QSIM_DEBUG
                fprintf(stdout,"SPX_QSIM_DEBUG (core %d): uop %lu (Mop %lu) | ",core_id,next_inst->uop_sequence,next_inst->Mop_sequence);
                switch(next_inst->opcode) {
                    case QSIM_INST_NULL: fprintf(stdout,"QSIM_INST_NULL"); break;
                    case QSIM_INST_INTBASIC: fprintf(stdout,"QSIM_INST_INTBASIC"); break;
                    case QSIM_INST_INTMUL: fprintf(stdout,"QSIM_INST_INTMUL"); break;
                    case QSIM_INST_INTDIV: fprintf(stdout,"QSIM_INST_INTDIV"); break;
                    case QSIM_INST_STACK: fprintf(stdout,"QSIM_INST_STACK"); break;
                    case QSIM_INST_BR: fprintf(stdout,"QSIM_INST_BR"); break;
                    case QSIM_INST_CALL: fprintf(stdout,"QSIM_INST_CALL"); break;
                    case QSIM_INST_RET: fprintf(stdout,"QSIM_INST_RET"); break;
                    case QSIM_INST_TRAP: fprintf(stdout,"QSIM_INST_TRAP"); break;
                    case QSIM_INST_FPBASIC: fprintf(stdout,"QSIM_INST_FPBASIC"); break;
                    case QSIM_INST_FPMUL: fprintf(stdout,"QSIM_INST_FPMUL"); break;
                    case QSIM_INST_FPDIV: fprintf(stdout,"QSIM_INST_FPDIV"); break;
                    default: break;
                }
                fprintf(stdout,"\n");
#endif*/
            }

#ifdef SPX_QSIM_DEBUG
            fprintf(stdout,"SPX_QSIM_DEBUG (core %d):      flag dest 0x%lx\n",core_id,reg);
#endif
     
            next_inst->dest_flag = (0x3f&reg); // bit maks the dest flags position
            Qsim_cb_status = SPX_QSIM_DEST_FLAG_CB;
        }
    }
    else { // src
        if(size > 0) { // regs
            // there can be multiple src regs - don't use >= sign
            if(Qsim_cb_status > SPX_QSIM_SRC_REG_CB) {
                Qsim_post_cb(next_inst);
                fetch(next_inst);
                
                stats.uop_count++;
                stats.interval.uop_count++;
                next_inst = new inst_t(next_inst,core,Mop_count,++uop_count,SPX_MEM_NONE);

/*#ifdef SPX_QSIM_DEBUG
                fprintf(stdout,"SPX_QSIM_DEBUG (core %d): uop %lu (Mop %lu) | ",core_id,next_inst->uop_sequence,next_inst->Mop_sequence);
                switch(next_inst->opcode) {
                    case QSIM_INST_NULL: fprintf(stdout,"QSIM_INST_NULL"); break;
                    case QSIM_INST_INTBASIC: fprintf(stdout,"QSIM_INST_INTBASIC"); break;
                    case QSIM_INST_INTMUL: fprintf(stdout,"QSIM_INST_INTMUL"); break;
                    case QSIM_INST_INTDIV: fprintf(stdout,"QSIM_INST_INTDIV"); break;
                    case QSIM_INST_STACK: fprintf(stdout,"QSIM_INST_STACK"); break;
                    case QSIM_INST_BR: fprintf(stdout,"QSIM_INST_BR"); break;
                    case QSIM_INST_CALL: fprintf(stdout,"QSIM_INST_CALL"); break;
                    case QSIM_INST_RET: fprintf(stdout,"QSIM_INST_RET"); break;
                    case QSIM_INST_TRAP: fprintf(stdout,"QSIM_INST_TRAP"); break;
                    case QSIM_INST_FPBASIC: fprintf(stdout,"QSIM_INST_FPBASIC"); break;
                    case QSIM_INST_FPMUL: fprintf(stdout,"QSIM_INST_FPMUL"); break;
                    case QSIM_INST_FPDIV: fprintf(stdout,"QSIM_INST_FPDIV"); break;
                    default: break;
                }
                fprintf(stdout,"\n");
#endif*/
            }
      
#ifdef SPX_QSIM_DEBUG
            fprintf(stdout,"SPX_QSIM_DEBUG (core %d):      reg src %d\n",core_id,reg);
#endif
      
            next_inst->src_reg |= (0x01<<reg); // bit mask the src reg position
            Qsim_cb_status = SPX_QSIM_SRC_REG_CB;
        }
        else { // flags
            if(Qsim_cb_status >= SPX_QSIM_SRC_FLAG_CB) {
                Qsim_post_cb(next_inst);
                fetch(next_inst);
                
                stats.uop_count++;
                stats.interval.uop_count++;
                next_inst = new inst_t(next_inst,core,Mop_count,++uop_count,SPX_MEM_NONE);
        
/*#ifdef SPX_QSIM_DEBUG
                fprintf(stdout,"SPX_QSIM_DEBUG (core %d): uop %lu (Mop %lu) | ",core_id,next_inst->uop_sequence,next_inst->Mop_sequence);
                switch(next_inst->opcode) {
                    case QSIM_INST_NULL: fprintf(stdout,"QSIM_INST_NULL"); break;
                    case QSIM_INST_INTBASIC: fprintf(stdout,"QSIM_INST_INTBASIC"); break;
                    case QSIM_INST_INTMUL: fprintf(stdout,"QSIM_INST_INTMUL"); break;
                    case QSIM_INST_INTDIV: fprintf(stdout,"QSIM_INST_INTDIV"); break;
                    case QSIM_INST_STACK: fprintf(stdout,"QSIM_INST_STACK"); break;
                    case QSIM_INST_BR: fprintf(stdout,"QSIM_INST_BR"); break;
                    case QSIM_INST_CALL: fprintf(stdout,"QSIM_INST_CALL"); break;
                    case QSIM_INST_RET: fprintf(stdout,"QSIM_INST_RET"); break;
                    case QSIM_INST_TRAP: fprintf(stdout,"QSIM_INST_TRAP"); break;
                    case QSIM_INST_FPBASIC: fprintf(stdout,"QSIM_INST_FPBASIC"); break;
                    case QSIM_INST_FPMUL: fprintf(stdout,"QSIM_INST_FPMUL"); break;
                    case QSIM_INST_FPDIV: fprintf(stdout,"QSIM_INST_FPDIV"); break;
                    default: break;
                }
                fprintf(stdout,"\n");
#endif*/
            }
      
#ifdef SPX_QSIM_DEBUG
            fprintf(stdout,"SPX_QSIM_DEBUG (core %d):      flag src 0x%lx\n",core_id,reg);
#endif
      
            next_inst->src_flag = (0x3f&reg); // bit mask the src flags position
            Qsim_cb_status = SPX_QSIM_SRC_FLAG_CB;
        }
    }
}

void pipeline_t::Qsim_post_cb(inst_t* inst) {
    switch(inst->opcode) {
        case QSIM_INST_NULL: { // do not split mov
            inst->excode = SPX_FU_MOV;
            inst->split_store = false;
            inst->split_load = false;
            break;
        }
        case QSIM_INST_INTBASIC: {
            inst->excode = SPX_FU_INT;
            break;
        }
        case QSIM_INST_INTMUL: {
            inst->excode = SPX_FU_MUL;
            break;
        }
        case QSIM_INST_INTDIV: {
            inst->excode = SPX_FU_MUL;
            break;
        }
        case QSIM_INST_STACK: { // do not split stack
            inst->excode = SPX_FU_INT;
            inst->split_store = false;
            inst->split_load = false;
            break;
        }
        case QSIM_INST_BR: { // do not split branch
            inst->excode = SPX_FU_BR;
            inst->split_store = false;
            inst->split_load = false;
            break;
        }
        case QSIM_INST_CALL: { // do not split call
            inst->excode = SPX_FU_INT;
            inst->split_store = false;
            inst->split_load = false;
            break;
        }
        case QSIM_INST_RET: { // do not split return
            inst->excode = SPX_FU_INT;
            inst->split_store = false;
            inst->split_load = false;
            break;
        }
        case QSIM_INST_TRAP: { // do not split trap
            inst->excode = SPX_FU_INT;
            inst->split_store = false;
            inst->split_load = false;
            break;
        }
        case QSIM_INST_FPBASIC: {
            inst->excode = SPX_FU_FP;
            break;
        }
        case QSIM_INST_FPMUL: {
            inst->excode = SPX_FU_MUL;
            break;
        }
        case QSIM_INST_FPDIV: {
            inst->excode = SPX_FU_MUL;
            break;
        }
        default: {
            inst->excode = SPX_FU_INT;
            break;
        }
    }

    switch(inst->memcode) {
        case SPX_MEM_LD: {
            if(inst->split_load) {
                stats.uop_count++;
                stats.interval.uop_count++;
            
                // create another inst that computes with a memory operand
                next_inst = new inst_t(next_inst,core,Mop_count,++uop_count,SPX_MEM_NONE);
                next_inst->excode = inst->excode;
                next_inst->src_dep.insert(pair<uint64_t,inst_t*>(inst->uop_sequence,inst));
                next_inst->data.vaddr = 0;
                next_inst->data.paddr = 0;

                // previous inst is a load inst
                inst->excode = SPX_FU_LD;
                inst->dest_dep.insert(pair<uint64_t,inst_t*>(next_inst->uop_sequence,next_inst));
                fetch(inst);
            }
            else
                inst->excode = SPX_FU_LD;
            break;
        }
        case SPX_MEM_ST: {
            if(inst->split_store) {
                stats.uop_count++;
                stats.interval.uop_count++;
                
                // create another inst that stores the result
                next_inst = new inst_t(next_inst,core,Mop_count,++uop_count,SPX_MEM_ST);
                next_inst->excode = SPX_FU_ST;
                next_inst->src_dep.insert(pair<uint64_t,inst_t*>(inst->uop_sequence,inst));
                next_inst->src_reg |= inst->dest_reg; // this store depends on the result of a previous inst
                // previous inst is a compute inst
                inst->memcode = SPX_MEM_NONE;
                inst->dest_dep.insert(pair<uint64_t,inst_t*>(next_inst->uop_sequence,next_inst));
                inst->data.vaddr = 0;
                inst->data.paddr = 0;
                fetch(inst);
            }
            else
                inst->excode = SPX_FU_ST;
            break;
        }
        default: { break; }
    }
}

// This function breaks down the inst whose Mop is comprised of more than 4 uops.
// uop of the same Mop becomes an independent inst.
void pipeline_t::handle_long_inst(inst_t *inst) {
    inst_t *uop = inst->Mop_head;
    while(uop) {
        uop->Mop_head = uop;
        uop->Mop_length = 1;
        uop->is_long_inst = true;

        // Cut out the connection between uops that belong to the same Mop
        // so that they can independently be scheduled.        
        if(uop->prev_inst) uop->prev_inst->next_inst = NULL;
        uop->prev_inst = NULL;
        uop = uop->next_inst;
    }
}

// This breaks down the uop chain of the deadlock inst
void pipeline_t::handle_deadlock_inst(inst_t *inst) {
    inst_t *uop = inst->Mop_head;
    while(uop) {
        uop->Mop_head = uop;
        if(uop->inflight) uop->Mop_length = 1;
        else uop->Mop_length = 0;
        
        // Cut out the connection between uops that belong to the same Mop
        // so that they can independently be scheduled.        
        if(uop->prev_inst) uop->prev_inst->next_inst = NULL;
        uop->prev_inst = NULL;
        uop = uop->next_inst;
    }
}

bool pipeline_t::is_nop(inst_t *inst) {
    if((inst->opcode == QSIM_INST_NULL)&&
       (inst->memcode == SPX_MEM_NONE)&&
       (inst->Mop_head == inst)&&
       (inst->src_reg == 0)&&
       (inst->src_flag == 0)&&
       (inst->dest_reg == 0)&&
       (inst->dest_flag == 0)) { 
#ifdef SPX_QSIM_DEBUG
        fprintf(stdout,"SPX_QSIM_DEBUG (core %d):      nop\n",core->core_id);
#endif
        return true; 
    }
    return false;
}

void pipeline_t::set_qsim_proxy(spx_qsim_proxy_t *QsimProxy)
{
    qsim_proxy = QsimProxy;
}

#endif // USE_QSIM
