#include "kitfox_proxy.h"
#include "kitfox.h"

using namespace std;
using namespace libKitFox;
using namespace manifold;
using namespace manifold::kernel;
using namespace manifold::kitfox_proxy;

kitfox_proxy_t::kitfox_proxy_t(const char *ConfigFile, 
                               uint64_t SamplingInterval) :
    kitfox(NULL),
    sampling_interval(SamplingInterval)
{
    kitfox = new kitfox_t(/* Intra (local) MPI Comm */ NULL, 
                          /* Inter MPI Comm */ NULL);

    cout << "Initializing KitFox ..." << endl;
    kitfox->configure(ConfigFile);

    /* Get the pair of <first ID, last ID> of KitFox pseudo components */
    pair<Comp_ID, Comp_ID> kitfox_component_id = 
    kitfox->component_id_range;

    /* Check which components are associated with which libraries. */
    for(Comp_ID pseudo_component_id = kitfox_component_id.first;
        pseudo_component_id <= kitfox_component_id.second;
        pseudo_component_id++) {
        /* Pseudo component is associated with energy library. */
        if(kitfox->get_energy_library(pseudo_component_id)) {
            add_kitfox_power_component(pseudo_component_id);
        }
        /* Pseudo component is associated with thermal library. */
        if(kitfox->get_thermal_library(pseudo_component_id)) {
            add_kitfox_thermal_component(pseudo_component_id);
        }
        /* Pseudo component is associated with reliability library. */
        if(kitfox->get_reliability_library(pseudo_component_id)) {
            add_kitfox_reliability_component(pseudo_component_id);
        }
    }

    cout << "Finished initializing KitFox" << endl;
}


kitfox_proxy_t::~kitfox_proxy_t()
{
    cout << "Terminating KitFox" << endl;
    delete kitfox;
}


void kitfox_proxy_t::tick()
{
    uint64_t clock_cycle = m_clk->NowTicks();

    /* Invoke KitFox calculations */
    if(clock_cycle % sampling_interval == 0) {
        /* Collect counters from Manifold nodes. */
        map<libKitFox::Comp_ID, libKitFox::counter_t> counters;
        for(vector<CompId_t>::iterator it = manifold_node.begin();
            it != manifold_node.end(); it++) {
        }

        /* Invoke calculate_power() */
        for(vector<Comp_ID>::iterator it = kitfox_power_component.begin();
            it != kitfox_power_component.end(); it++) {
        }

        /* Invoke calculate_temperature() */
        for(vector<Comp_ID>::iterator it = kitfox_power_component.begin();
            it != kitfox_power_component.end(); it++) {
        }

        /* Invoke calculate_failure_rate() */
        for(vector<Comp_ID>::iterator it = kitfox_power_component.begin();
            it != kitfox_power_component.end(); it++) {
        }
    }
}


void kitfox_proxy_t::tock()
{
    /* Nothing to do */
}

/* Add Manifold components to withdraw couters. */
void kitfox_proxy_t::add_manifold_node(CompId_t NodeId)
{
    /* Check if there already exists the same node ID. */
    for(vector<CompId_t>::iterator it = manifold_node.begin();
        it != manifold_node.end(); it++) {
        if((*it) == NodeId) { return; }
    }

    manifold_node.push_back(NodeId);
}

/* Add KitFox components to calculate power. */
void kitfox_proxy_t::add_kitfox_power_component(Comp_ID ComponentID)
{
    /* Check if there already exists the same component ID. */
    for(vector<Comp_ID>::iterator it = kitfox_power_component.begin();
        it != kitfox_power_component.end(); it++) {
        if((*it) == ComponentID) { return; }
    }

    kitfox_power_component.push_back(ComponentID);
}

/* Add KitFox components to calculate thermal. */
void kitfox_proxy_t::add_kitfox_thermal_component(Comp_ID ComponentID)
{
    /* Check if there already exists the same component ID. */
    for(vector<Comp_ID>::iterator it = kitfox_thermal_component.begin();
        it != kitfox_thermal_component.end(); it++) {
        if((*it) == ComponentID) { return; }
    }

    kitfox_thermal_component.push_back(ComponentID);
}

/* Add KitFox components to calculate reliability. */
void kitfox_proxy_t::add_kitfox_reliability_component(Comp_ID ComponentID)
{
    /* Check if there already exists the same component ID. */
    for(vector<Comp_ID>::iterator it = kitfox_reliability_component.begin();
        it != kitfox_reliability_component.end(); it++) {
        if((*it) == ComponentID) { return; }
    }

    kitfox_reliability_component.push_back(ComponentID);
}
