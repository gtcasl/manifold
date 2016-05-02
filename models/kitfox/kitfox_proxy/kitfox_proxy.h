#ifndef __KITFOX_PROXY_H__
#define __KITFOX_PROXY_H__

#include <assert.h>
#include "kitfox.h"
#include "kernel/component.h"
#include "kernel/clock.h"

namespace manifold {
namespace kitfox_proxy {

class kitfox_proxy_t : public manifold::kernel::Component
{
public:
    kitfox_proxy_t(const char *ConfigFile,
                   uint64_t SamplingInterval);
    ~kitfox_proxy_t();

    void tick();
    void tock();

    /* Add Manifold components to calculate power. */
    void add_manifold_node(manifold::kernel::CompId_t);

    /* Add KitFox components to invoke calculations. */
    void add_kitfox_power_component(libKitFox::Comp_ID);
    void add_kitfox_thermal_component(libKitFox::Comp_ID);
    void add_kitfox_reliability_component(libKitFox::Comp_ID);

private:
    libKitFox::kitfox_t *kitfox;
    uint64_t sampling_interval;

    std::vector<manifold::kernel::CompId_t> manifold_node;
    std::vector<libKitFox::Comp_ID> kitfox_power_component;
    std::vector<libKitFox::Comp_ID> kitfox_thermal_component;
    std::vector<libKitFox::Comp_ID> kitfox_reliability_component;
};

} // namespace kitfox_proxy
} // namespace manifold

#endif

