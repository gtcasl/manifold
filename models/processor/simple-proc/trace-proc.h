#ifndef  MANIFOLD_SIMPLE_PROC_TRACE_PROC_H
#define  MANIFOLD_SIMPLE_PROC_TRACE_PROC_H

#include "simple-proc.h"
#include <fstream>
#include <string>


namespace manifold {
namespace simple_proc {


//! @brief Fetches instructions from a trace file.
//!
class TraceProcessor: public SimpleProcessor
{
    public:
        TraceProcessor (int id, std::string fname, const SimpleProc_Settings&);
        ~TraceProcessor ();       

    protected:
#ifdef SIMPLE_PROC_UTEST
    public:
#else
    private:
#endif

        virtual Instruction* fetch();

        std::ifstream m_trace_file;
	int m_next_inst_idx; //next instruction in the buffer
	int m_num_inst; //number of instructions in the buffer
	static const int BUF_SIZE = 4096;
	Instruction* m_instructions[BUF_SIZE];
};




} //namespace simple_proc
} //namespace manifold




#endif // MANIFOLD_SIMPLE_PROC_TRACE_PROC_H
