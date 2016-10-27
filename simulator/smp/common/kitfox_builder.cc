#include "kitfox_builder.h"

using namespace manifold::kitfox_proxy;

KitFoxBuilder::KitFoxBuilder(const char* configFile, uint64_t samplingFreq) : m_config_file(configFile), m_clock_freq(samplingFreq)
{
}

void KitFoxBuilder::create_proxy()
{
    component_id = manifold::kernel::Component::Create<kitfox_proxy_t>(0, m_config_file, m_clock_freq); // kitfox_proxy is in LP 0
    proxy = manifold::kernel::Component :: GetComponent<kitfox_proxy_t>(component_id);
}

KitFoxBuilder::~KitFoxBuilder()
{
    if (proxy)
        delete proxy;
}
