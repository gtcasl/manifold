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
    if(argc != 4) {
        cerr << "Usage: mpirun -np <NP> " << argv[0] << " <config_file> <state_file> <benchmark_tar_file>" << endl;
	    exit(1);
    }

    Manifold::Init(argc, argv);

    vector<string> args;

    SysBuilder_llp sysBuilder(argv[1]);

    sysBuilder.config_system();


    int N_LPs = 1; //number of LPs
    MPI_Comm_size(MPI_COMM_WORLD, &N_LPs);
    cout << "Number of LPs = " << N_LPs << endl;


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

    sysBuilder.build_system(args, argv[2], argv[3], N_LPs, SysBuilder_llp::PART_1);


    //==========================================================================
    //start simulation
    //==========================================================================
    sysBuilder.pre_simulation();
    sysBuilder.print_config(cout);
    Manifold::StopAt(sysBuilder.get_stop_tick());
    Manifold::Run();


    sysBuilder.print_stats(cerr);


#ifdef REDIRECT_COUT
    std::cout.rdbuf(cout_sbuf);
#endif

    Manifold::Finalize();
}

