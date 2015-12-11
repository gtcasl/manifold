#ifdef USE_QSIM

#include "qsimlib-proc.h"
#include <assert.h>

//#include <iostream>
//using namespace std;


namespace manifold {
namespace simple_proc {


//! @param \c osd    The QSim OS domain.
//! @param \c cpuid  The CPU ID of the processor.
//! @param \c id     The node ID of the processor.
QsimLibProcessor :: QsimLibProcessor (Qsim::OSDomain* osd, int cpuid, int id, const SimpleProc_Settings& settings) : SimpleProcessor(id, settings)
{
    m_Qsim_osd = osd;
    m_Qsim_queue = 0;
    m_Qsim_cpuid = cpuid;
    Procs.push_back(this);
}


QsimLibProcessor :: ~QsimLibProcessor()
{
    delete m_Qsim_queue;
}


//! Qsim queue should be created at app start.
void QsimLibProcessor :: create_queue()
{
    m_Qsim_queue = new Qsim::Queue(*m_Qsim_osd, m_Qsim_cpuid);
}


//for Qsim: app start callback
static bool Qsim_inited = false;
static unsigned Count = 0; //this records number of instructions % 1M at the time when app_start_cb is called.
                           //we need this because we need to call timer_interrupt() roughly after each proc
			   //has advanced 1M instructions.



std::vector<QsimLibProcessor*> QsimLibProcessor :: Procs; //static variable.

//! @brief This function does the fast forwarding of the OS domain.
void QsimLibProcessor :: Start_qsim(Qsim::OSDomain* qsim_osd)
{
//With QSim 0.1.2, we use state file and benchmark tar file. The QSim function
//load_file will fast-forward to the point where APP_START is called. Therefore,
//we do not need to do fast-forward anymore.

    for(unsigned i=0; i<Procs.size(); i++)
	Procs[i]->create_queue();

    for(unsigned i=0; i<Procs.size(); i++) {
	//Procs[i]->get_first_pc();
    }
}



typedef enum {
	MEM_RD=0, 
	MEM_WR 
} MemOpType;


Instruction* QsimLibProcessor::fetch ( void )
{
    // Get instruction from qsim
    Qsim::Queue *q_ptr = m_Qsim_queue;
    Instruction* insn = 0;

    if ( q_ptr->empty() ) {
	m_Qsim_osd->run(m_Qsim_cpuid, 100); // can single step or fetch a batch.. to handle atomics you need to single step
        if(m_Qsim_cpuid == Procs.size() - 1) {
	    //When to call timer_interrupt() is tricky. Here we call it after the processor with the largest
	    //ID has advanced 1M instructions. There is no special reason to pick the last one. We might as well
	    //have picked the first one.
	    //Note since the cpu is advanced only after m_Qsim_queue becomes empty,
	    //different processors may have advanced different times at any moment. In one test, CPU 7 has advanced
	    //34 times (10K each) while CPU 4 has only advanced 31 times, because every time CPU 4 advances, it gets
	    //much more in the queue than CPU 7.
	    //An alternative solution is, every time the queue of any processor becomes empty it calls a static
	    //function, which advances all CPUs, and when all have advanced 1M times, timer_interrupt() is called.
	    //The drawback is, if one CPU gets substantially more items in the queue every time, then its size could
	    //become unboundedly large.
	    Count += 100;
	    if(Count == 1000000) {
		m_Qsim_osd->timer_interrupt(); //this is called after the designated proc has advanced 1M instructions
		Count = 0;
	    }
	}
    }


    if(!q_ptr->empty()) {
	if( q_ptr->front().cb_type == Qsim::QueueItem::MEM_CB) {
	    if(q_ptr->front().data.mem_cb.type==MEM_RD) {
//cout << "RD " << hex << "0x" << q_ptr->front().data.mem.paddr << endl;
		insn = new Instruction(Instruction::OpMemLd, q_ptr->front().data.mem_cb.paddr);
	    }
	    else {
	        assert(q_ptr->front().data.mem_cb.type == MEM_WR);

//cout << "WR " << hex << "0x" << q_ptr->front().data.mem.paddr << endl;
		insn =  new Instruction(Instruction::OpMemSt, q_ptr->front().data.mem_cb.paddr);
	    }
	}
	else { //non-mem instruction
	    insn =  new Instruction(Instruction::OpNop);
//cout << "Inst " << hex << "0x" << q_ptr->front().data.inst.paddr << endl;
	}
	q_ptr->pop();
    }

    return insn;
}	




} //namespace simple_proc
} //namespace manifold


#endif //USE_QSIM

