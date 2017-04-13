//! @file smp_llp.cc
//! @brief This program builds a simple simulator of multicore systems. Its purpose is to
//! demonstrate how to build such a simulator using the Manifold framework.
//! The multicore system is illustrated as follows:
//!
//! @verbatim
//!                                              ------------      ------------
//!      -----------       -----------          |   memory   |    |   memory   |
//!     | processor |     | processor |         | controller |    | controller |
//!      -----------       -----------           ------------      ------------
//!           |                 |                     |                  |
//!        ----------           ----------            |                  |
//!       | L1 cache |         | L1 cache |           |                  |
//!        ----------           ----------            |                  |
//!         |     |              |     |              |                  |
//!  ----------   |       ----------   |              |                  |
//! | L2 cache |  |      | L2 cache |  |              |                  |
//!  ----------   |       ----------   |              |                  |
//!       |       |            |       |              |                  |
//!        ----   |             ----   |              |                  |
//!            |  |                 |  |              |                  |
//!           -----                -----              |                  |
//!          | mux |              | mux |             |                  |
//!           -----                -----              |                  |
//!             |                    |                |                  |
//!  ---------------------------------------------------------------------------
//!       ------------         ------------       ------------      ------------ 
//!      | NetIntface |       | NetIntface |     | NetIntface |    | NetIntface |  
//!       ------------         ------------       ------------      ------------ 
//!  ---------------------------------------------------------------------------
//! @endverbatim
//!
//!
#include "../common/sysBuilder_llp.h"
#include "kernel/manifold.h"
#include <iostream>
#include "mpi.h"
#include "qsim-load.h"


using namespace std;
using namespace manifold::kernel;


int main(int argc, char** argv)
{
    if(argc < 4) {
        cerr << "Usage: mpirun -np <NP> " << argv[0] << " <config_file> <benchmark_tar_file> <log_file>" << endl;
	    exit(1);
    }
    
    // parse command args
    const char *config = argv[1];
    const char *benchmark = argv[2];
    const char *logfile = nullptr;
    if (argc > 3) {
      logfile = argv[3];    
    }

    Manifold::Init(argc, argv);

    vector<string> args;

    SysBuilder_llp sysBuilder(config);

    sysBuilder.config_system();

    int N_LPs = 1; //number of LPs
    MPI_Comm_size(MPI_COMM_WORLD, &N_LPs);
    cout << "Number of LPs = " << N_LPs << endl;

    ofstream ofsout;
    std::streambuf *old_cout_buf = nullptr;
    if (logfile) {
      // redirect cout to file.
      int Mytid;
      MPI_Comm_rank(MPI_COMM_WORLD, &Mytid);
      if (Mytid == 0) {      
        ofsout.open(logfile);      
        old_cout_buf = std::cout.rdbuf(); // save original sbuf
        std::cout.rdbuf(ofsout.rdbuf());
      }
    }

    sysBuilder.build_system(args, benchmark, N_LPs, SysBuilder_llp::PART_1);

    //==========================================================================
    //start simulation
    //==========================================================================
    sysBuilder.pre_simulation();
    sysBuilder.print_config(cout);
    Manifold::StopAt(sysBuilder.get_stop_tick());
    Manifold::Run();

    sysBuilder.print_stats(cerr);

    if (old_cout_buf) {
      std::cout.rdbuf(old_cout_buf);
    }

    Manifold::Finalize();
}

