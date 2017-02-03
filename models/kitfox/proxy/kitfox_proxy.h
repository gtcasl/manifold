#ifndef __KITFOX_PROXY_H__
#define __KITFOX_PROXY_H__
#ifdef USE_KITFOX

#include <assert.h>
#include <string>
#include "kitfox.h"
#include "kernel/component.h"
#include "kernel/clock.h"
#include "uarch/kitfoxCounter.h"

namespace manifold {
namespace kitfox_proxy {

template <typename T>
class kitfox_proxy_request_t
{
public:
    kitfox_proxy_request_t() {}
    kitfox_proxy_request_t(int NodeID, manifold::uarch::KitFoxType s, manifold::kernel::Time_t t) : node_id(NodeID), type(s), time(t) {  counter.clear(); }
    ~kitfox_proxy_request_t() {}
    int get_id() const { return node_id; }
    manifold::uarch::KitFoxType get_type()  const { return type; }
    manifold::kernel::Time_t get_time() const { return time; }
    void set_counter (const T c) { counter = c; }
    T get_counter (void) { return counter;}

    // kitfox_proxy_request_t operator=(const kitfox_proxy_request_t &_)
    // {
    //     core_id = _.core_id;
    //     time = _.time;
    //     counter = _.counter;
    //     name = _.name;

    //     return *this;
    // }

private:
    int node_id;
    manifold::kernel::Time_t time;
    manifold::uarch::KitFoxType type;
    T counter;
};

class kitfox_proxy_t : public manifold::kernel::Component
{
public:
    kitfox_proxy_t(const char *ConfigFile,
                   uint64_t SamplingFreq);
    ~kitfox_proxy_t();

    void tick();
    void tock();

    /* Add Manifold components to calculate power. */
    void add_manifold_node(manifold::kernel::CompId_t, manifold::uarch::KitFoxType);
#if 0
    void pair_counter_to_power_component(manifold::kernel::CompId_t);
    /* Add KitFox components to invoke calculations. */
    void add_kitfox_power_component(libKitFox::Comp_ID);
    void add_kitfox_thermal_component(libKitFox::Comp_ID);
    void add_kitfox_reliability_component(libKitFox::Comp_ID);
#endif

    void calculate_power(manifold::uarch::pipeline_counter_t c, manifold::kernel::Time_t t, const string prefix);
    void calculate_power(manifold::uarch::cache_counter_t c, manifold::kernel::Time_t t, const string prefix);
    template <typename T> void handle_kitfox_proxy_response(int temp, kitfox_proxy_request_t<T> *Req);

private:
    libKitFox::kitfox_t *kitfox;
    size_t comp_num;

    std::vector<std::pair<manifold::kernel::CompId_t, manifold::uarch::KitFoxType>> manifold_node;
#if 0
    std::vector<std::pair<libKitFox::Comp_ID, libKitFox::counter_t*> > kitfox_power_component;
    std::vector<libKitFox::Comp_ID> kitfox_thermal_component;
    std::vector<libKitFox::Comp_ID> kitfox_reliability_component;
#endif
};


template <typename T>
void kitfox_proxy_t::handle_kitfox_proxy_response(int temp, kitfox_proxy_request_t<T> *Req)
{
    assert(Req != NULL);
    T c = Req->get_counter();
    string prefix;

    if (Req->get_type() == manifold::uarch::KitFoxType::core_type) {
        prefix = "package.core_die.core" + std::to_string(Req->get_id());
    } else if (Req->get_type() == manifold::uarch::KitFoxType::l1cache_type) {
        prefix = "package.core_die.l1cache" + std::to_string(Req->get_id());
    } else if (Req->get_type() == manifold::uarch::KitFoxType::l2cache_type) {
        prefix = "package.llc_die.cache" + std::to_string(Req->get_id());
    } else {
        cerr << "unknown counter type!" << endl;
        exit(1);
    }
    calculate_power(c, Req->get_time(), prefix);

    comp_num += 1;
    if (comp_num == manifold_node.size()) {
        libKitFox::Comp_ID package_id = kitfox->get_component_id("package");
        assert(package_id != INVALID_COMP_ID);
        kitfox->calculate_temperature(package_id, Req->get_time(), m_clk->period);

        for(unsigned int i = 0; i < manifold_node.size(); i++){
            libKitFox::Kelvin t;
            string partition;
            libKitFox::Comp_ID par_id;

            switch (manifold_node[i].second) {
            case manifold::uarch::KitFoxType::core_type: {
                partition = "package.core_die.core" + std::to_string(i);
                break;
            }
            case manifold::uarch::KitFoxType::l1cache_type: {
                partition = "package.core_die.l1cache" + std::to_string(i);
                break;
            }
            case manifold::uarch::KitFoxType::l2cache_type: {
                partition = "package.llc_die.cache" + std::to_string(i);
                break;
            }
            default:
                cerr << "error manifold_node type!" << endl;
                exit(1);
            }

            par_id = kitfox->get_component_id(partition);
            assert(kitfox->pull_data(par_id, Req->get_time(), m_clk->period, libKitFox::KITFOX_DATA_TEMPERATURE, &t) == libKitFox::KITFOX_QUEUE_ERROR_NONE);

            cerr << partition + ".temperature = " << t << "Kelvin @" << Req->get_time() << endl;
        }

        // Print thermal grid
#if 0
        libKitFox::grid_t<libKitFox::Kelvin> thermal_grid;
        assert(kitfox->pull_data(package_id, Req->get_time(), m_clk->period, libKitFox::KITFOX_DATA_THERMAL_GRID, &thermal_grid) == libKitFox::KITFOX_QUEUE_ERROR_NONE);

        for(unsigned z = 0; z < thermal_grid.dies(); z++) {
            printf("die %d:\n", z);
            for(unsigned x = 0; x < thermal_grid.rows(); x++) {
                for(unsigned y = 0; y < thermal_grid.cols(); y++) {
                    printf("%3.2lf ", thermal_grid(x, y, z));
                }
                printf("\n");
            }
            printf("\n");
        }
        printf("\n");
#endif

        // reset kitfox component counter
        comp_num = 0;
    }

    delete Req;
}


} // namespace kitfox_proxy
} // namespace manifold

#endif // USE_KITFOX
#endif
