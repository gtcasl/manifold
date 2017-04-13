#ifndef HMCSIM_H
#define HMCSIM_H

#include "kernel/manifold.h"
#include "kernel/component.h"
#include "utils.h"

struct hmcsim_t;

namespace manifold {

struct hmc_cfg_t {
  int num_devs;
  int capacity;
  int block_size;
  int num_links;
  int num_vaults;
  int num_drams;
  int num_banks;
  int queue_depth;
  int xbar_depth;
  double vault_clock;
};

class HMCSim : public IMemoryController {
public:  
  
  HMCSim(const hmc_cfg_t& config, ICallback* cb);
  ~HMCSim();
  
  int send_request(uint64_t addr, bool is_read);
  
  void print_stats(std::ostream& out);
  
private:
  
  void tick();
  
  int send_packet(int cube, int link, uint64_t addr, bool is_read, uint32_t payload_size);
  
  int check_receive_packet(int cube, int link);  
  
  enum {
    HMC_PKT_TAG_BITS = 9,
  };
  
  hmcsim_t*   m_hmc;
  kernel::Clock* m_clock;
  uint32_t    m_dst_cube;
  uint32_t    m_dst_link;
  
  ICallback*  m_cb;
  unsigned    m_msg_tag;
  hmc_cfg_t   m_config;
  FILE*       m_logfile;
};

}

#endif // HMCSIM_H
