#ifdef USE_QSIM

#include "qsimproxy-core.h"
#include "zesto-fetch.h"
#include <assert.h>

namespace manifold {
namespace zesto {

//! @param \c core_id  ID of the core.
//! @param \c conifg  Name of the config file.
//! @param \c cpuid  QSim cpu ID assigned to this core. Note this may be different from core ID.
qsimproxy_core_t::qsimproxy_core_t(const int core_id, char * config, int cpuid, const QsimProxyClient_Settings& settings) :
    core_t(core_id, config),
    m_Qsim_cpuid(cpuid)
{
    m_shm = new SharedMemBuffer(settings.key_file, cpuid+1, settings.size);
    m_shm->reader_init();


    md_addr_t nextPC = 0;
    fetch_next_pc(&nextPC, this);

    current_thread->regs.regs_PC = nextPC;
    current_thread->regs.regs_NPC = nextPC;
    fetch->PC = current_thread->regs.regs_PC;
}



qsimproxy_core_t:: ~qsimproxy_core_t()
{
}


#define RUN_COUNT 1

//! @return  True if interrupt is seen.
bool qsimproxy_core_t::fetch_next_pc(md_addr_t *nextPC, struct core_t * tcore)
{
//cerr << "fetch_next_pc()\n";
    qsimproxy_core_t* core = dynamic_cast<qsimproxy_core_t*>(tcore);
    assert(core != 0);

    list<Qsim::QueueItem> *q_ptr = &(core->m_q); 
    bool interrupt=false;

    while(true) {
	if(!q_ptr->empty()) {
	    if (q_ptr->front().cb_type == Qsim::QueueItem::INST_CB) {
		*nextPC = q_ptr->front().data.inst_cb.vaddr;
		break;
	    }
	    else if(q_ptr->front().cb_type == Qsim::QueueItem::INT_CB) {
#ifdef ZDEBUG
if(sim_cycle> PRINT_CYCLE)
fprintf(stdout,"\n[%lld][Core%d]Interrupt seen in md_fetch_next_PC",core->sim_cycle,core->id);
#endif
		interrupt=true;
		q_ptr->pop_front();
	    }
	    else if(q_ptr->front().cb_type == Qsim::QueueItem::MEM_CB) {	
		//should this happen?
	        q_ptr->pop_front();
	    }
	}
        else { //queue empty
	    int rc = run(RUN_COUNT);
	    if(rc == 0 && m_shm->is_writer_done() == true) {//no more instructions
	        cerr << "cpu " << core->m_Qsim_cpuid << " out of insn fetching next pc\n";
		manifold :: kernel :: Manifold :: Terminate();
                *nextPC = 0;
		tcore->current_thread->active = false;
		return false;
	    }
	}
    }//while

    return(interrupt);
}	







void qsimproxy_core_t::fetch_inst(md_inst_t *inst, struct mem_t *mem, const md_addr_t pc, core_t * const tcore)
{
    qsimproxy_core_t* core = dynamic_cast<qsimproxy_core_t*>(tcore);
    assert(core != 0);

    list<Qsim::QueueItem> *q_ptr = &(core->m_q); 

    while(true) {
	if(q_ptr->empty()) {
	    int rc = run(RUN_COUNT);
	    if(rc == 0 && m_shm->is_writer_done() == true) {//no more instructions
	        cerr << "cpu " << core->m_Qsim_cpuid << " out of insn\n";
		manifold :: kernel :: Manifold :: Terminate();
		tcore->current_thread->active = false;
		return;
	    }
        }
	else  {
	    if (q_ptr->front().cb_type == Qsim::QueueItem::INST_CB) {
#ifdef ZDEBUG
fprintf(stdout,"\n[%lld][Core%d]Trace DequeuedPC: 0x%llx   ",core->sim_cycle,core->id,q_ptr->front().data.inst_cb.vaddr);
#endif
		core->current_thread->insn_count++;
		assert(q_ptr->front().data.inst_cb.len <= MD_MAX_ILEN); //size of inst->code[] is MD_MAX_ILEN

	        for (unsigned i = 0; i < q_ptr->front().data.inst_cb.len; i++) {
		    inst->code[i] = q_ptr->front().data.inst_cb.bytes[i]; 
	        }
	        inst->vaddr=q_ptr->front().data.inst_cb.vaddr;
	        inst->paddr=q_ptr->front().data.inst_cb.paddr;
	        inst->qemu_len=q_ptr->front().data.inst_cb.len;
	        inst->mem_ops.mem_vaddr_ld[0]=0;
	        inst->mem_ops.mem_vaddr_ld[1]=0;
	        inst->mem_ops.mem_vaddr_str[0]=0;
	        inst->mem_ops.mem_vaddr_str[1]=0;
	        inst->mem_ops.memops=0;
	    }
	    else if (q_ptr->front().cb_type == Qsim::QueueItem::INT_CB) { //interrupt
		q_ptr->pop_front(); //ignore interrupt
		continue;
	    }
	    else {
		fprintf(stdout, "Memory Op found while looking for an instruction!\n");
		q_ptr->pop_front(); //ignore this
		continue;
	    }

	    q_ptr->pop_front();

	    if(q_ptr->empty()) {
	        int rc = run(RUN_COUNT);
		if(rc == 0 && m_shm->is_writer_done() == true) {//no more instructions
		    cerr << "cpu " << core->m_Qsim_cpuid << " out of insn getting mem op\n";
		    manifold :: kernel :: Manifold :: Terminate();
		    tcore->current_thread->active = false;
		    return;
		}
	    }

	    //Process the memory ops following the instruction.
	    while (q_ptr->front().cb_type == Qsim::QueueItem::MEM_CB) {
    
	        if(q_ptr->front().data.mem_cb.type==MEM_RD) { //read
		    if(inst->mem_ops.mem_vaddr_ld[0]==0) {
			inst->mem_ops.mem_vaddr_ld[0]=q_ptr->front().data.mem_cb.vaddr;
			inst->mem_ops.mem_paddr_ld[0]=q_ptr->front().data.mem_cb.paddr;
			inst->mem_ops.ld_dequeued[0]=false;
			inst->mem_ops.ld_size[0]=q_ptr->front().data.mem_cb.size;
			inst->mem_ops.memops++;
		    }
		    else if(inst->mem_ops.mem_vaddr_ld[1]==0) {
		      inst->mem_ops.mem_vaddr_ld[1]=q_ptr->front().data.mem_cb.vaddr;
		      inst->mem_ops.mem_paddr_ld[1]=q_ptr->front().data.mem_cb.paddr;
		      inst->mem_ops.ld_dequeued[1]=false;
		      inst->mem_ops.ld_size[1]=q_ptr->front().data.mem_cb.size;
		      inst->mem_ops.memops++;
		    }
		}
	        else if(q_ptr->front().data.mem_cb.type==MEM_WR) {
		    if(inst->mem_ops.mem_vaddr_str[0]==0) {
		        inst->mem_ops.mem_vaddr_str[0]=q_ptr->front().data.mem_cb.vaddr;
		        inst->mem_ops.mem_paddr_str[0]=q_ptr->front().data.mem_cb.paddr;
		        inst->mem_ops.str_dequeued[0]=false;
		        inst->mem_ops.str_size[0]=q_ptr->front().data.mem_cb.size;
		        inst->mem_ops.memops++;
		    }
		    else if(inst->mem_ops.mem_vaddr_str[1]==0) {
		        inst->mem_ops.mem_vaddr_str[1]=q_ptr->front().data.mem_cb.vaddr;
		        inst->mem_ops.mem_paddr_str[1]=q_ptr->front().data.mem_cb.paddr;
		        inst->mem_ops.str_dequeued[1]=false;
		        inst->mem_ops.str_size[1]=q_ptr->front().data.mem_cb.size;
		        inst->mem_ops.memops++;
		    }
	        }
		else
		    assert(0);

	        q_ptr->pop_front();

	        if(q_ptr->empty())
		    run(RUN_COUNT);
	    }//while MEM ops
	    break; //got one complete instruction
	}

    }//while(true)
}


//return actual number of instructions read
int qsimproxy_core_t :: run(int count)
{
//cout << "run() called, count= " << count << endl;
    Qsim::QueueItem item(0);

    int cnt = 0;
    while(cnt < count) {
        if(m_shm->is_empty()) {
	    cout << "buffer is empty\n";
	    usleep(50000); //0.05s
	    break;
	}
	item = m_shm->read();
	//cout << "run() got an item\n";
	//item.print();
	cnt++;
	m_q.push_back(item);
    }

    return cnt;
}



} //namespace zesto
} //namespace manifold


#endif //USE_QSIM
