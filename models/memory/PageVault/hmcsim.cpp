#include "utils.h"
#include "hmcsim.h"
#include "hmc_sim.h"
using namespace std;
using namespace manifold::kernel;

namespace manifold {

HMCSim::HMCSim(const hmc_cfg_t& config, ICallback* cb) 
  : m_config(config)
  , m_dst_cube(0)
  , m_dst_link(0)
  , m_msg_tag(0)
  , m_cb(cb) {
  
  cb->add_ref();
  
  // initialize HMCSIM  
  m_hmc = new hmcsim_t();    
  int ret = hmcsim_init(
    m_hmc, 
    1, 
    config.num_links, 
    config.num_vaults, 
    config.queue_depth, 
    config.num_banks, 
    config.num_drams, 
    config.capacity, 
    config.xbar_depth);
	if (ret != 0) { 
		printf("*** Failed to initialize HMCSIM\n");
    abort();
	}
  
  // setup the device topology
  if (config.num_devs > 1) {
    //--
  } else {
    // single host to all links
    for (unsigned i = 0, n = config.num_links; i < n; ++i) {
      ret = hmcsim_link_config(m_hmc, 1, 0, i, i, HMC_LINK_HOST_DEV);
      if (ret != 0) {
        printf("*** Failed to initialize link %d\n", i);
        abort();
      }    
    }
  }
  
  // initialize max block size 
  ret = hmcsim_util_set_all_max_blocksize(m_hmc, config.block_size);
  if (ret != 0) {
    printf("*** Failed to initialize max block size\n");
    abort();
  }  
  
  // setup tracing
  m_logfile = std::tmpfile();
  if (m_logfile == nullptr) {
    printf("*** Failed to create trace file\n");
    abort();
  }
  hmcsim_trace_handle(m_hmc, m_logfile);
  hmcsim_trace_level(m_hmc, (HMC_TRACE_BANK | HMC_TRACE_QUEUE | HMC_TRACE_CMD |
                             HMC_TRACE_STALL | HMC_TRACE_LATENCY)); 
  
  m_clock = new kernel::Clock(config.vault_clock); 
  Clock::Register(*m_clock, this, &HMCSim::tick, (void(HMCSim::*)(void))0);
}

HMCSim::~HMCSim() {
  fclose(m_logfile);  
  hmcsim_free(m_hmc);
  delete m_hmc;
  m_cb->release();
  delete m_clock;
}

void HMCSim::tick() {   
  // update hmcsim clock
  hmcsim_clock(m_hmc);
  
  // process memory replies 
  int num_links = m_config.num_links;
  for (int link = 0; link < num_links; ++link) { 
    int tag = this->check_receive_packet(m_dst_cube, link);
    if (tag != -1) {
      // notify caller
      m_cb->onComplete(tag);
    }
  }  
}

int HMCSim::send_request(uint64_t addr, bool is_read) {  
  // ensure address range
  assert((addr + m_config.block_size) <= (static_cast<uint64_t>(m_config.capacity) << 30));
  
  // try to send the request
  int tag = this->send_packet(m_dst_cube, m_dst_link, addr, is_read, m_config.block_size);
  
  // cycle through all links
  if (++m_dst_link >= m_config.num_links) {
    m_dst_link = 0;
  }
  
  return tag;
}

int HMCSim::send_packet(int cube, int link, uint64_t addr, bool is_read, uint32_t payload_size) {
  int ret;  
  uint64_t packet[HMC_MAX_UQ_PACKET];
  uint64_t payload[8]; // 64 bytes
  uint64_t head, tail;
  
  assert(payload_size == sizeof(payload));
  
  const int tag = m_msg_tag;
  
  if (is_read) {
    ret = hmcsim_build_memrequest(m_hmc, cube, addr, tag, 
                                  RD64, link, &(payload[0]), &head, &tail);
    packet[0] = head;
    packet[1] = tail;
  } else {
    ret = hmcsim_build_memrequest(m_hmc, cube, addr, tag, 
                                  WR64, link, &(payload[0]), &head, &tail);
    packet[0] = head;
    packet[1] = 0x05;
    packet[2] = 0x06;
    packet[3] = 0x07;
    packet[4] = 0x08;
    packet[5] = 0x09;
    packet[6] = 0x0A;
    packet[7] = 0x0B;
    packet[8] = 0x0C;
    packet[9] = tail;
  }
  if (ret != HMC_OK) {
    printf("*** Failed to packet generation\n");
    abort();
  }
  
  // try sending packet
  ret = hmcsim_send(m_hmc, packet);
  
  switch (ret) {
  case HMC_OK:
    // packet identifer (wrap around on overflows)
    if (++m_msg_tag == (1 << HMC_PKT_TAG_BITS))
      m_msg_tag = 0;   
    // return the packet identifier
    return tag;
  case HMC_STALL:
    return -1;
  default:
    printf("*** Failed to send packet\n");
    abort();
  }
}

int HMCSim::check_receive_packet(int cube, int link) {
  int ret;
  uint64_t packet[HMC_MAX_UQ_PACKET];
  uint64_t d_response_head;
  uint64_t d_response_tail;
  hmc_response_t d_type;
  uint8_t d_length;
  uint16_t d_tag;
  uint8_t d_rtn_tag;
  uint8_t d_src_link;
  uint8_t d_rrp;
  uint8_t d_frp;
  uint8_t d_seq;
  uint8_t d_dinv;
  uint8_t d_errstat;
  uint8_t d_rtc;
  uint32_t d_crc;
  
  // check received packet
  ret = hmcsim_recv(m_hmc, cube, link, packet);
  
  switch (ret) {
  case HMC_OK:  
    // decode received packet
    hmcsim_decode_memresponse(
        m_hmc, packet, &d_response_head, &d_response_tail, &d_type,
        &d_length, &d_tag, &d_rtn_tag, &d_src_link, &d_rrp, &d_frp,
        &d_seq, &d_dinv, &d_errstat, &d_rtc, &d_crc); 
    // return packet identifier
    return d_tag;
  case HMC_STALL:
    return -1;
  default:
    printf("*** Failed to receive packet\n");
    abort();
  } 
}

void HMCSim::print_stats(std::ostream& out) {
  // only generate this log at level 3
  if (getDbgLevel() != 3)
    return;
    
  char buf[8];
  std::rewind(m_logfile);
  out << "*****   HMCSIM   *****" << endl;  
  while (std::fgets(buf, sizeof(buf), m_logfile) != nullptr) {
      out << buf;
  }
}

}
