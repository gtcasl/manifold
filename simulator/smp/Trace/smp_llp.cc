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


using namespace std;
using namespace manifold::kernel;


int main(int argc, char** argv)
{
    if(argc != 3 && argc != 4) {
        cerr << "Usage: mpirun -np <NP> " << argv[0] << " <config_file>  <zesto_config_file>  <trace_file_basename>" << endl
	     << "                                 <config_file> <trace_file_basename>" << endl;
	cerr << "  NP the number of MPI tasks." << endl;
	exit(1);
    }


    Manifold::Init(argc, argv, Manifold::TICKED, SyncAlg::SA_CMB_OPT_TICK);


    vector<string> args;
    for(int i=2; i<argc; i++) //get all arguments except program name and config file name.
        args.push_back(argv[i]);

    SysBuilder_llp sysBuilder(argv[1]);

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



    sysBuilder.build_system(SysBuilder_llp::FT_TRACE, N_LPs, args, SysBuilder_llp::PART_1);



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
