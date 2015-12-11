#ifdef USE_QSIM

#include "qsim-proc.h"
#include "qsim-net.h"
#include <stdio.h>
#include <assert.h>

//#include <iostream>
//using namespace std;


namespace manifold {
namespace simple_proc {


//! @param \c s  Name of ther server.
//! @param \c p  Port of the server where it is listening for incoming connections.
QsimProc_Settings :: QsimProc_Settings(const char* s, int p, unsigned cl_size, int msize) :
SimpleProc_Settings :: SimpleProc_Settings(cl_size, msize),
server(s), port(p)
{
}



//! @param \c cpuid        CPU ID of the processor; could be different from its node ID.
//! @param \c id           Node ID of the processor.
QsimProcessor :: QsimProcessor (int cpuid, int id, const QsimProc_Settings& settings) : SimpleProcessor(id, settings)
{
    m_Qsim_cpuid = cpuid;
    //client_socket() requires port to be passed as a string, so convert to string first.
    char port_str[20];
    sprintf(port_str, "%d", settings.port);
    m_Qsim_client = new Qsim::Client(client_socket(settings.server, port_str));
    m_Qsim_queue = new Qsim::ClientQueue(*m_Qsim_client, m_Qsim_cpuid);
}


QsimProcessor :: ~QsimProcessor()
{
    delete m_Qsim_queue;
    delete m_Qsim_client;
}




static unsigned Count = 0; //This is used by cpu 0 to count the number of instructions requested so
                           //it can call timer_interrupt() periodically.


typedef enum {
	MEM_RD=0, 
	MEM_WR 
} MemOpType;


Instruction* QsimProcessor::fetch ( void )
{
    // Get instruction from qsim
    Qsim::ClientQueue *q_ptr = m_Qsim_queue;
    Instruction* insn = 0;

    if ( q_ptr->empty() ) {
        const int N_INST = 100; //number of instructions to request.

	m_Qsim_client->run(m_Qsim_cpuid, N_INST); // can single step or fetch a batch.. to handle atomics you need to single step
        if(m_Qsim_cpuid == 0) {
	    //When to call timer_interrupt() is tricky. Here we call it after the processor with ID 0
	    //has advanced 1M instructions. Note since the cpu is advanced only after m_Qsim_queue becomes empty,
	    //different processors may have advanced different times at any moment. In one test, CPU 7 has advanced
	    //34 times (10K each) while CPU 4 has only advanced 31 times, because every time CPU 4 advances, it gets
	    //much more in the queue than CPU 7.
	    Count += N_INST;
	    if(Count == 1000000) { //every 1M instructions, cpu 0 calls timer_interrupt().
		m_Qsim_client->timer_interrupt(); //this is called after each proc has advanced 1M instructions
		Count = 0; //reset Count
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

