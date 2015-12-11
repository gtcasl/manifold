#ifndef SYSBUILDER_L1L2_H
#define SYSBUILDER_L1L2_H

#include "sysBuilder_llp.h"


class SysBuilder_l1l2 : public SysBuilder_llp {
public:
    SysBuilder_l1l2(const char* fn) : SysBuilder_llp(fn) {}

    std::map<int, int>& get_l2_id_lp_map() { return l2_id_lp_map; }

protected:
    void config_components(); //overwrite baseclass
    //void create_nodes(int type, int n_lps, int part); //overwrite baseclass

    void do_partitioning_1_part(int n_lps); //overwrite baseclass
    void do_partitioning_y_part(int n_lps); //overwrite baseclass

private:
    std::vector<int> l2_node_idx_vec;
    std::set<int> l2_node_idx_set; //set is used to ensure each index is unique

    std::map<int, int> l2_id_lp_map; //maps proc's node id to its LP
};




#endif // #ifndef SYSBUILDER_L1L2_H
