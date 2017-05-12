#ifdef USE_QSIM

#include <limits.h>
#include "component.h"
#include "core.h"

using namespace std;
using namespace manifold::spx;

int manifold::spx::QSIM_N_REGS = 0;

instQ_t::instQ_t(pipeline_t *pl, int instQ_size, int cache_line_width) :
    size(instQ_size), occupancy(0), fetch_addr(0), pipeline(pl)
{
    fetch_mask = log2(cache_line_width);
    queue.reserve(size*6);
}

instQ_t::~instQ_t()
{
    while(queue.size()) { queue.erase(queue.begin()); }
    queue.clear();
}

void instQ_t::push_back(inst_t *inst)
{
    inst->inflight = true;
    queue.push_back(inst);

    if(inst->is_head) { occupancy++; }

#ifdef SPX_QSIM_DEBUG_
    fprintf(stdout,"SPX_QSIM_DEBUG (core %d) | %lu: instQ.push_back (%d/%d) uop %lu Mop %lu  | ",pipeline->core->core_id,pipeline->core->clock_cycle,occupancy,size,inst->uop_sequence,inst->Mop_sequence);
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
            fprintf(stdout,"SPX_QSIM_DEBUG (core %d) | %lu:       src reg %d\n",pipeline->core->core_id,pipeline->core->clock_cycle,i);
    }
    uint8_t src_flag_mask = inst->src_flag;
    for(int i = 0; src_flag_mask >0; src_flag_mask = src_flag_mask>>1, i++) {
        if(src_flag_mask&0x01)
            fprintf(stdout,"SPX_QSIM_DEBUG (core %d) | %lu:       src flag %d\n",pipeline->core->core_id,pipeline->core->clock_cycle,i);
    }
    for(map<uint64_t,inst_t*>::iterator it = inst->src_dep.begin(); it != inst->src_dep.end(); it++) {
        fprintf(stdout,"SPX_QSIM_DEBUG (core %d) | %lu:       mem dep 0x%lx\n",pipeline->core->core_id,pipeline->core->clock_cycle,it->second->data.paddr);
    }
    if(inst->memcode != SPX_MEM_NONE) {
        fprintf(stdout,"SPX_QSIM_DEBUG (core %d) | %lu:       mem %s 0x%lx\n",pipeline->core->core_id,pipeline->core->clock_cycle,(inst->memcode==SPX_MEM_ST)?"STORE":"LOAD",inst->data.paddr);
    }
    uint64_t dest_reg_mask = inst->dest_reg;
    for(int i = 0; dest_reg_mask > 0; dest_reg_mask = dest_reg_mask>>1, i++) {
        if(dest_reg_mask&0x01)
            fprintf(stdout,"SPX_QSIM_DEBUG (core %d) | %lu:       dest reg %d\n",pipeline->core->core_id,pipeline->core->clock_cycle,i);
    }
    uint8_t dest_flag_mask = inst->dest_flag;
    for(int i = 0; dest_flag_mask > 0; dest_flag_mask = dest_flag_mask>>1, i++) {
        if(dest_flag_mask&0x01)
            fprintf(stdout,"SPX_QSIM_DEBUG (core %d) | %lu:       dest flag %d\n",pipeline->core->core_id,pipeline->core->clock_cycle,i);
    }
#else
#ifdef SPX_DEBUG
    fprintf(stdout,"SPX_DEBUG (core %d) | %lu: instQ.push_back (%d/%d) uop %lu Mop %lu  | ",pipeline->core->core_id,pipeline->core->clock_cycle,occupancy,size,inst->uop_sequence,inst->Mop_sequence);
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
            fprintf(stdout,"SPX_DEBUG (core %d) | %lu:       src reg %d\n",pipeline->core->core_id,pipeline->core->clock_cycle,i);
    }
    uint8_t src_flag_mask = inst->src_flag;
    for(int i = 0; src_flag_mask >0; src_flag_mask = src_flag_mask>>1, i++) {
        if(src_flag_mask&0x01)
            fprintf(stdout,"SPX_DEBUG (core %d) | %lu:       src flag %d\n",pipeline->core->core_id,pipeline->core->clock_cycle,i);
    }
    for(map<uint64_t,inst_t*>::iterator it = inst->src_dep.begin(); it != inst->src_dep.end(); it++) {
        fprintf(stdout,"SPX_DEBUG (core %d) | %lu:       mem dep 0x%lx\n",pipeline->core->core_id,pipeline->core->clock_cycle,it->second->data.paddr);
    }
    if(inst->memcode != SPX_MEM_NONE) {
        fprintf(stdout,"SPX_DEBUG (core %d) | %lu:       mem %s 0x%lx\n",pipeline->core->core_id,pipeline->core->clock_cycle,(inst->memcode==SPX_MEM_ST)?"STORE":"LOAD",inst->data.paddr);
    }
    uint64_t dest_reg_mask = inst->dest_reg;
    for(int i = 0; dest_reg_mask > 0; dest_reg_mask = dest_reg_mask>>1, i++) {
        if(dest_reg_mask&0x01)
            fprintf(stdout,"SPX_DEBUG (core %d) | %lu:       dest reg %d\n",pipeline->core->core_id,pipeline->core->clock_cycle,i);
    }
    uint8_t dest_flag_mask = inst->dest_flag;
    for(int i = 0; dest_flag_mask > 0; dest_flag_mask = dest_flag_mask>>1, i++) {
        if(dest_flag_mask&0x01)
            fprintf(stdout,"SPX_DEBUG (core %d) | %lu:       dest flag %d\n",pipeline->core->core_id,pipeline->core->clock_cycle,i);
    }
#endif
#endif

#ifdef LIBKITFOX
    // Assume different Mops are separate instruction cache access.
    /*if(fetch_addr != (inst->op.vaddr>>fetch_mask)) {
        fetch_addr = inst->op.vaddr>>fetch_mask;
    }*/
    if(inst->is_head) {
        if(pipeline->inst_tlb) { pipeline->inst_tlb->access(inst->op.vaddr,false); }
        if(pipeline->inst_cache) { pipeline->inst_cache->access(inst->op.paddr,false); }

        pipeline->counter.latch_ic2ib.switching++; // inst_cache to instQ latch
        pipeline->counter.pc.switching++; // program counter
        pipeline->counter.inst_buffer.write++; // inst_buffer write

        // Branch predictors are read in parallel before insts are decoded.
        pipeline->counter.predictor_chooser.read++;
        pipeline->counter.global_predictor.read++;
        pipeline->counter.l1_local_predictor.read++;
        pipeline->counter.l2_local_predictor.read++;
        pipeline->counter.l1_btb.read++;
    }
#endif
}

inst_t* instQ_t::get_front()
{
    vector<inst_t*>::iterator it = queue.begin();
    while(it != queue.end()) {
        inst_t *inst = *it;
        return inst;
    }
    return NULL; // Empty instQ
}

void instQ_t::pop_front() // It must be called after get_front().
{
    inst_t *inst = *queue.begin();

#ifdef SPX_DEBUG
    fprintf(stdout,"SPX_DEBUG (core %d) | %lu: instQ.pop_front (%d/%d) uop %lu (Mop %lu)\n",pipeline->core->core_id,pipeline->core->clock_cycle,occupancy-1,size,inst->uop_sequence,inst->Mop_sequence);
#endif

    queue.erase(queue.begin());
    if(inst->is_tail) { occupancy--; }

#ifdef LIBKITFOX
    if(inst->is_head) {
        pipeline->counter.inst_buffer.read++;
        pipeline->counter.latch_ib2id.switching++; // InstQ to decoder latch
    }

    // There isn't a decoder in the SPX model. Assume an instruction is decoded when instQ dequeue happens. inst_decoder is assumed to switch when there is an inst leaving the instQ.
    pipeline->counter.inst_decoder.switching++;

    // Operands decode
    uint64_t src_reg_mask = inst->src_reg;
    for(int i = 0; src_reg_mask > 0; src_reg_mask = src_reg_mask>>1, i++)
        pipeline->counter.operand_decoder.switching++;
    uint64_t dest_reg_mask = inst->dest_reg;
    for(int i = 0; dest_reg_mask > 0; dest_reg_mask = dest_reg_mask>>1, i++)
        pipeline->counter.operand_decoder.switching++;

    if(inst->is_long_inst)
        pipeline->counter.uop_sequencer.switching++;

    pipeline->counter.latch_id2uq.switching+=2; // decoder to queue latch (2 means decode depth) | outorder
    pipeline->counter.latch_id2iq.switching+=2; // decoder to queue latch (2 means decode depth) | inorder

    if(inst->opcode == QSIM_INST_RET)
        pipeline->counter.ras.read++;

    // There isn't a uop queue in the SPX model. Assume a uop enqueue/dequeue when instQ dequeue happens.
    pipeline->counter.uop_queue.write++;
    pipeline->counter.uop_queue.read++;
    pipeline->counter.latch_uq2rr.switching++; // queue to register renaming unit latch
#endif
}

bool instQ_t::is_available()
{
    return ((occupancy < size)&&((int)queue.size() < size*6));
}





ROB_t::ROB_t(pipeline_t *pl, int ROB_size) :
    size(ROB_size), /*occupancy(0),*/ pipeline(pl)
{
    queue.reserve(size);
}

ROB_t::~ROB_t()
{
    while(queue.size()) { queue.erase(queue.begin()); }
    queue.clear();
}

void ROB_t::push_back(inst_t *inst)
{
#ifdef SPX_DEBUG
    fprintf(stdout,"SPX_DEBUG (core %d) | %lu: ROB.push_back (%d/%d) uop %lu (Mop %lu)\n",pipeline->core->core_id,pipeline->core->clock_cycle,queue.size()+1,size,inst->uop_sequence,inst->Mop_sequence);
#endif

    queue.push_back(inst);
    //occupancy++;

#ifdef LIBKITFOX
    pipeline->counter.rob.write++;
#endif
}

inst_t* ROB_t::get_front()
{
    vector<inst_t*>::iterator it = queue.begin();
    if(it != queue.end()) {
        inst_t *inst = *it;
        // Only Mop_head inst carries Mop_length information,
        // and can stall ROB until other uops are completed.
        if((inst->Mop_length == 0)&&!inst->inflight) { return inst; }
    }
    return NULL;
}

inst_t* ROB_t::get_head()
{
    if(queue.size()) { return *queue.begin(); }
    return NULL;
}

void ROB_t::update(inst_t *inst)
{
    inst->inflight = false; // Mark this inst completed.
    inst->Mop_head->Mop_length--; // Decrease the number of inflight insts in the same Mop

#ifdef LIBKITFOX
    // This access should update only physical register (dest operand) part of ROB, so it's an overestimation of power.
    pipeline->counter.rob.write++;
#endif
}

void ROB_t::pop_front() // It must be called after get_front().
{
    inst_t *inst = *queue.begin();

#ifdef SPX_DEBUG
    fprintf(stdout,"SPX_DEBUG (core %d) | %lu: ROB.pop_front (%d/%d) uop %lu (Mop %lu)\n",pipeline->core->core_id,pipeline->core->clock_cycle,queue.size()-1,size,inst->uop_sequence,inst->Mop_sequence);
#endif

#ifdef LIBKITFOX
    pipeline->counter.rob.read++;
    if(inst->dest_reg||inst->dest_flag||inst->dest_fpreg)
        pipeline->counter.latch_rob2reg.switching++;
    if(inst->opcode == QSIM_INST_CALL)
        pipeline->counter.ras.write++;
    if(inst->opcode == QSIM_INST_BR) {
        pipeline->counter.l1_local_predictor.write++;
        pipeline->counter.l2_local_predictor.write++;
        pipeline->counter.global_predictor.write++;
        pipeline->counter.predictor_chooser.write++;
    }
#endif

    queue.erase(queue.begin());
    //occupancy--;
}

bool ROB_t::is_available()
{
#ifdef SPX_DEBUG
    if(!(queue.size() < size)) {
        inst_t *head_inst = *queue.begin();
        inst_t *tail_inst = *queue.rbegin();
        fprintf(stdout,"SPX_DEBUG (core %d) | %lu: ROB full | head uop %lu (Mop %lu) | tail uop %lu (Mop %lu)\n",pipeline->core->core_id,pipeline->core->clock_cycle,head_inst->uop_sequence,head_inst->Mop_sequence,tail_inst->uop_sequence,tail_inst->Mop_sequence);
    }
#endif

    //return ((queue.size() < size)&&((int)queue.size() < size));
    return (queue.size() < (unsigned)size);
}





RS_t::RS_t(pipeline_t *pl, int RS_size, int exec_port, std::vector<int> *FU_port_binding) :
    size(RS_size), occupancy(0), port(exec_port), pipeline(pl)
{
    ready = new vector<inst_t*>[port];
    for(int i = 0; i < port; i++)
        ready[i].reserve(size);

    port_loads = new int[port];
    memset(port_loads,0,sizeof(int)*port);
    memset(FU_port,0,SPX_NUM_FU_TYPES*sizeof(uint16_t));

    // Copy FU port binding.
    for(int fu = 0; fu < SPX_NUM_FU_TYPES; fu++) {
        for(vector<int>::iterator it = FU_port_binding[fu].begin();
            it != FU_port_binding[fu].end(); it++) {
                FU_port[fu] |= (0x01<<(*it));
        }
    }
}
RS_t::~RS_t()
{
    for(int i = 0; i < port; i++)
        ready[i].clear();
    delete [] ready;
    delete [] port_loads;
}

void RS_t::push_back(inst_t *inst)
{
    // Inst has no dependency. Schedule in ready queue to execute.
    if(inst->src_dep.size() == 0) {
#ifdef SPX_DEBUG
        fprintf(stdout,"SPX_DEBUG (core %d) | %lu: RS.ready[%d].push_back (%d/%d) uop %lu (Mop %lu)\n",pipeline->core->core_id,pipeline->core->clock_cycle,inst->port,occupancy+1,size,inst->uop_sequence,inst->Mop_sequence);
#endif

        ready[inst->port].push_back(inst);
    }
    else { // Dependency exists. Wait in the pool.
#ifdef SPX_DEBUG
        fprintf(stdout,"SPX_DEBUG (core %d) | %lu: RS.pool.push_back (%d/%d) uop %lu (Mop %lu)\n",pipeline->core->core_id,pipeline->core->clock_cycle,occupancy+1,size,inst->uop_sequence,inst->Mop_sequence);
#endif
    }

    occupancy++;

#ifdef LIBKITFOX
    // outorder
    //pipeline->counter.rs.search++; // Assume finding an empty entry is tedious
    pipeline->counter.rs.write++;

    // inorder
    pipeline->counter.inst_queue.write++;
#endif

/* DEBUG DEBUG !!
    fprintf(stdout,"SPX_QSIM_DEBUG (core %d) | %lu: instQ.push_back (%d/%d) uop %lu Mop %lu  | ",pipeline->core->core_id,pipeline->core->clock_cycle,occupancy,size,inst->uop_sequence,inst->Mop_sequence);
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
            fprintf(stdout,"SPX_QSIM_DEBUG (core %d) | %lu:       src reg %d\n",pipeline->core->core_id,pipeline->core->clock_cycle,i);
    }
    uint8_t src_flag_mask = inst->src_flag;
    for(int i = 0; src_flag_mask >0; src_flag_mask = src_flag_mask>>1, i++) {
        if(src_flag_mask&0x01)
            fprintf(stdout,"SPX_QSIM_DEBUG (core %d) | %lu:       src flag %d\n",pipeline->core->core_id,pipeline->core->clock_cycle,i);
    }
    uint8_t src_fpreg_mask = inst->src_fpreg;
    for(int i = 0; src_fpreg_mask >0; src_fpreg_mask = src_fpreg_mask>>1, i++) {
        if(src_fpreg_mask&0x01)
            fprintf(stdout,"SPX_QSIM_DEBUG (core %d) | %lu:       src fpreg %d\n",pipeline->core->core_id,pipeline->core->clock_cycle,i);
    }
    for(map<uint64_t,inst_t*>::iterator it = inst->src_dep.begin(); it != inst->src_dep.end(); it++) {
        if(it->second->memcode == SPX_MEM_LD)
            fprintf(stdout,"SPX_QSIM_DEBUG (core %d) | %lu:       mem dep 0x%lx\n",pipeline->core->core_id,pipeline->core->clock_cycle,it->second->data.paddr);
    }
    if(inst->memcode != SPX_MEM_NONE) {
        fprintf(stdout,"SPX_QSIM_DEBUG (core %d) | %lu:       mem %s 0x%lx\n",pipeline->core->core_id,pipeline->core->clock_cycle,(inst->memcode==SPX_MEM_ST)?"STORE":"LOAD",inst->data.paddr);
    }
    uint64_t dest_reg_mask = inst->dest_reg;
    for(int i = 0; dest_reg_mask > 0; dest_reg_mask = dest_reg_mask>>1, i++) {
        if(dest_reg_mask&0x01)
            fprintf(stdout,"SPX_QSIM_DEBUG (core %d) | %lu:       dest reg %d\n",pipeline->core->core_id,pipeline->core->clock_cycle,i);
    }
    uint8_t dest_flag_mask = inst->dest_flag;
    for(int i = 0; dest_flag_mask > 0; dest_flag_mask = dest_flag_mask>>1, i++) {
        if(dest_flag_mask&0x01)
            fprintf(stdout,"SPX_QSIM_DEBUG (core %d) | %lu:       dest flag %d\n",pipeline->core->core_id,pipeline->core->clock_cycle,i);
    }
    uint8_t dest_fpreg_mask = inst->dest_fpreg;
    for(int i = 0; dest_fpreg_mask > 0; dest_fpreg_mask = dest_fpreg_mask>>1, i++) {
        if(dest_fpreg_mask&0x01)
            fprintf(stdout,"SPX_QSIM_DEBUG (core %d) | %lu:       dest fpreg %d\n",pipeline->core->core_id,pipeline->core->clock_cycle,i);
    }
*/
}

inst_t* RS_t::get_front(int port)
{
    vector<inst_t*>::iterator it = ready[port].begin();
    if(it != ready[port].end())
        return *it;
    else
        return NULL;
}

void RS_t::pop_front(int port) // It must be called after get_front().
{
#ifdef SPX_DEBUG
    inst_t *inst = *(ready[port].begin());
    fprintf(stdout,"SPX_DEBUG (core %d) | %lu: RS.pop_front (%d/%d) uop %lu (Mop %lu)\n",pipeline->core->core_id,pipeline->core->clock_cycle,occupancy-1,size,inst->uop_sequence,inst->Mop_sequence);
#endif

    ready[port].erase(ready[port].begin());
    occupancy--;
    port_loads[port]--;

#ifdef LIBKITFOX
    // outorder
    pipeline->counter.rs.search++; // Scan ready insts
    pipeline->counter.issue_select.switching++; // Select ready insts
    pipeline->counter.rs.read++; // Read the selected, ready inst
    pipeline->counter.latch_rs2ex.switching++; // Latch from RS to EX

    // inorder
    pipeline->counter.inst_queue.read++;
    pipeline->counter.latch_iq2ex.switching++;
#endif
}

void RS_t::update(inst_t *inst)
{
    for(map<uint64_t,inst_t*>::iterator it = inst->dest_dep.begin(); it != inst->dest_dep.end(); it++) {
        inst_t *dep_inst = it->second; // Dependent inst
        dep_inst->src_dep.erase(dep_inst->src_dep.find(inst->uop_sequence)); // Remove dependency.

        if((dep_inst->src_dep.size() == 0)&&(dep_inst->port > -1)) { // Dependent inst is now ready to execute.
#ifdef SPX_DEBUG
            fprintf(stdout,"SPX_DEBUG (core %d) | %lu: RS.ready uop %lu (Mop %lu) free from uop %lu (Mop %lu)\n",pipeline->core->core_id,pipeline->core->clock_cycle,dep_inst->uop_sequence,dep_inst->Mop_sequence,inst->uop_sequence,inst->Mop_sequence);
#endif

            // Move dep_inst to ready queue.
            ready[dep_inst->port].push_back(dep_inst);
        }

#ifdef LIBKITFOX
        // outorder
        // This access should update only the source operands of dependent insts, so it's an overestimation of power.
        pipeline->counter.rs.search++;
        pipeline->counter.rs.write++;

        // inorder
        pipeline->counter.inst_queue.write++;
#endif
    }

    inst->dest_dep.clear();
}

void RS_t::port_binding(inst_t *inst)
{
    int min_load = INT_MAX;
    uint16_t port_mask = FU_port[inst->excode];
    for(int port = 0; port_mask > 0; port_mask = port_mask>>1, port++) {
        if((port_mask&0x01)&&(port_loads[port] < min_load)) {
            min_load = port_loads[port];
            inst->port = port;
        }
    }

#ifdef SPX_DEBUG
    fprintf(stdout,"SPX_DEBUG (core %d) | %lu: RS.port_binding %d (load %d) to uop %lu (Mop %lu) | ",pipeline->core->core_id,pipeline->core->clock_cycle,inst->port,port_loads[inst->port],inst->uop_sequence,inst->Mop_sequence);
    switch(inst->excode) {
        case SPX_FU_INT: fprintf(stdout,"FU_INT"); break;
        case SPX_FU_MUL: fprintf(stdout,"FU_MUL"); break;
        case SPX_FU_FP: fprintf(stdout,"FU_FP"); break;
        case SPX_FU_MOV: fprintf(stdout,"FU_MOV"); break;
        case SPX_FU_BR: fprintf(stdout,"FU_BR"); break;
        case SPX_FU_LD: fprintf(stdout,"FU_LD"); break;
        case SPX_FU_ST: fprintf(stdout,"FU_ST"); break;
        default: break;
    }
    fprintf(stdout,"\n");
#endif
    port_loads[inst->port]++;
}

bool RS_t::is_available()
{
    return (occupancy < size);
}





RF_t::RF_t(pipeline_t *pl) :
    fpregs_stack_ptr(0),
    pipeline(pl)
{
    regs.reserve(QSIM_N_REGS);

    for(int i = 0; i < QSIM_N_REGS; i++) { regs[i] = NULL; }
    for(int i = 0; i < SPX_N_FLAGS; i++) { flags[i] = NULL; }
    for(int i = 0; i < SPX_N_FPREGS; i++) { fpregs[i] = NULL; }
}

RF_t::~RF_t()
{
}

void RF_t::resolve_dependency(inst_t *inst)
{
    // reg src dependency
    uint64_t src_reg_mask = inst->src_reg;
    for(int i = 0; src_reg_mask > 0; src_reg_mask = src_reg_mask>>1, i++) {
        if(src_reg_mask&0x01) { // inst has source operand(s).
#ifdef LIBKITFOX
            pipeline->counter.dependency_check.switching++;
            pipeline->counter.rat.search++;
#endif
            if(regs[i] != NULL)  { // Dependency exists.
#ifdef LIBKITFOX
                pipeline->counter.rat.read++;
#endif
                if(regs[i]->inflight) {
#ifdef SPX_DEBUG
                    fprintf(stdout,"SPX_DEBUG (core %d) | %lu: RF.check_dependency reg %d | uop %lu (Mop %lu) depends on uop %lu (Mop %lu)\n",pipeline->core->core_id,pipeline->core->clock_cycle,i,inst->uop_sequence,inst->Mop_sequence,regs[i]->uop_sequence,regs[i]->Mop_sequence);
#endif
                    // Source inst knows which subsequent insts are dependent on its dest regs.
                    regs[i]->dest_dep.insert(pair<uint64_t,inst_t*>(inst->uop_sequence,inst));
                    // Depenedent inst know which precendent insts it is dependent on.
                    inst->src_dep.insert(pair<uint64_t,inst_t*>(regs[i]->uop_sequence,regs[i]));
                }
#ifdef LIBKITFOX
                else { // Source inst has completed execution but not committed yet.
                    // The source operand is available in the physical register (ROB).
                    pipeline->counter.rob.read++;
                    pipeline->counter.latch_rob2rs.switching++;
                }
#endif
            }
#ifdef LIBKITFOX
            else {
                pipeline->counter.reg_int.read++;
            }
#endif
        }
    }

    // reg dest dependency
    uint64_t dest_reg_mask = inst->dest_reg;
    for(int i = 0; dest_reg_mask > 0; dest_reg_mask = dest_reg_mask>>1, i++) {
        if(dest_reg_mask&0x01) {
#ifdef LIBKITFOX
            pipeline->counter.rat.write++;
            pipeline->counter.freelist.read++;
#endif
            regs[i] = inst; // Inst writes reg.
        }
    }

    if (pipeline->config.arch_type == SPX_A64)
        return ;

    // flag src dependency
    uint8_t src_flag_mask = inst->src_flag;
    for(int i = 0; src_flag_mask > 0; src_flag_mask = src_flag_mask>>1, i++) {
        if((src_flag_mask&0x01)&&flags[i]&&flags[i]->inflight) { // dependecy exists
#ifdef SPX_DEBUG
            fprintf(stdout,"SPX_DEBUG (core %d) | %lu: RF.check_dependency flag %d | uop %lu (Mop %lu) depends on uop %lu (Mop %lu)\n",pipeline->core->core_id,pipeline->core->clock_cycle,i,inst->uop_sequence,inst->Mop_sequence,flags[i]->uop_sequence,flags[i]->Mop_sequence);
#endif

            // Source inst knows which subsequent insts are dependent on its dest regs.
            flags[i]->dest_dep.insert(pair<uint64_t,inst_t*>(inst->uop_sequence,inst));
            // Depenedent inst know which precendent insts it is dependent on.
            inst->src_dep.insert(pair<uint64_t,inst_t*>(flags[i]->uop_sequence,flags[i]));
        }
    }

    // flag dest dependency
    uint8_t dest_flag_mask = inst->dest_flag;
    for(int i = 0; dest_flag_mask > 0; dest_flag_mask = dest_flag_mask>>1, i++) {
        if(dest_flag_mask&0x01)
            flags[i] = inst; // Inst writes flag.
    }

    // fpregs dependency for X87 FPU
    if(inst->opcode >= QSIM_INST_FPBASIC) {
        switch(inst->memcode) {
            case SPX_MEM_LD: { // FLD
#ifdef LIBKITFOX
                pipeline->counter.rat.write++;
                pipeline->counter.freelist.read++;
#endif
                fpregs_stack_ptr = (++fpregs_stack_ptr)%(int)SPX_N_FPREGS;
                fpregs[fpregs_stack_ptr] = inst;
                inst->dest_fpreg |= (0x01<<fpregs_stack_ptr); // ST0
                break;
            }
            case SPX_MEM_ST: { // FST
#ifdef LIBKITFOX
                pipeline->counter.dependency_check.switching++;
                pipeline->counter.rat.search++;
#endif
                if(fpregs[fpregs_stack_ptr]) { // Dependency exists.
#ifdef LIBKITFOX
                    pipeline->counter.rat.read++;
#endif
                    if(fpregs[fpregs_stack_ptr]->inflight) {
                        fpregs[fpregs_stack_ptr]->dest_dep.insert(pair<uint64_t,inst_t*>(inst->uop_sequence,inst));
                        inst->src_dep.insert(pair<uint64_t,inst_t*>(fpregs[fpregs_stack_ptr]->uop_sequence,fpregs[fpregs_stack_ptr]));
                    }
#ifdef LIBKITFOX
                    else { // Source inst has completed execution but not committed yet.
                        // Source operand is available in the physical register (ROB).
                        pipeline->counter.rob.read++;
                        pipeline->counter.latch_rob2rs.switching++;
                    }
#endif
                    fpregs[fpregs_stack_ptr] = NULL;
                    inst->src_fpreg |= (0x01<<fpregs_stack_ptr); // ST0
                    if(--fpregs_stack_ptr < 0) fpregs_stack_ptr = (int)SPX_N_FPREGS-1;
                }
#ifdef LIBKITFOX
                else
                    pipeline->counter.reg_fp.read++;
#endif
                break;
            }
            default: { // Regular FP inst
#ifdef LIBKITFOX
                pipeline->counter.dependency_check.switching++;
                pipeline->counter.rat.search++;
#endif
                if(fpregs[fpregs_stack_ptr]) { // Dependency exists.
#ifdef LIBKITFOX
                    pipeline->counter.rat.read++;
#endif
                    if(fpregs[fpregs_stack_ptr]->inflight) {
                        fpregs[fpregs_stack_ptr]->dest_dep.insert(pair<uint64_t,inst_t*>(inst->uop_sequence,inst));
                        inst->src_dep.insert(pair<uint64_t,inst_t*>(fpregs[fpregs_stack_ptr]->uop_sequence,fpregs[fpregs_stack_ptr]));
                    }
#ifdef LIBKITFOX
                    else { // Source inst has completed execution but not committed yet.
                        // Source operand is available in the physical register (ROB).
                        pipeline->counter.rob.read++;
                        pipeline->counter.latch_rob2rs.switching++;
                    }
#endif
                    inst->src_fpreg |= (0x01<<fpregs_stack_ptr); // ST0

                    // ST1 src dependency
                    int st1_ptr = fpregs_stack_ptr-1;
                    if(st1_ptr < 0) st1_ptr = (int)SPX_N_FPREGS-1;
#ifdef LIBKITFOX
                    pipeline->counter.dependency_check.switching++;
                    pipeline->counter.rat.search++;
#endif
                    if(fpregs[st1_ptr]) { // Dependency exists.
#ifdef LIBKITFOX
                        pipeline->counter.rat.read++;
#endif
                        if(fpregs[st1_ptr]->inflight) {
                            fpregs[st1_ptr]->dest_dep.insert(pair<uint64_t,inst_t*>(inst->uop_sequence,inst));
                            inst->src_dep.insert(pair<uint64_t,inst_t*>(fpregs[st1_ptr]->uop_sequence,fpregs[st1_ptr]));
                        }
#ifdef LIBKITFOX
                        else { // Source inst has completed execution but not committed yet.
                            // Source operand is available in the physical register (ROB).
                            pipeline->counter.rob.read++;
                            pipeline->counter.latch_rob2rs.switching++;
                        }
#endif
                        inst->src_fpreg |= (0x01<<st1_ptr); // ST1
                    }
                }

                fpregs[fpregs_stack_ptr] = inst; // This inst now the latest producer of ST0.
                inst->dest_fpreg |= (0x01<<fpregs_stack_ptr); // ST0
                break;
            }
        }
    }

#ifdef LIBKITFOX
    pipeline->counter.latch_rr2rs.switching++;
#endif
}

void RF_t::writeback(inst_t *inst)
{
    // Inst writes reg.
    uint64_t dest_reg_mask = inst->dest_reg;
    for(int i = 0; dest_reg_mask > 0; dest_reg_mask = dest_reg_mask>>1, i++) {
        if(dest_reg_mask&0x01) {
#ifdef LIBKITFOX
            pipeline->counter.rat.read++;
            pipeline->counter.freelist.write++;
            pipeline->counter.reg_int.write++;
#endif
            // This reg is now free, no dependency exists
            if(regs[i] == inst) { regs[i] = NULL; }
        }
    }

    if (pipeline->config.arch_type == SPX_A64)
        return ;

    // Inst writes flag.
    uint8_t dest_flag_mask = inst->dest_flag;
    for(int i = 0; dest_flag_mask > 0; dest_flag_mask = dest_flag_mask>>1, i++) {
        // this flag is now free, no dependency exists
        if((dest_flag_mask&0x01)&&(flags[i] == inst))
            flags[i] = NULL;
    }

    // Inst write fpreg.
    uint8_t dest_fpreg_mask = inst->dest_fpreg;
    for(int i = 0; dest_fpreg_mask > 0; dest_fpreg_mask = dest_fpreg_mask>>1, i++) {
        if(dest_fpreg_mask&0x01) {
#ifdef LIBKITFOX
            pipeline->counter.rat.read++;
            pipeline->counter.freelist.write++;
            pipeline->counter.reg_fp.write++;
#endif
            // This fpreg is now free, no dependency exists
            if(fpregs[i] == inst) { fpregs[i] = NULL; }
        }
    }
}




FU_t::FU_t(pipeline_t *pl, int FU_delay, int FU_issue_rate) :
    delay(FU_delay), issue_rate(FU_issue_rate), pipeline(pl)
{
    queue.reserve(FU_delay);
}

FU_t::~FU_t()
{
    queue.clear();
}

void FU_t::push_back(inst_t *inst)
{
    inst->completed_cycle = pipeline->core->clock_cycle+delay;
    queue.push_back(inst);
}

inst_t* FU_t::get_front()
{
    vector<inst_t*>::iterator it = queue.begin();
    if(it != queue.end()) {
        inst_t *inst = (*it);
        if(inst->completed_cycle <= pipeline->core->clock_cycle) { return inst; }
        return NULL; // Inst still in execution
    }
    return NULL;// FU is empty
}

void FU_t::pop_front()
{
    if(queue.begin() != queue.end()) { queue.erase(queue.begin()); }
}

bool FU_t::is_available()
{
    vector<inst_t*>::reverse_iterator it = queue.rbegin();
    if(it != queue.rend()) {
        inst_t *inst = (*it);
        return (((inst->completed_cycle-delay) <= (pipeline->core->clock_cycle - issue_rate))
               &&(queue.size() < (unsigned int)delay));
    }
    return true;
}

void FU_t::stall()
{
    // Insts with bubble(s) ahead in the queue will proceed without stall.
    uint64_t stall_stamp = (*queue.begin())->completed_cycle;
    for(vector<inst_t*>::iterator it = queue.begin(); it != queue.end(); it++) {
        inst_t *inst = *it;
        if(it == queue.begin()) { // Head inst has always reached the end of queue.
            stall_stamp = ++inst->completed_cycle;
        }
        else if(stall_stamp == inst->completed_cycle) {
            stall_stamp = ++inst->completed_cycle;
        }
        else {
            stall_stamp = inst->completed_cycle;
        }
    }
}





EX_t::EX_t(pipeline_t *pl, int FU_port, vector<int> *FU_port_binding, int *FU_delay, int *FU_issue_rate) :
    num_FUs(0), pipeline(pl)
{
    for(int fu = 0; fu < SPX_NUM_FU_TYPES; fu++) {
        for(vector<int>::iterator it = FU_port_binding[fu].begin(); it != FU_port_binding[fu].end(); it++) {
            if(FU_port == (*it)) { // An FU binds to this port
                FUs[fu] = new FU_t(pipeline,FU_delay[fu],FU_issue_rate[fu]);
                FU_index[num_FUs++] = fu;
            }
        }
    }
}

EX_t::~EX_t()
{
    for(int fu = 0; fu < SPX_NUM_FU_TYPES; fu++)
        delete FUs[fu];
}

void EX_t::push_back(inst_t *inst)
{
#ifdef SPX_DEBUG
    fprintf(stdout,"SPX_DEBUG (core %d) | %lu: EX[%d].push_back uop %lu (Mop %lu)\n",pipeline->core->core_id,pipeline->core->clock_cycle,inst->port,inst->uop_sequence,inst->Mop_sequence);
#endif
    FUs[inst->excode]->push_back(inst);
}

inst_t* EX_t::get_front()
{
    inst_t *oldest_inst = NULL;

    for(int fu = 0; fu < num_FUs; fu++) {
        inst_t *inst = FUs[FU_index[fu]]->get_front();

        if(inst) {
            if(!oldest_inst)
                oldest_inst = inst;
            else if(inst->uop_sequence < oldest_inst->uop_sequence) {
                FUs[oldest_inst->excode]->stall();
                oldest_inst = inst;
            }
        }
    }
    return oldest_inst;
}

void EX_t::pop_front(int excode)
{
#ifdef LIBKITFOX
    switch(excode) {
        case SPX_FU_FP: {
            pipeline->counter.fpu.switching += pipeline->config.FU_delay[excode];
            pipeline->counter.fp_bypass.switching++;
            pipeline->counter.latch_ex_fp2rob.switching += pipeline->config.FU_delay[excode];
            break;
        }
        case SPX_FU_MUL: {
            pipeline->counter.mul.switching += pipeline->config.FU_delay[excode];
            pipeline->counter.int_bypass.switching++;
            pipeline->counter.latch_ex_int2rob.switching += pipeline->config.FU_delay[excode];
            break;
        }
        default: {
            pipeline->counter.alu.switching += pipeline->config.FU_delay[excode];
            pipeline->counter.int_bypass.switching++;
            pipeline->counter.latch_ex_int2rob.switching += pipeline->config.FU_delay[excode];
            break;
        }

        // inorder
        pipeline->counter.latch_ex2reg.switching++;
    }
#endif

#ifdef SPX_DEBUG
    inst_t *inst = FUs[excode]->get_front();
    fprintf(stdout,"SPX_DEBUG (core %d) | %lu: EX[%d].pop_front %lu (Mop %lu)\n",pipeline->core->core_id,pipeline->core->clock_cycle,inst->port,inst->uop_sequence,inst->Mop_sequence);
#endif

    FUs[excode]->pop_front();
}

bool EX_t::is_available(int excode)
{
    return FUs[excode]->is_available();
}





LDQ_t::LDQ_t(pipeline_t *pl, int LDQ_size) :
    size(LDQ_size), occupancy(0), pipeline(pl)
{
    outgoing.reserve(size);
}

LDQ_t::~LDQ_t()
{
    outgoing.clear();
}

void LDQ_t::push_back(inst_t *inst)
{
#ifdef SPX_DEBUG
    fprintf(stdout,"SPX_DEBUG (core %d) | %lu: LDQ.push_back (%d/%d) uop %lu (Mop %lu)\n",pipeline->core->core_id,pipeline->core->clock_cycle,occupancy+1,size,inst->uop_sequence,inst->Mop_sequence);
#endif

    occupancy++;

#ifdef LIBKITFOX
    // outorder
    //pipeline->counter.ldq.search++; // Assume finding an empty entry is tedious
    pipeline->counter.ldq.write++;

    // inorder
    //pipeline->counter.lsq.search++; // Assume finding an empty entry is tedious
    pipeline->counter.lsq.write++;
#endif
}

void LDQ_t::pop(inst_t *inst)
{
#ifdef SPX_DEBUG
    fprintf(stdout,"SPX_DEBUG (core %d) | %lu: LDQ.pop (%d/%d) uop %lu (Mop %lu)\n",pipeline->core->core_id,pipeline->core->clock_cycle,occupancy-1,size,inst->uop_sequence,inst->Mop_sequence);
#endif

    if(pipeline->config.memory_deadlock_avoidance)
        outstanding.erase(inst->uop_sequence);

    occupancy--;

#ifdef LIBKITFOX
    // outorder
    pipeline->counter.ldq.search++;
    pipeline->counter.ldq.read++;
    pipeline->counter.latch_ldq2rs.switching++;

    // inorder
    pipeline->counter.lsq.search++;
    pipeline->counter.lsq.read++;
    pipeline->counter.latch_lsq2reg.switching++;
#endif
}

void LDQ_t::schedule(inst_t *inst)
{
    if(inst->mem_disamb_status == SPX_MEM_DISAMB_NONE) {
        // No memory disambiguation, schedule memory request.
        outgoing.push_back(inst);
    }
    else if(inst->mem_disamb_status == SPX_MEM_DISAMB_CLEAR) {
        // Memory disambiguation exists but is already cleared.
        // Store was executed already, so data should have been forwarded.
        // Treat this load as if a memory request is immediately returned.
#ifdef SPX_DEBUG
        fprintf(stdout,"SPX_DEBUG (core %d) | %lu: uop %lu (Mop %lu) bypasses handle_cache() by store forwarding\n",pipeline->core->core_id,pipeline->core->clock_cycle,inst->uop_sequence,inst->Mop_sequence);
#endif

        cache_request_t *cache_request = new cache_request_t(inst,pipeline->core->core_id,pipeline->core->node_id,inst->data.paddr,SPX_MEM_LD);
        pipeline->handle_cache_response(0,cache_request);

#ifdef LIBKITFOX
        // outorder
        // Data forwards from STQ to LDQ
        pipeline->counter.stq.read++;
        pipeline->counter.ldq.write++;
        pipeline->counter.latch_stq2ldq.switching++;

        // inorder
        pipeline->counter.lsq.read++;
        pipeline->counter.lsq.write++;
#endif
    }
    else {
        // This load has memory order violation.
        // Wait for store until its address is calculated so that data can be forwarded.
#ifdef SPX_DEBUG
        fprintf(stdout,"SPX_DEBUG (core %d) | %lu: uop %lu (Mop %lu) is blocked by memory disambiguation\n",pipeline->core->core_id,pipeline->core->clock_cycle,inst->uop_sequence,inst->Mop_sequence);
#endif

        inst->mem_disamb_status = SPX_MEM_DISAMB_WAIT;
    }

#ifdef LIBKITFOX
    // outorder
    pipeline->counter.ldq.search++;

    // inorder
    pipeline->counter.lsq.search++;
#endif
}

void LDQ_t::handle_cache()
{
    vector<inst_t*>::iterator it = outgoing.begin();
    if(it != outgoing.end()) {
        inst_t *inst = *it;

#ifdef SPX_DEBUG
        fprintf(stdout,"SPX_DEBUG (core %d) | %lu: LDQ.handle_cache uop %lu (Mop %lu)\n",pipeline->core->core_id,pipeline->core->clock_cycle,inst->uop_sequence,inst->Mop_sequence);
        //fprintf(stderr,"@%d SPX_DEBUG (core %d) | LDQ.handle_cache LD 0x%lx\n",manifold::kernel::Manifold::NowTicks(), pipeline->core->core_id,inst->data.paddr);
#endif
        // Send cache request.
        cache_request_t *cache_request = new cache_request_t(inst,pipeline->core->core_id,pipeline->core->node_id,inst->data.paddr,SPX_MEM_LD);
        inst->memory_request_time_stamp = pipeline->core->clock_cycle;

        if(pipeline->config.memory_deadlock_avoidance)
            outstanding.insert(pair<uint64_t,inst_t*>(inst->uop_sequence,inst));

        pipeline->core->Send(pipeline->core->OUT_TO_DATA_CACHE,cache_request);

#ifdef LIBKITFOX
        if(pipeline->data_tlb) pipeline->data_tlb->access(inst->data.vaddr,false);
#endif

        outgoing.erase(it);

#ifdef LIBKITFOX
        // outorder
        pipeline->counter.ldq.read++;
        pipeline->counter.latch_ldq2dcache.switching++;

        // inorder
        pipeline->counter.lsq.read++;
        pipeline->counter.latch_lsq2dcache.switching++;
#endif
    }
}

bool LDQ_t::is_available()
{
    //return ((occupancy < size)&&(outgoing.size()+outstanding.size() < (unsigned int)size));
    return ((occupancy < size)&&(outstanding.size() < (unsigned int)size));
}

inst_t* LDQ_t::handle_deadlock()
{
    if(outstanding.size()) {
        inst_t *deadlock_inst = outstanding.begin()->second;
        // An outstanding memory request has not returned more than threshold cycles.
        // Remove this inst from the LDQ.
        if(pipeline->core->clock_cycle - deadlock_inst->memory_request_time_stamp > pipeline->config.memory_deadlock_threshold) {
            fprintf(stdout,"SPX WARNING: core%d (node %d) load inst %lu (Mop %lu) deadlock detected (threshold =%lu) paddr = %lx\n",pipeline->core->core_id,pipeline->core->node_id,deadlock_inst->uop_sequence,deadlock_inst->Mop_sequence,pipeline->config.memory_deadlock_threshold,deadlock_inst->data.paddr);
            deadlock_inst->squashed = true;
            pop(deadlock_inst);
            abort();
            return deadlock_inst;
        }
    }
    return NULL;
}





STQ_t::STQ_t(pipeline_t *pl, int STQ_size) :
    size(STQ_size), occupancy(0), pipeline(pl)
{
    outgoing.reserve(size);
}

STQ_t::~STQ_t()
{
    outgoing.clear();
}

void STQ_t::push_back(inst_t *inst)
{
#ifdef SPX_DEBUG
    fprintf(stdout,"SPX_DEBUG (core %d) | %lu: STQ.push_back (%d/%d) uop %lu (Mop %lu)\n",pipeline->core->core_id,pipeline->core->clock_cycle,occupancy+1,size,inst->uop_sequence,inst->Mop_sequence);
#endif

    occupancy++;
    // Memory disambiguation map for store forwarding.
    // Since store inst is not yet executed, it is like a perfect prediction for memory disambiguation.
    map<uint64_t,inst_t*>::iterator it = mem_disamb.find(inst->data.paddr);
    if(it != mem_disamb.end())
        it->second = inst; // Replace the latest producer of this address.
    else {
        mem_disamb.insert(pair<uint64_t,inst_t*>(inst->data.paddr,inst));
    }

#ifdef LIBKITFOX
    // outorder
    //pipeline->counter.stq.search++; // Assume finding an empty entry is tedious.
    pipeline->counter.stq.write++;

    // inorder
    pipeline->counter.lsq.write++;
#endif
}

void STQ_t::pop(inst_t *inst)
{
#ifdef SPX_DEBUG
    fprintf(stdout,"SPX_DEBUG (core %d) | %lu: STQ.pop (%d/%d) uop %lu (Mop %lu)\n",pipeline->core->core_id,pipeline->core->clock_cycle,occupancy-1,size,inst->uop_sequence,inst->Mop_sequence);
#endif

    if(pipeline->config.memory_deadlock_avoidance)
        outstanding.erase(inst->uop_sequence);

    occupancy--;

#ifdef LIBKITFOX
    // outorder
    pipeline->counter.stq.search++;
    pipeline->counter.stq.read++;

    // inorder
    pipeline->counter.lsq.search++;
    pipeline->counter.lsq.read++;
#endif
}

void STQ_t::schedule(inst_t *inst)
{
    outgoing.push_back(inst);

#ifdef LIBKITFOX
    // outorder
    pipeline->counter.stq.search++;

    // inorder
    pipeline->counter.lsq.search++;
#endif
}

void STQ_t::handle_cache()
{
    vector<inst_t*>::iterator outgoing_it = outgoing.begin();
    if(outgoing_it != outgoing.end()) {
        inst_t *inst = *outgoing_it;

#ifdef SPX_DEBUG
        fprintf(stdout,"SPX_DEBUG (core %d) | %lu: STQ.handle_cache uop %lu (Mop %lu)\n",pipeline->core->core_id,pipeline->core->clock_cycle,inst->uop_sequence,inst->Mop_sequence);
        //fprintf(stderr,"@%d SPX_DEBUG (core %d) | STQ.handle_cache ST 0x%lx\n",manifold::kernel::Manifold::NowTicks(), pipeline->core->core_id,inst->data.paddr);
#endif

        // Since store is written back when the ROB commits, clear memory disambiguation map.
        map<uint64_t,inst_t*>::iterator mem_disamb_it = mem_disamb.find(inst->data.paddr);
        if((mem_disamb_it != mem_disamb.end())&&(mem_disamb_it->second == inst))
            mem_disamb.erase(mem_disamb_it);

        // Send cache request.
        cache_request_t *cache_request = new cache_request_t(inst,pipeline->core->core_id,pipeline->core->node_id,inst->data.paddr,SPX_MEM_ST);
        inst->memory_request_time_stamp = pipeline->core->clock_cycle;

        if(pipeline->config.memory_deadlock_avoidance)
            outstanding.insert(pair<uint64_t,inst_t*>(inst->uop_sequence,inst));

        pipeline->core->Send(pipeline->core->OUT_TO_DATA_CACHE,cache_request);

#ifdef LIBKITFOX
        if(pipeline->data_tlb) pipeline->data_tlb->access(inst->data.vaddr,false);
#endif

        outgoing.erase(outgoing_it);

#ifdef LIBKITFOX
        // outorder
        pipeline->counter.stq.read++;
        pipeline->counter.latch_stq2dcache.switching++;

        // inorder
        pipeline->counter.lsq.read++;
        pipeline->counter.latch_lsq2dcache.switching++;
#endif
    }
}

bool STQ_t::is_available()
{
    //if((occupancy < size)&&(outgoing.size()+outstanding.size() < (unsigned int)size)) { return true; }
    if((occupancy < size)&&(outstanding.size() < (unsigned int)size)) { return true; }
    if(pipeline->config.memory_deadlock_avoidance) { return (bool)handle_deadlock(); }
    return false;
}

inst_t* STQ_t::handle_deadlock()
{
    if(outstanding.size()) {
        inst_t *deadlock_inst = outstanding.begin()->second;
        // An outstanding memory request's response has not returned more than threshold cycles.
        // Remove this inst from the STQ.
        if(pipeline->core->clock_cycle - deadlock_inst->memory_request_time_stamp > pipeline->config.memory_deadlock_threshold) {
            fprintf(stdout,"SPX WARNING: core%d (node %d) store inst %lu (Mop %lu) deadlock detected (threshold =%lu) paddr = %lx\n",pipeline->core->core_id,pipeline->core->node_id,deadlock_inst->uop_sequence,deadlock_inst->Mop_sequence,pipeline->config.memory_deadlock_threshold,deadlock_inst->data.paddr);
            deadlock_inst->squashed = true;
            pop(deadlock_inst);
            abort();
            return deadlock_inst;
        }
    }
    return NULL;
}

void STQ_t::store_forward(inst_t *inst)
{
    // If there is any loads after this store, forward data.
    for(vector<inst_t*>::iterator it = inst->mem_disamb.begin(); it != inst->mem_disamb.end(); it++) {
        inst_t *ld_inst = (*it);
        // There is a load waiting on this store to forward.
        if(ld_inst->mem_disamb_status == SPX_MEM_DISAMB_WAIT) {
#ifdef SPX_DEBUG
            fprintf(stdout,"\n\n\n\n\nSPX_DEBUG (core %d) | %lu: uop %lu (Mop %lu) does store forwarding to uop %lu (Mop %lu)\n",pipeline->core->core_id,pipeline->core->clock_cycle,inst->uop_sequence,inst->Mop_sequence,ld_inst->uop_sequence,ld_inst->Mop_sequence);
#endif

            // Handle pending load as if a memory request is immediately returned.
            cache_request_t *cache_request = new cache_request_t(ld_inst,pipeline->core->core_id,pipeline->core->node_id,ld_inst->data.paddr,SPX_MEM_LD);
            pipeline->handle_cache_response(0,cache_request);

#ifdef LIBKITFOX
            // outorder
            // Data forwards from STQ to LDQ
            pipeline->counter.stq.read++;
            pipeline->counter.ldq.write++;
            pipeline->counter.latch_stq2ldq.switching++;

            // inorder
            pipeline->counter.lsq.read++;
            pipeline->counter.lsq.write++;
#endif
        }
        else // This load is not yet executed; handled by LDQ_t::schedule.
            ld_inst->mem_disamb_status = SPX_MEM_DISAMB_CLEAR;
    }
}

void STQ_t::mem_disamb_check(inst_t *inst)
{
    // Check if memory disambiguation exists; perfect prediction for memory disambiguation.
    map<uint64_t,inst_t*>::iterator it = mem_disamb.find(inst->data.paddr);
    if(it != mem_disamb.end()) {
#ifdef SPX_DEBUG
        fprintf(stdout,"\n\n\n\n\nSPX_DEBUG (core %d) | %lu: uop %lu (Mop %lu) has memory disambiguation with uop %lu (Mop %lu)\n",pipeline->core->core_id,pipeline->core->clock_cycle,inst->uop_sequence,inst->Mop_sequence,it->second->uop_sequence,it->second->Mop_sequence);
#endif

        if(it->second->inflight) { // Store is not yet executed, mark load inst as memory disambiguation detected.
            it->second->mem_disamb.push_back(inst);
            inst->mem_disamb_status = SPX_MEM_DISAMB_DETECTED;
        }
        else // Store is executed, mark load inst as cleared.
            inst->mem_disamb_status = SPX_MEM_DISAMB_CLEAR;
    }

#ifdef LIBKITFOX
    // outorder
    // Search STQ entries and compare tags
    pipeline->counter.stq.search++;

    // inorder
    pipeline->counter.lsq.search++;
#endif
}

#endif // USE_QSIM
