#include "trace-proc.h"
#include <stdlib.h>
#include <assert.h>
#include <sstream>


#ifdef DBG_SIMPLE_PROC
#include "kernel/manifold.h"
using namespace manifold::kernel;
#endif

using namespace std;

namespace manifold {
namespace simple_proc {


//!
//! @param \c id  Node ID of the processor.
//! @param \c fname  File name of the trace file.
TraceProcessor::TraceProcessor (int id, string fname, const SimpleProc_Settings& settings) : SimpleProcessor(id, settings)
{
    m_trace_file.open(fname.c_str());

    if(!m_trace_file.is_open()) {
	cerr << "Error: Could no open trace file " << fname << endl;
	exit(1);
    }
    m_next_inst_idx = 0;
    m_num_inst = 0;
}


TraceProcessor::~TraceProcessor ()
{
    if(m_trace_file.is_open())
        m_trace_file.close();
}



//! We use a buffer to hold instructions read from the trace file. This is to
//! reduce the number of file reads. When fetch() is called, we check first if
//! there's any instruction left in the buffer, and read file only if nothing is
//! left in the buffer.
Instruction* TraceProcessor::fetch ( void )
{
    if(m_next_inst_idx < m_num_inst) {
	Instruction* inst = m_instructions[m_next_inst_idx++];
	return inst;
    }

    //no more left in m_instructions, get more if any.

    if(!m_trace_file.is_open()) //file closed, meaning no more to read.
        return 0;


    //Read BUF_SIZE instructions or until end of file, whichever comes first.
    string line;
    int nline=0;
    while(nline < BUF_SIZE && getline(m_trace_file, line) ) {
	istringstream strin(line);

	uint64_t c;
	strin >>hex>> c >>dec;
	if(c == 0x0) {
	    uint64_t a;
	    strin >>hex>> a >>dec;
	    m_instructions[nline] = new Instruction(Instruction::OpMemLd, a);
	}
	else if(c == 0x1) {
	    uint64_t a;
	    strin >>hex>> a >>dec;
	    m_instructions[nline] = new Instruction(Instruction::OpMemSt, a);
	}
	else {
	    m_instructions[nline] = new Instruction(Instruction::OpNop);
	}

	nline++;
    }

    if(m_trace_file.eof()) {
        m_trace_file.close();
    }

    m_num_inst = nline;
    m_next_inst_idx = 0;
    if(m_next_inst_idx < m_num_inst) {
	Instruction* inst = m_instructions[m_next_inst_idx++];
	return inst;
    }
    else
        return 0;
}



} //namespace simple_proc
} //namespace manifold


