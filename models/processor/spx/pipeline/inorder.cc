#ifdef USE_QSIM

#include "inorder.h"
#include "core.h"

using namespace std;
using namespace libconfig;
using namespace manifold;
using namespace manifold::spx;

inorder_t::inorder_t(spx_core_t *spx_core, libconfig::Config *parser)
{
    type = SPX_PIPELINE_INORDER;
    core = spx_core;

    // set config
    set_config(parser);
    assert(QSIM_N_REGS != 0);
  
    // initialize core components
    instQ = new instQ_t((pipeline_t*)this,config.instQ_size,config.cache_line_width);
    threadQ = new RS_t((pipeline_t*)this,1,config.exec_width,config.FU_port);
    STQ = new STQ_t((pipeline_t*)this,config.STQ_size);
    LDQ = new LDQ_t((pipeline_t*)this,config.LDQ_size);
    RF = new RF_t((pipeline_t*)this);
    for(int i = 0; i < config.exec_width; i++)
        EX.push_back(new EX_t((pipeline_t*)this,i,config.FU_port,config.FU_delay,config.FU_issue_rate));
}

inorder_t::~inorder_t()
{
    delete instQ;
    delete threadQ;
    delete STQ;
    delete LDQ;
    delete RF;
  
    for(vector<EX_t*>::iterator it = EX.begin(); it != EX.end(); it++)
        delete *it;
    EX.clear();
}

void inorder_t::set_config(libconfig::Config *parser)
{
    try {
        config.memory_deadlock_avoidance = false;
        config.memory_deadlock_threshold = 50000;
        if(parser->exists("memory_deadlock_avoidance")) {
            config.memory_deadlock_avoidance = parser->lookup("memory_deadlock_avoidance");
            config.memory_deadlock_threshold = parser->lookup("memory_deadlock_threshold");
        }
        
        config.exec_width = parser->lookup("execute_width");
        
        config.instQ_size = parser->lookup("instQ_size");
        config.LDQ_size = parser->lookup("LDQ_size");
        config.STQ_size = parser->lookup("STQ_size");
        
        config.FU_delay[SPX_FU_INT] = parser->lookup("FU_INT.delay");
        config.FU_delay[SPX_FU_MUL] = parser->lookup("FU_MUL.delay");
        config.FU_delay[SPX_FU_FP] = parser->lookup("FU_FP.delay");
        config.FU_delay[SPX_FU_MOV] = parser->lookup("FU_MOV.delay");
        config.FU_delay[SPX_FU_BR] = parser->lookup("FU_BR.delay");
        config.FU_delay[SPX_FU_LD] = parser->lookup("FU_LD.delay");
        config.FU_delay[SPX_FU_ST] = parser->lookup("FU_ST.delay");
        
        config.FU_issue_rate[SPX_FU_INT] = parser->lookup("FU_INT.issue_rate");
        config.FU_issue_rate[SPX_FU_MUL] = parser->lookup("FU_MUL.issue_rate");
        config.FU_issue_rate[SPX_FU_FP] = parser->lookup("FU_FP.issue_rate");
        config.FU_issue_rate[SPX_FU_MOV] = parser->lookup("FU_MOV.issue_rate");
        config.FU_issue_rate[SPX_FU_BR] = parser->lookup("FU_BR.issue_rate");
        config.FU_issue_rate[SPX_FU_LD] = parser->lookup("FU_LD.issue_rate");
        config.FU_issue_rate[SPX_FU_ST] = parser->lookup("FU_ST.issue_rate");

        const char *arch_string = parser->lookup("arch_type");
        switch (str2int(arch_string)) {
            case str2int("x64") : config.arch_type = SPX_X64; break;
            case str2int("a64") : config.arch_type = SPX_A64; break;
            case str2int("x86") : config.arch_type = SPX_X86; break;
            default: 
                fprintf(stderr, "unknown arch_type in config file, please specify among x86/x64/a64\n");
                exit(1);
        }
        fprintf(stdout, "arch_type %d\n", config.arch_type);
 

        Setting *setting;
        setting = &parser->lookup("FU_INT.port");
        for(int i = 0; i < setting->getLength(); i++)
            config.FU_port[SPX_FU_INT].push_back((*setting)[i]);
        setting = &parser->lookup("FU_MUL.port");
        for(int i = 0; i < setting->getLength(); i++)
            config.FU_port[SPX_FU_MUL].push_back((*setting)[i]);
        setting = &parser->lookup("FU_FP.port");
        for(int i = 0; i < setting->getLength(); i++)
            config.FU_port[SPX_FU_FP].push_back((*setting)[i]);
        setting = &parser->lookup("FU_MOV.port");
        for(int i = 0; i < setting->getLength(); i++)
            config.FU_port[SPX_FU_MOV].push_back((*setting)[i]);
        setting = &parser->lookup("FU_BR.port");
        for(int i = 0; i < setting->getLength(); i++)
            config.FU_port[SPX_FU_BR].push_back((*setting)[i]);
        setting = &parser->lookup("FU_LD.port");
        for(int i = 0; i < setting->getLength(); i++)
            config.FU_port[SPX_FU_LD].push_back((*setting)[i]);
        setting = &parser->lookup("FU_ST.port");
        for(int i = 0; i < setting->getLength(); i++)
            config.FU_port[SPX_FU_ST].push_back((*setting)[i]);
            
        // add switch between different arch
        if (config.arch_type == SPX_X86) {
            QSIM_N_REGS = QSIM_X86_N_REGS;
            config.mem_addr_mask = ~0x0;
            config.mem_addr_mask = (config.mem_addr_mask<<32)>>32;
        } else if (config.arch_type == SPX_X64){ // SPX_X64 || SPX_A64
            QSIM_N_REGS = QSIM_X86_N_REGS;
            config.mem_addr_mask = ~0x0;
        } else {
            QSIM_N_REGS = QSIM_ARM64_ENDING;
            config.mem_addr_mask = ~0x0;
        }
        fprintf(stdout,"mem addr mask: 0x%lx\n", config.mem_addr_mask);
 

        config.cache_line_width = 64;
        if(parser->exists("cache_line_width")) {
            config.cache_line_width = parser->lookup("cache_line_width");
        }

#ifdef LIBEI
        cache_config_t cache_config;

        if(parser->exists("inst_cache")) {
            setting = &parser->lookup("inst_cache");
            cache_config.cache_size = (*setting)["cache_size"];
            cache_config.assoc = (*setting)["assoc"];
            cache_config.block_size = (*setting)["block_size"];
            inst_cache = new cache_table_t(SPX_INST_CACHE,0,cache_config,this);
        }

        if(parser->exists("data_tlb")) {
            setting = &parser->lookup("data_tlb");
            uint64_t block_size = (*setting)["block_size"];
            uint64_t page_size = (*setting)["page_size"];
            cache_config.cache_size = (*setting)["cache_size"];
            cache_config.assoc = (*setting)["assoc"];
            cache_config.cache_size = page_size*(cache_config.cache_size/block_size);
            cache_config.block_size = page_size;// Treat it like 4KB-line cache
            data_tlb = new cache_table_t(SPX_DATA_TLB,0,cache_config,this);
        }
    
        if(parser->exists("inst_tlb")) {
            setting = &parser->lookup("inst_tlb");
            uint64_t block_size = (*setting)["block_size"];
            uint64_t page_size = (*setting)["page_size"];
            cache_config.cache_size = (*setting)["cache_size"];
            cache_config.assoc = (*setting)["assoc"];
            cache_config.cache_size = page_size*(cache_config.cache_size/block_size);
            cache_config.block_size = page_size;// Treat it like 4KB-line cache
            inst_tlb = new cache_table_t(SPX_INST_TLB,0,cache_config,this);
        }
        
        if(parser->exists("l2_tlb")) {
            setting = &parser->lookup("l2_tlb");
            uint64_t block_size = (*setting)["block_size"];
            uint64_t page_size = (*setting)["page_size"];
            cache_config.cache_size = (*setting)["cache_size"];
            cache_config.assoc = (*setting)["assoc"];
            cache_config.cache_size = page_size*(cache_config.cache_size/block_size);
            cache_config.block_size = page_size;// Treat it like 4KB-line cache
            l2_tlb = new cache_table_t(SPX_L2_TLB,1,cache_config,this);
            
            assert((data_tlb != NULL)||(inst_tlb != NULL));
            if(data_tlb) { data_tlb->add_next_level(l2_tlb); }
            if(inst_tlb) { inst_tlb->add_next_level(l2_tlb); }
        }
#endif
    }
    catch(SettingNotFoundException e) {
        fprintf(stdout,"%s not defined\n",e.getPath());
        exit(1);
    }
    catch(SettingTypeException e) {
        fprintf(stdout,"%s has incorrect type\n",e.getPath());
        exit(1);
    }
}

void inorder_t::commit()
{
}

void inorder_t::memory()
{
    // nothing to do
}

void inorder_t::execute()
{
    // process execution units
    for(int port = 0; port < config.exec_width; port++) {
        // EX to commit scheduling
        inst_t *inst = EX[port]->get_front();
        if(inst) {
            EX[port]->pop_front(inst->excode); // remove this inst from EX pipe
            stats.last_commit_cycle = core->clock_cycle;
                        
            if(inst->memcode == SPX_MEM_LD) { // load
                LDQ->schedule(inst);
                LDQ->handle_cache();
            }
            else {
                if(inst->memcode == SPX_MEM_ST) { // store
                    STQ->store_forward(inst);
                    STQ->schedule(inst);
                    STQ->handle_cache();
                    
                    threadQ->update(inst); // update all dependent insts
                    RF->writeback(inst); // commit result
                }
                else {
                    threadQ->update(inst); // update all dependent insts
                    RF->writeback(inst); // commit result
                    
                    if(!inst->squashed) delete inst; 
                }
            }
        }
    }
    
    if(Qsim_osd_state != QSIM_OSD_ACTIVE) {
        stats.last_commit_cycle = core->clock_cycle;
    }
    else if(core->clock_cycle - stats.last_commit_cycle > config.memory_deadlock_threshold) {
        // If the pipeline stalls for too long, it is only due to unreturned load request.
        if(config.memory_deadlock_avoidance)
            avoid_memory_deadlock();
        else {
            fprintf(stdout,"SPX_ERROR (core %d): possible pipeline deadlock at %lu\n",core->core_id,stats.last_commit_cycle);
        
            /*if(memory_deadlock_tracker->get_front(SPX_MEM_LD)) {
                inst_t *deadlock_inst = LDQ->handle_deadlock();
                fprintf(stdout,"ROB HEAD INSTS\n");
                while(deadlock_inst) {
                    debug_deadlock_inst(deadlock_inst);
                    deadlock_inst = deadlock_inst->next_inst;
                }
            }*/
            exit(1);
        }
    }

    for(int port = 0; port < config.exec_width; port++) {
        // threadQ to EX scheduling
        inst_t *inst = threadQ->get_front(port);
        if(inst) { // there is a ready instruction to start execution
            if(EX[inst->port]->is_available(inst->excode)) {
                threadQ->pop_front(port);
                EX[inst->port]->push_back(inst);
            }
        }
    }
}

void inorder_t::allocate()
{
    inst_t *inst = instQ->get_front();
    if(inst) {
        if(threadQ->is_available()) {
            if(inst->memcode == SPX_MEM_ST) { // store
                // If there is a store request whose response didn't return for long time,
                // squash the deadlock inst so that new inst can enter.
                /*if(config.memory_deadlock_avoidance)
                    avoid_memory_deadlock(SPX_MEM_ST);*/
                if(STQ->is_available()) 
                    STQ->push_back(inst);
                else {
                    return;
                }
            }
            else if(inst->memcode == SPX_MEM_LD) { // load
                if(LDQ->is_available()) {
                    LDQ->push_back(inst);
                    STQ->mem_disamb_check(inst);
                }
                else {
                    return;
                }
            }
            RF->resolve_dependency(inst);
            threadQ->port_binding(inst);
            threadQ->push_back(inst);

            instQ->pop_front();
        }
    }
}

void inorder_t::frontend()
{
    if(instQ->is_available()&&(Qsim_osd_state == QSIM_OSD_ACTIVE)) {
        stats.Mop_count++;
        stats.uop_count++;
        stats.interval.Mop_count++;
        stats.interval.uop_count++;
          
        next_inst = new inst_t(core,++Mop_count,++uop_count);

        int rc = qsim_proxy->run(core->core_id, 1);

        if(rc) { /* Core is active */
            assert(Qsim_osd_state == QSIM_OSD_ACTIVE);
            if(!next_inst->inflight) {
                Qsim_post_cb(next_inst);
                fetch(next_inst);
            }

            if(next_inst->Mop_head->Mop_length > 4)
                handle_long_inst(next_inst);

            next_inst = NULL;
        }
        else { /* Core is idle or terminated */
            /*assert((Qsim_osd_state == QSIM_OSD_IDLE)||
                   (Qsim_osd_state == QSIM_OSD_TERMINATED));*/
            delete next_inst;

            /* Roll back counters */
            stats.Mop_count--;
            stats.uop_count--;
            stats.interval.Mop_count--;
            stats.interval.uop_count--;
            Mop_count--;
            uop_count--;
        }
    }
}

void inorder_t::fetch(inst_t *inst)
{
    instQ->push_back(inst);
    
    /* Roll back counters for nop instructions */
    if(is_nop(inst)) {
        stats.Mop_count--;
        stats.uop_count--;
        stats.interval.Mop_count--;
        stats.interval.uop_count--;
    }
}

void inorder_t::handle_cache_response(int temp, cache_request_t *cache_request)
{
    inst_t *inst = cache_request->inst;
  
    assert(inst);
    if(core->core_id != core->core_id) {
        fprintf(stdout,"SPX_ERROR (core %d) | %lu : strange cache response uop %lu (Mop %lu) of core %d received\n",core->core_id,core->clock_cycle,inst->uop_sequence,inst->Mop_sequence,core->core_id);
        debug_deadlock_inst(inst);
        exit(1);
    }

#ifdef SPX_DEBUG
    fprintf(stdout,"SPX_DEBUG (core %d) | %lu: cache_response uop %lu (Mop %lu)\n",core->core_id,core->clock_cycle,inst->uop_sequence,inst->Mop_sequence);
#endif

    if(!inst->squashed) {
        switch(cache_request->op_type) {
            case SPX_MEM_LD: {
                LDQ->pop(inst);
                threadQ->update(inst); // update all dependent insts
                RF->writeback(inst); // writeback result
                break;
            }
            case SPX_MEM_ST: {
                STQ->pop(inst);
                break;
            }
            default: {
                fprintf(stdout,"SPX_ERROR (core %d): strange cache response %d received\n",core->core_id,cache_request->op_type);
                exit(1);
            }
        }
    }
    else {
        inst->squashed = false;
#ifdef SPX_DEADLOCK_DEBUG    
        fprintf(stdout,"SPX_DEADLOCK_DEBUG (core %d) | uop %lu (Mop %lu) %s returned in %lu cycles\n",core->core_id,inst->uop_sequence,inst->Mop_sequence,(inst->memcode==SPX_MEM_LD)?"SPX_MEM_LD":"SPX_MEM_ST",core->clock_cycle-inst->memory_request_time_stamp);
#endif
    }

    delete inst;
    delete cache_request;
}

void inorder_t::avoid_memory_deadlock()
{
    inst_t *deadlock_inst = LDQ->handle_deadlock();
    if(deadlock_inst) {
        threadQ->update(deadlock_inst);
        RF->writeback(deadlock_inst);
    }
}

#endif // USE_QSIM
