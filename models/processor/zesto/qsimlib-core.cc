#ifdef USE_QSIM

#include "qsimlib-core.h"
#include "zesto-fetch.h"
#include <assert.h>
#include "kernel/manifold-decl.h"

namespace manifold {
namespace zesto {

//! @param \c core_id  ID of the core.
//! @param \c conifg  Name of the config file.
//! @param \c osd  The QSim OS domain object.
//! @param \c cpuid  QSim cpu ID assigned to this core. Note this may be different from core ID.
qsimlib_core_t::qsimlib_core_t(const int core_id, char * config, Qsim::OSDomain* osd, int cpuid) :
    core_t(core_id, config),
    m_Qsim_cpuid(cpuid)
{
  m_Qsim_osd = osd;
  m_Qsim_queue = 0;
  m_got_first_pc = false;
  Procs.push_back(this);
}



qsimlib_core_t:: ~qsimlib_core_t()
{
    delete m_Qsim_queue;
}

//! Qsim queue should be created at app start.
void qsimlib_core_t :: create_queue()
{
    m_Qsim_queue = new Qsim::Queue(*m_Qsim_osd, m_Qsim_cpuid);
}

//! Get the first PC to get things going; should be called before 1st tick.
void qsimlib_core_t :: get_first_pc()
{
    if(m_got_first_pc == false) {
        m_got_first_pc = true;
        md_addr_t nextPC = 0;
        fetch_next_pc(&nextPC,this);

        current_thread->regs.regs_PC = nextPC;
        current_thread->regs.regs_NPC = nextPC;
        fetch->PC = current_thread->regs.regs_PC;
    }
}


//for Qsim: app start callback
static unsigned Count = 0; //this records number of instructions % 1M at the time when app_start_cb is called.
                           //we need this because we need to call timer_interrupt() roughly after each proc
			   //has advanced 1M instructions.



std::vector<qsimlib_core_t*> qsimlib_core_t :: Procs; //static variable.


/*
//application-end callback
class AppEnd {
public:
    void app_end_cb(int c) 
    {
        manifold::kernel::Manifold :: Terminate();
    }
};

static AppEnd MyAppEnd;
*/


void qsimlib_core_t :: Start_qsim(Qsim::OSDomain* qsim_osd)
{
//With QSim 0.1.2, we use state file and benchmark tar file. The QSim function
//load_file will fast-forward to the point where APP_START is called. Therefore,
//we do not need to do fast-forward anymore.

    for(unsigned i=0; i<qsimlib_core_t::Procs.size(); i++)
	qsimlib_core_t::Procs[i]->create_queue();

    for(unsigned i=0; i<qsimlib_core_t::Procs.size(); i++) {
	Procs[i]->get_first_pc();
    }

    //qsim_osd->set_app_end_cb(&MyAppEnd, &AppEnd :: app_end_cb);
}



//! @return  True if interrupt is seen.
bool qsimlib_core_t::fetch_next_pc(md_addr_t *nextPC, struct core_t * tcore)
{
    qsimlib_core_t* core = dynamic_cast<qsimlib_core_t*>(tcore);
    assert(core != 0);

    Qsim::Queue *q_ptr = core->m_Qsim_queue; 
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
		q_ptr->pop();
	    }
	    else if(q_ptr->front().cb_type == Qsim::QueueItem::MEM_CB) {	
	        q_ptr->pop();
	    }
	}
        else { //queue empty
	    int rc = core->m_Qsim_osd->run(core->m_Qsim_cpuid, 1);
	    if(rc == 0 && core->m_Qsim_osd->booted(core->m_Qsim_cpuid) == false) {//no more instructions
	        cout << "cpu " << core->m_Qsim_cpuid << " out of insn pc\n";
		manifold :: kernel :: Manifold :: Terminate();
		*nextPC = 0;
		tcore->current_thread->active = false;
		return false;
	    }

	    if(m_Qsim_cpuid == Procs.size() - 1) { //processor with largest cpuid calls timer_interrupt
		Count += 1;
		if(Count >= 1000000) {
		    m_Qsim_osd->timer_interrupt(); //this is called after the designated proc has advanced 1M instructions
		    Count = 0;
		}
	    }

	}
    }

    return(interrupt);
}

void qsimlib_core_t::fetch_inst(md_inst_t *inst, struct mem_t *mem, const md_addr_t pc, core_t * const tcore)
{
    qsimlib_core_t* core = dynamic_cast<qsimlib_core_t*>(tcore);
    assert(core != 0);

    Qsim::Queue *q_ptr = core->m_Qsim_queue;

    while(true) {
	if(q_ptr->empty()) {
	    int rc = core->m_Qsim_osd->run(core->m_Qsim_cpuid, 1);
	    if(rc == 0 && core->m_Qsim_osd->booted(core->m_Qsim_cpuid) == false) {//no more instructions
	        cerr << "cpu " << core->m_Qsim_cpuid << " outof insn\n";
		manifold :: kernel :: Manifold :: Terminate();
		tcore->current_thread->active = false;
		return;
	    }
	    if(m_Qsim_cpuid == Procs.size() - 1) { //processor with largest cpuid calls timer_interrupt
		Count += 1;
		if(Count >= 1000000) {
		    m_Qsim_osd->timer_interrupt(); //this is called after the designated proc has advanced 1M instructions
		    Count = 0;
		}
	    }
        }
	else  {
	    if (q_ptr->front().cb_type == Qsim::QueueItem::INST_CB) {
#ifdef ZDEBUG
fprintf(stdout,"\n[%lld][Core%d]Trace DequeuedPC: 0x%llx   ",core->sim_cycle,core->id,q_ptr->front().data.inst.vaddr);
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
		q_ptr->pop(); //ignore interrupt
		continue;
	    }
	    else {
		fprintf(stdout, "Memory Op found while looking for an instruction!\n");
		q_ptr->pop(); //ignore this
		continue;
	    }

	    q_ptr->pop();

	    if(q_ptr->empty()) {
	        int rc = core->m_Qsim_osd->run(core->m_Qsim_cpuid, 1);
		if(rc == 0 && core->m_Qsim_osd->booted(core->m_Qsim_cpuid) == false) {//no more instructions
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

	        q_ptr->pop();

	        if(q_ptr->empty())
		    core->m_Qsim_osd->run(core->m_Qsim_cpuid, 1);
	    }//while MEM ops
	    break; //got one complete instruction
	}

    }//while(true)

}





} // namespace zesto
} // namespace manifold

#endif //USE_QSIM
