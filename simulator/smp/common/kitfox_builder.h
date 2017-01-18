#ifndef KITFOX_PROXY_BUILDER_H
#define KITFOX_PROXY_BUILDER_H

#define USE_KITFOX 1

#include "kernel/component-decl.h"
#include "kitfox_proxy.h"

class SysBuilder_llp;

class KitFoxBuilder {
public:
    KitFoxBuilder(const char* configFile, uint64_t s);
    virtual ~KitFoxBuilder();

    int get_component_id() const { return component_id; }
    void create_proxy();
    manifold::kitfox_proxy::kitfox_proxy_t* get_kitfox() const { return proxy; }

protected:
    manifold::kitfox_proxy::kitfox_proxy_t *proxy;
    const char* m_config_file;
    const uint64_t m_clock_freq;

    int component_id;
};


#endif //KITFOX_PROXY_BUILDER_H
