#ifndef MANIFOLD_SIMPLE_PROC_QSIMLIB_PROC_H
#define MANIFOLD_SIMPLE_PROC_QSIMLIB_PROC_H

#ifdef USE_QSIM  //this file would be empty if USE_QSIM is not defined.

#include "simple-proc.h"
#include "qsim.h"


namespace manifold {
namespace simple_proc {


//! @brief This processor model is built with QSim library and fetches instructions from QSim.
//!
//! As all processors must use the same QSim OS domain, when using this processor model, all
//! the processors must belong to one process. In an MPI program, all the processors must be
//! in one MPI task.
class QsimLibProcessor: public SimpleProcessor
{
    public:
        QsimLibProcessor (Qsim::OSDomain*, int cpuid, int id, const SimpleProc_Settings&);

        //Since there should only be one OS domain, we use a static function to do the
	//fast forwarding of the OS domain.
	static void Start_qsim(Qsim::OSDomain*);
	static void App_start_cb(int);


	~QsimLibProcessor();

    protected:

    private:
        friend class AppStartCB;

        //This vector stores pointers to all processors in the program.
        static std::vector<QsimLibProcessor*> Procs;

	void create_queue();

        virtual Instruction* fetch();

	Qsim::OSDomain* m_Qsim_osd;
	Qsim::Queue* m_Qsim_queue;
	int m_Qsim_cpuid; //This tells us which Queue to use.
};




} //namespace simple_proc
} //namespace manifold

#endif //USE_QSIM

#endif // MANIFOLD_SIMPLE_PROC_QSIMLIB_PROC_H
