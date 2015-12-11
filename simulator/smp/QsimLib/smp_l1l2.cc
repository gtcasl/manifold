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
#include "qsim-load.h"


using namespace std;
using namespace manifold::kernel;


int main(int argc, char** argv)
{
    if(argc != 4 && argc != 5) {
        cerr << "Usage: mpirun -np 1 " << argv[0] << " <config_file> <zesto_conf_file> <state_file> <benchmark_tar_file>" << endl
	      << "                              <config_file> <state_file> <benchmark_tar_file>" << endl
	      << "                              <config_file> <spx_conf_file> <state_file> <benchmark_tar_file>" << endl;
	exit(1);
    }

    Manifold::Init(argc, argv);


    vector<string> args;
    if(argc == 5)
        args.push_back(argv[2]);

    SysBuilder_l1l2 sysBuilder(argv[1]);

    sysBuilder.config_system();


    int N_LPs = 1; //number of LPs
    MPI_Comm_size(MPI_COMM_WORLD, &N_LPs);
    cout << "Number of LPs = " << N_LPs << endl;


    if(N_LPs != 1) {
        cerr << "Number of LPs must be 1 !" << endl;
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

    int state_idx = 3;
    int benchmark_idx = 4;
    if(argc == 5) {
	state_idx = 3;
	benchmark_idx = 4;
    }
    else if(argc == 4) {
	state_idx = 2;
	benchmark_idx = 3;
    }
    std::cout << "Loading state...\n";
    Qsim::OSDomain* qsim_osd = new Qsim::OSDomain(argv[state_idx]);
    std::cout << "Loading app " << argv[benchmark_idx] << " ...\n";
    Qsim::load_file(*qsim_osd, argv[benchmark_idx]);
    std::cout << "Finished loading app.\n";


    sysBuilder.build_system(qsim_osd, args);


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
