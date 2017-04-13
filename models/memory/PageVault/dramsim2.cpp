#include "dramsim2.h"

#include "../DRAMSim2/DRAMSim2-2.2.2/SystemConfiguration.h"
#include "../DRAMSim2/DRAMSim2-2.2.2/MultiChannelMemorySystem.h"
#include "../DRAMSim2/DRAMSim2-2.2.2/Transaction.h"

using namespace std;
using namespace manifold::kernel;

namespace manifold {

DRAMSim2::DRAMSim2(const dramsim2_cfg_t& config, kernel::Clock *clock, ICallback* cb) 
  : m_cb(cb) {  
  cb->add_ref();
  
  // instantiate the DRAMSim module
  m_mem = new DRAMSim::MultiChannelMemorySystem(config.dev_file,
                                                config.sys_file, ".", "res",
                                                config.size);

  // create and register DRAMSim callback functions
  DRAMSim::ITransactionCompleteCB* const read_cb = 
      new DRAMSim::TransactionCompleteCB<DRAMSim2>(this, &DRAMSim2::read_complete_cb);
  DRAMSim::ITransactionCompleteCB* const write_cb = 
      new DRAMSim::TransactionCompleteCB<DRAMSim2>(this, &DRAMSim2::write_complete_cb);
  m_mem->RegisterCallbacks(read_cb, write_cb, NULL);
  m_mem->setCPUClockSpeed(clock->freq);
  
  // register with clock
  Clock::Register(*clock, this, &DRAMSim2::tick, (void (DRAMSim2::*)(void))0);
}

DRAMSim2::~DRAMSim2() {
  delete m_mem;
  m_cb->release();
}

void DRAMSim2::tick() {
  m_mem->update();
}

int DRAMSim2::send_request(uint64_t addr, bool is_read) {
  if (m_mem->willAcceptTransaction()) {     
    int tag = m_mem->addTransaction(addr, !is_read);
    return tag;
  }
  return -1;
}

void DRAMSim2::read_complete_cb(unsigned sysid, int tag, uint64_t cycle) {
  m_cb->onComplete(tag);
}

void DRAMSim2::write_complete_cb(unsigned sysid, int tag, uint64_t cycle) {
  m_cb->onComplete(tag);
}

void DRAMSim2::print_stats(std::ostream& out) {
  //--
}

}
