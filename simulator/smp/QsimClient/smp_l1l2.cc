//! @file zesto_l1l2.cc
//! @brief This program builds a simple simulator of multicore systems. Its purpose is to
//! demonstrate how to build such a simulator using the Manifold framework.
//! The multicore system is illustrated as follows:
//!
//! @verbatim
//!      -----------       -----------     ----------------      ----------
//!     | processor |     | processor |   | mem controller |    | L2 cache |
//!      -----------       -----------     ----------------      ----------
//!           |                 |                 |                  |
//!        -------           -------              |                  |
//!       | cache |         | cache |             |                  |
//!        -------           -------              |                  |
//!           |                 |                 |                  |
//!           |                 |                 |                  |
//!  ---------------------------------------------------------------------------
//!     ------------      ------------       ------------      ------------ 
//!    | NetIntface |    | NetIntface |     | NetIntface |    | NetIntface |  
//!     ------------      ------------       ------------      ------------ 
//!  ---------------------------------------------------------------------------
//! @endverbatim
//!
//!

#include "../common/sysBuilder_l1l2.h"
#include "kernel/manifold.h"
#include <iostream>
#include "mpi.h"


using namespace std;
using namespace manifold::kernel;


int main(int argc, char** argv)
{
    if(argc != 3 && argc != 4 && argc != 5) {
        cerr << "Usage: mpirun -np <NP> " << argv[0] << " <config_file> <zesto_conf_file> <server_name> <port>" << endl
	     << "                                 <config_file> <server_name> <port>" << endl
	     << "                                 <config_file> <spx_conf_file>" << endl;
	exit(1);
    }


    Manifold::Init(argc, argv, Manifold::TICKED, SyncAlg::SA_CMB_OPT_TICK);


    vector<string> args;
    for(int i=2; i<argc; i++) //get all arguments except program name and config file name.
        args.push_back(argv[i]);

    SysBuilder_l1l2 sysBuilder(argv[1]);

    sysBuilder.config_system();


    int N_LPs = 1; //number of LPs
    MPI_Comm_size(MPI_COMM_WORLD, &N_LPs);
    cout << "Number of LPs = " << N_LPs << endl;

    unsigned expectedLPs = sysBuilder.get_proc_node_size() + 1;

    if(N_LPs != 1 && N_LPs !=2 && N_LPs != expectedLPs) {
        cerr << "Number of LPs must be 1, 2, or " << expectedLPs << "!" << endl;
	exit(1);
    }

#define REDIRECT_COUT

#ifdef REDIRECT_COUT
    // create a file into which to write debug/stats info.
    int Mytid;
    MPI_Comm_rank(MPI_COMM_WORLD, &Mytid);
    char buf[10];
    sprintf(buf, "DBG_LOG%d", Mytid);
    ofstream DBG_LOG(buf);

    //redirect cout to file.
    std::streambuf* cout_sbuf = std::cout.rdbuf(); // save original sbuf
    std::cout.rdbuf(DBG_LOG.rdbuf()); // redirect cout
#endif



    sysBuilder.build_system(SysBuilder_llp::FT_QSIMCLIENT, N_LPs, args, SysBuilder_llp::PART_1);



    //==========================================================================
    //start simulation
    //==========================================================================
    sysBuilder.pre_simulation();
    sysBuilder.print_config(cout);
    Manifold::StopAt(sysBuilder.get_stop_tick());
    Manifold::Run();


    sysBuilder.print_stats();


#ifdef REDIRECT_COUT
    std::cout.rdbuf(cout_sbuf);
#endif

    Manifold :: Finalize();

}
