#ifndef QSIM_PROXY_BUILDER_H
#define QSIM_PROXY_BUILDER_H

#include <libconfig.h++>
#include "kernel/component-decl.h"
#include "qsim.h"

class SysBuilder_llp;

class QsimBuilder {
public:
    QsimBuilder(SysBuilder_llp* b) : m_sysBuilder(b),
                                     qsim_osd(NULL) {}
    virtual ~QsimBuilder() { delete qsim_osd; }

    int get_component_id() const { return component_id; }
    Qsim::OSDomain* get_qsim_osd() const { return qsim_osd; }

    virtual void read_config(libconfig::Config& config, const char* stateFile, const char* appFile) = 0;
    virtual void create_qsim(int LP) = 0;
    virtual void print_config(std::ostream&) = 0;
    virtual void print_stats(std::ostream&) = 0;

protected:
    SysBuilder_llp* m_sysBuilder;
    Qsim::OSDomain *qsim_osd;

    int component_id;
    uint64_t qsim_interrupt_handler_clock;
    char state_file[128];
    char app_file[128];
};



class QsimProxyBuilder : public QsimBuilder {
public:
    QsimProxyBuilder(SysBuilder_llp* b) : QsimBuilder(b) {}
    ~QsimProxyBuilder() {}

    void read_config(libconfig::Config& config, const char* stateFile, const char* appFile);
    void create_qsim(int LP);
    void print_config(std::ostream&);
    void print_stats(std::ostream&);

private:
    uint64_t qsim_interrupt_interval;
};



class QsimLibBuilder : public QsimBuilder {
public:
    QsimLibBuilder(SysBuilder_llp* b) : QsimBuilder(b) {}
    ~QsimLibBuilder() {}

    void read_config(libconfig::Config& config, const char* stateFile, const char* appFile);
    void create_qsim(int LP);
    void print_config(std::ostream&);
    void print_stats(std::ostream&);
};

#endif
