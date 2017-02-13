#include "qsim_builder.h"
#include "sysBuilder_llp.h"
#include "proxy/proxy.h"
#include "qsim-load.h"

using namespace libconfig;
using namespace manifold::kernel;
using namespace manifold::uarch;
using namespace manifold::qsim_proxy;
using namespace Qsim;

void QsimProxyBuilder::read_config(Config& config, const char*appFile)
{
    try {
        qsim_interrupt_handler_clock = config.lookup("qsim_interrupt_handler_clock");
        const char* stateFile = config.lookup("processor.state");
        strcpy(state_file,stateFile);
        uint64_t default_clock = config.lookup("default_clock");
        qsim_interrupt_interval = default_clock/qsim_interrupt_handler_clock;
    }
    catch(SettingNotFoundException e) {
        cout << e.getPath() << " not set." << endl;
        exit(1);
    }
    catch(SettingTypeException e) {
        cout << e.getPath() << " has incorrect type." << endl;
        exit(1);
    }

    strcpy(app_file, appFile);
}

void QsimProxyBuilder::create_qsim(int LP)
{
    component_id = Component::Create<qsim_proxy_t>(LP, state_file, app_file, qsim_interrupt_interval);
    qsim_proxy_t *qsim_proxy = manifold::kernel::Component :: GetComponent<qsim_proxy_t>(component_id);

    if(qsim_proxy) {
	    Clock* clk = m_sysBuilder->get_default_clock();
        Clock :: Register(*clk, (qsim_proxy_t*)qsim_proxy, &qsim_proxy_t::tick, (void(qsim_proxy_t::*)(void))0);
    }
}

void QsimProxyBuilder::print_config(std::ostream& out)
{
   out << "Qsim type: Qsim Proxy\n";
   out << "  state file: " << state_file << "\n";
   out << "  app file: " << app_file << "\n";
}

void QsimProxyBuilder::print_stats(std::ostream& out)
{
    /* Nothiing to do */
}



void QsimLibBuilder::read_config(Config& config, const char* appFile)
{
    try {
        qsim_interrupt_handler_clock = config.lookup("qsim_interrupt_handler_clock");
        const char* stateFile = config.lookup("processor.state");
        strcpy(state_file,stateFile);
    }
    catch(SettingNotFoundException e) {
        cout << e.getPath() << " not set." << endl;
        exit(1);
    }
    catch(SettingTypeException e) {
        cout << e.getPath() << " has incorrect type." << endl;
        exit(1);
    }
    strcpy(app_file, appFile);
}

void QsimLibBuilder::create_qsim(int LP)
{
    cout << "Loading Qsim state file ..." << endl;
    OSDomain *qsim_osd = new Qsim::OSDomain(state_file);

    cout << "Loading application ..." << endl;
    load_file(*qsim_osd, app_file);

    cout << "Finished initializing Qsim" << endl;
}

void QsimLibBuilder::print_config(std::ostream& out)
{
   out << "  Qsim type: Qsim Proxy\n"; 
}

void QsimLibBuilder::print_stats(std::ostream& out)
{
    /* Nothiing to do */
}
