#ifndef PROC_BUILDER_H
#define PROC_BUILDER_H

#include <libconfig.h++>
#include <map>

#include "kernel/component-decl.h"
#include "cache_builder.h"
#include "qsim_builder.h"
#include "qsim.h"
#include "interrupt_handler/interrupt_handler.h"

#ifdef LIBKITFOX
#include "kitfox_builder.h"
using namespace manifold::kitfox_proxy;
#endif

class ProcBuilder {
public:
    enum ProcType { PROC_ZESTO, PROC_SIMPLE, PROC_SPX}; //processor model type

    enum FEType {INVALID_FE_TYPE=0, QSIMCLIENT, QSIMLIB, QSIMPROXY, TRACE};  //front-end type

    ProcBuilder(SysBuilder_llp* b) : m_fe_type(INVALID_FE_TYPE),
                                     m_sysBuilder(b),
                                     m_qsim_interrupt_handler_clock(0),
                                     m_qsim_interrupt_handler(0) {}
    virtual ~ProcBuilder() {
        delete m_qsim_interrupt_handler_clock;
        delete m_qsim_interrupt_handler;
    }

    virtual ProcType get_proc_type() = 0;

    FEType get_fe_type() { return m_fe_type; }
    void set_fe_type(FEType t) { m_fe_type = t; }

    virtual void read_config(libconfig::Config&) = 0;
    virtual void create_qsimclient_procs(std::map<int,int>& id_lp) = 0;
    virtual void create_qsimlib_procs(std::map<int,int>& id_lp) = 0;
    virtual void create_qsimproxy_procs(std::map<int,int>& id_lp) = 0;
    virtual void create_trace_procs(std::map<int,int>& id_lp) = 0;

#ifdef LIBKITFOX
    virtual void connect_proc_kitfox_proxy(KitFoxBuilder* kitfox_builder) = 0;
#endif

    virtual void connect_proc_cache(CacheBuilder* cache_builder) = 0;
    virtual void connect_proc_qsim_proxy(QsimBuilder* qsim_builder) = 0;
    virtual void print_config(std::ostream&);
    virtual void print_stats(std::ostream&) {}

protected:
    ProcType m_proc_type;
    FEType m_fe_type;  //front-end type
    SysBuilder_llp* m_sysBuilder;

    unsigned m_NUM_PROC; //num of processors
    std::vector<double> m_CLOCK_FREQ; //clock frequency for the processors
    std::vector<manifold::kernel::Clock*> m_clocks;
    bool m_use_default_clock;

    std::map<int, int> m_proc_id_cid_map;

    uint64_t m_QSIM_INTERRUPT_HANDLER_CLOCK_FREQ; // Qsim interrupt handler clock frequency
    manifold::kernel::Clock* m_qsim_interrupt_handler_clock; // Manifold clock object for interrupt handler
    manifold::qsim_interrupt_handler::interrupt_handler_t *m_qsim_interrupt_handler; // Interrupt handler object
};


#if 0
//#####################################################################
//#####################################################################
class Zesto_builder : public ProcBuilder {
public:
    Zesto_builder(SysBuilder_llp* b) : ProcBuilder(b) {}
    #if 0
    Zesto_builder(ProcType type, char* conf, const char* server, int port) :  //qsim server
          ProcBuilder(type), m_conf(conf), m_server(server), m_port(port) {}
    Zesto_builder(ProcType type, SysBuilder_llp* b, char* conf, Qsim::OSDomain* osd) :  //qsim lib
          ProcBuilder(type, b), m_conf(conf), m_qsim_osd(osd) {}
    Zesto_builder(ProcType type, SysBuilder_llp* b, char* conf, const char* trace) :  //trace
          ProcBuilder(type, b), m_conf(conf), m_trace(trace) {}
#endif

    ProcType get_proc_type() { return PROC_ZESTO; }

    void read_config(libconfig::Config&);
    void set_qsimclient_vals(const char* server, int port) {
        m_server = server;
    m_port = port;
    }
    void set_qsimlib_vals(Qsim::OSDomain* osd) {
        m_qsim_osd = osd;
    }
    void set_trace_vals(const char* trace) {
        m_trace = trace;
    }

    void create_qsimclient_procs(std::map<int,int>& id_lp);
    void create_qsimlib_procs(std::map<int,int>& id_lp);
    void create_qsimproxy_procs(std::map<int,int>& id_lp);
    void create_trace_procs(std::map<int,int>& id_lp);

    void connect_proc_cache(CacheBuilder* cache_builder);
    void connect_proc_qsim_proxy(QsimBuilder* qsim_builder);

    void print_config(std::ostream&);
    void print_stats(std::ostream&);
private:
    //char* m_conf; //config file name
    const char* m_server; //server name or IP
    int m_port; //server port
    Qsim::OSDomain* m_qsim_osd;
    const char* m_trace; //trace file name
    std::string m_CONFIG_FILE;
};


//#####################################################################
//#####################################################################
//for simple-proc
class Simple_builder : public ProcBuilder {
public:
    Simple_builder(SysBuilder_llp* b) : ProcBuilder(b) {}
#if 0
    Simple_builder(ProcType type, SysBuilder_llp* b, int l1_line_sz, const char* server, int port) :  //qsim server
          ProcBuilder(type, b), m_l1_line_sz(l1_line_sz), m_server(server), m_port(port) {}
    Simple_builder(ProcType type, SysBuilder_llp* b, int l1_line_sz, Qsim::OSDomain* osd) :  //qsim lib
          ProcBuilder(type, b), m_l1_line_sz(l1_line_sz), m_qsim_osd(osd) {}
    Simple_builder(ProcType type, SysBuilder_llp* b, int l1_line_sz, const char* trace) :  //trace file
          ProcBuilder(type, b), m_l1_line_sz(l1_line_sz), m_trace(trace) {}
#endif
    ProcType get_proc_type() { return PROC_SIMPLE; }

    void read_config(libconfig::Config&) {
    m_use_default_clock = true;
    }
    void set_qsimclient_vals(int l1_line_sz, const char* server, int port) {
        m_l1_line_sz = l1_line_sz;
        m_server = server;
    m_port = port;
    }
    void set_qsimlib_vals(int l1_line_sz, Qsim::OSDomain* osd) {
    m_l1_line_sz = l1_line_sz;
        m_qsim_osd = osd;
    }
    void set_trace_vals(int l1_line_sz, const char* trace) {
        m_l1_line_sz = l1_line_sz;
        m_trace = trace;
    }

    void create_qsimclient_procs(std::map<int,int>& id_lp);
    void create_qsimlib_procs(std::map<int,int>& id_lp);
    void create_qsimproxy_procs(std::map<int,int>& id_lp);
    void create_trace_procs(std::map<int,int>& id_lp);

    void connect_proc_cache(CacheBuilder* cache_builder);
    void connect_proc_qsim_proxy(QsimBuilder* qsim_builder);
    void print_config(std::ostream&);
    void print_stats(std::ostream&);
private:
    int m_l1_line_sz; //L1 line size
    const char* m_server; //server name or IP
    int m_port; //server port
    Qsim::OSDomain* m_qsim_osd;
    const char* m_trace; //trace file name
};
#endif

//#####################################################################
//#####################################################################
class Spx_builder : public ProcBuilder {
public:
    Spx_builder(SysBuilder_llp* b) : ProcBuilder(b) {}
#if 0
    Spx_builder(ProcType type, SysBuilder_llp* b, const char* conf) :  //qsim server
          ProcBuilder(type, b), m_conf(conf) {}
    Spx_builder(ProcType type, SysBuilder_llp* b, const char* conf, Qsim::OSDomain* osd) :  //qsim lib
          ProcBuilder(type, b), m_conf(conf), m_qsim_osd(osd) {}
#endif
    ProcType get_proc_type() { return PROC_SPX; }

    void read_config(libconfig::Config&);

    #if 0
    void set_qsimclient_vals(const char* server, int port) {
        m_server = server;
    m_port = port;
    }
    #endif

    void set_qsimlib_vals(Qsim::OSDomain* osd) {
        m_qsim_osd = osd;
    }

    void create_qsimlib_procs(std::map<int,int>& id_lp); //qsimlib not supported
    void create_qsimclient_procs(std::map<int,int>& id_lp); //qsimclient not supported
    void create_qsimproxy_procs(std::map<int,int>& id_lp);
    void create_trace_procs(std::map<int,int>& id_lp); //trace not supported

#ifdef LIBKITFOX
    void connect_proc_kitfox_proxy(KitFoxBuilder* kitfox_builder);
#endif

    void connect_proc_cache(CacheBuilder* cache_builder);
    void connect_proc_qsim_proxy(QsimBuilder* qsim_builder);

    void print_config(std::ostream&);
    void print_stats(std::ostream&);

private:
    Qsim::OSDomain* m_qsim_osd;
    const char* m_server; //server name or IP
    int m_port; //server port
    std::string m_CONFIG_FILE;

};



#endif // #ifndef PROC_BUILDER_H
