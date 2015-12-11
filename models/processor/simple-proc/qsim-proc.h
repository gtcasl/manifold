#ifndef MANIFOLD_SIMPLE_PROC_QSIM_PROC_H
#define MANIFOLD_SIMPLE_PROC_QSIM_PROC_H

#ifdef USE_QSIM  //this file would be empty if USE_QSIM is not defined.

#include "simple-proc.h"
#include "qsim.h"
#include "qsim-client.h"


namespace manifold {
namespace simple_proc {


struct QsimProc_Settings : public SimpleProc_Settings {
    QsimProc_Settings(const char* server, int port, unsigned cl_size, int msize=16);

    const char* server;
    int port;
};



//! @brief This processor model acts as a QSim client and fetches instructions from QSim server.
//!
class QsimProcessor: public SimpleProcessor
{
    public:
	QsimProcessor (int cpuid, int id, const QsimProc_Settings&);

	~QsimProcessor();

    protected:

    private:

        virtual Instruction* fetch();

	Qsim::Client* m_Qsim_client;
	Qsim::ClientQueue* m_Qsim_queue;
	int m_Qsim_cpuid; //CPU ID
};




} //namespace simple_proc
} //namespace manifold

#endif //USE_QSIM

#endif // MANIFOLD_SIMPLE_PROC_QSIM_PROC_H
