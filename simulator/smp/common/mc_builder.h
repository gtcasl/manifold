#ifndef MC_BUILDER_H
#define MC_BUILDER_H

#include <map>
#include <libconfig.h++>
#include "CaffDRAM/Controller.h"
#include "DRAMSim2/dram_sim.h"
#include "uarch/DestMap.h"



class SysBuilder_llp;
class NetworkBuilder;

class MemControllerBuilder {
public:
    enum {CAFFDRAM, DRAMSIM}; //mc types

    MemControllerBuilder(SysBuilder_llp* b) : m_sysBuilder(b) {}

    virtual ~MemControllerBuilder() {}

    virtual int get_type() = 0;

    virtual void read_config(libconfig::Config&) = 0;
    virtual void create_mcs(std::map<int, int>& id_lp) = 0;
    virtual void connect_mc_network(NetworkBuilder*) = 0;
    virtual void print_config(std::ostream&);
    virtual void print_stats(std::ostream&) = 0;

protected:
    SysBuilder_llp* m_sysBuilder;

    unsigned m_NUM_MC;
    std::vector<double> m_CLOCK_FREQ;
    std::vector<manifold::kernel::Clock*> m_clocks;
    bool m_use_default_clock;
    std::map<int, int> m_mc_id_cid_map; //node id to cid map

};



//builder for CaffDRAM
class CaffDRAM_builder : public MemControllerBuilder {
public:
    CaffDRAM_builder(SysBuilder_llp* b) : MemControllerBuilder(b) {}

    int get_type() { return CAFFDRAM; }

    void read_config(libconfig::Config&);
    void create_mcs(std::map<int, int>& id_lp);
    void connect_mc_network(NetworkBuilder*);

    manifold::caffdram::Dsettings& get_settings() { return m_dram_settings; }

    void print_config(std::ostream&);
    void print_stats(std::ostream&);

private:
    manifold::caffdram::Dsettings m_dram_settings;
    int m_MC_DOWNSTREAM_CREDITS; //credits for sending down to network
    int m_MEM_MSG_TYPE;
    int m_CREDIT_MSG_TYPE;
};



//builder for DramSim
class DramSim_builder : public MemControllerBuilder {
public:
    DramSim_builder(SysBuilder_llp* b) : MemControllerBuilder(b) {}

    int get_type() { return DRAMSIM; }

    void read_config(libconfig::Config&);
    void create_mcs(std::map<int, int>& id_lp);
    void connect_mc_network(NetworkBuilder*);

    void print_config(std::ostream&);
    void print_stats(std::ostream&);

private:
    int m_MC_DOWNSTREAM_CREDITS; //credits for sending down to network
    int m_MEM_MSG_TYPE;
    int m_CREDIT_MSG_TYPE;
    std::string m_DEV_FILE; //device file name
    std::string m_SYS_FILE; //system file name
    unsigned m_MEM_SIZE; //mem size;
};






#endif // #ifndef NETWORK_BUILDER_H
