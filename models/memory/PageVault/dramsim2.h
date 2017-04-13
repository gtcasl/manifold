#ifndef DRAMSIM2_H
#define DRAMSIM2_H

#include "kernel/manifold.h"
#include "kernel/component.h"
#include "utils.h"

namespace DRAMSim {
class MultiChannelMemorySystem;
}

namespace manifold {

struct dramsim2_cfg_t {
  string dev_file;
  string sys_file;
  uint32_t size;
};

class DRAMSim2 : public IMemoryController {
public:  
  
  DRAMSim2(const dramsim2_cfg_t& config, kernel::Clock *clock, ICallback* cb);
  ~DRAMSim2();
  
  int send_request(uint64_t addr, bool is_read);
  
  void print_stats(std::ostream& out);
  
private:
  
  struct stats_t {
    uint32_t n_reads;
    uint32_t n_writes;
    uint32_t n_reads_sent;
    uint64_t totalMemLat;
    
    stats_t() : n_reads(0)
              , n_writes(0)
              , n_reads_sent(0)
              , totalMemLat(0)
    {}
  };
  
  void tick();
  
  /* callbacks for read and write */
  void read_complete_cb(unsigned sysid, int tag, uint64_t cycle);
  void write_complete_cb(unsigned sysid, int tag, uint64_t cycle);
  
  DRAMSim::MultiChannelMemorySystem* m_mem; // the original DRAMSim object
  ICallback* m_cb;
  stats_t m_stats;
};

}

#endif // DRAMSIM2_H
