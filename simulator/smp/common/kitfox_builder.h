#ifndef KITFOX_PROXY_BUILDER_H
#define KITFOX_PROXY_BUILDER_H

#include "kernel/component-decl.h"
#include "kitfox_proxy.h"

class SysBuilder_llp;

class KitFoxBuilder {
public:
    KitFoxBuilder(const char* configFile, uint64_t s);
    virtual ~KitFoxBuilder();

    int get_component_id() const { return component_id; }
    manifold::kitfox_proxy::kitfox_proxy_t* get_kitfox() const { return proxy; }

protected:
    manifold::kitfox_proxy::kitfox_proxy_t *proxy;

    int component_id;
};


#endif //KITFOX_PROXY_BUILDER_H
