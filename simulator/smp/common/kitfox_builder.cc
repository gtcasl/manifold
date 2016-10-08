#include "kitfox_builder.h"

using namespace manifold::kitfox_proxy;

KitFoxBuilder::KitFoxBuilder(const char* configFile, uint64_t samplingInterval)
{
    component_id = manifold::kernel::Component::Create<kitfox_proxy_t>(0, configFile, samplingInterval);
    proxy = manifold::kernel::Component :: GetComponent<kitfox_proxy_t>(component_id);
    assert(proxy);
}

KitFoxBuilder::~KitFoxBuilder()
{
    assert(proxy);
    delete proxy;
}
