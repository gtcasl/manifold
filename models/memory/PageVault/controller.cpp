#include "controller.h"
#include "../../cache/mcp-cache/coh_mem_req.h"
#include "utils.h"
#include "gmt.h"
#include "bmt.h"
#include "pmt.h"
#include "sgx.h"

using namespace std;
using namespace manifold::kernel;
using namespace manifold::uarch;

namespace manifold {
namespace pagevault {

static config_t g_config;

config_t* get_config() {
  return &g_config;
}

int ea_get_partition_size() {
  if (g_config.type == "PMT") {
    return g_config.pmt_size;
  } else {
    return 1;
  }
}

Controller::Controller(int nid, kernel::Clock *clock) 
  : m_nodeid(nid)
  , m_mc_map(nullptr) {
  
  // initialize router credits
  m_downstream_credits = g_config.downstream_credits;
  
  if (g_config.mc_type == "DRAMSim2") {
    // create DRAMSim2 component
    IMemoryController::ICallback* const cb = 
        new IMemoryController::TCallback<Controller>(this, &Controller::mc_request_complete_cb);
    m_mc = new DRAMSim2(g_config.dramsim2, clock, cb);
    cb->release();
  } else 
  if (g_config.mc_type == "HMCSim") {
    // create HMCSim component
    IMemoryController::ICallback* const cb = 
        new IMemoryController::TCallback<Controller>(this, &Controller::mc_request_complete_cb);
    m_mc = new HMCSim(g_config.hmc, cb);
    cb->release();
  } else {
    printf("ERROR: invalid MC type");
    abort();
  }
  
  // create the EACore component
  if (g_config.type == "GMT")
    m_eacore = new gmt::EACore(this);
  else if (g_config.type == "BMT")
    m_eacore = new bmt::EACore(this);
  else if (g_config.type == "PMT")
    m_eacore = new pmt::EACore(this);
  else if (g_config.type == "NOEA")
    m_eacore = nullptr;
  else {
    printf("ERROR: invalid EA type");
    abort();
  }
  
  // create the complete callback  
  m_req_complete_cb = new TReqCompleteCB<Controller>(this, &Controller::request_complete_cb);
    
  // register clock
  Clock::Register(*clock, this, &Controller::tick, (void(Controller::*)(void))0);  
}

Controller::~Controller() {
  delete m_mc;  
  if (m_eacore)
    delete m_eacore;
  m_req_complete_cb->release();
}

void Controller::set_mc_map(manifold::uarch::DestMap *m) {
  m_mc_map = m;
}

void Controller::tick() {  
  // process incoming requests
  if (!m_incoming_reqs.empty())   
    this->schedule_request();
  
  // process outgoing replies
  if (!m_outgoing_replies.empty()
   && m_downstream_credits > 0) 
    this->process_replies();
  
  // advance the EA unit
  if (m_eacore)
    m_eacore->tick();
    
  // deadlock check
  uint64_t cycle = m_clk->NowTicks();
  if (0 == (cycle % 10000))
    this->checkDeadlock(cycle);
}

void Controller::checkDeadlock(uint64_t cycle) {
  for (auto reqs : m_pending_reqs) {
    for (memreq_t* req : reqs.second) {
      uint64_t delay = cycle - req->cycle;
      if (delay > 80000) {
        printf("ERROR: Deadlock at cycle %ld for req#%d, elapsed %ld cycles\n", cycle, reqs.first, delay);
        abort();
      }
    }
  }
}

int Controller::submit_mem_request(uint64_t addr, bool is_read, EA_BLOCK_TYPE dt, int req, IMemCompleteCB* cb) {
  // check if the request is already pending
  auto iter = std::find_if(m_mc_pending_reqs.begin(), m_mc_pending_reqs.end(), 
      [addr](const MemPQ::value_type& v)->bool { return v.second->addr == addr; });
  if (iter != m_mc_pending_reqs.end()) {    
    DBG(3, "*** duplicate %s request for %s block 0x%lx (tag#%d) at cycle %ld (req#%d)\n",
        toRWString(is_read),
        toString(dt),
        addr,
        iter->second->tag,
        m_clk->NowTicks(),
        req);
    
    // if ignore duplicate reads 
    if (is_read && iter->second->is_read) 
      return iter->second->tag;
    
    // should be a read-after-write scenario
    // wait for the pending write to complete
    assert(is_read && !iter->second->is_read);
    return -1;
  }
  
  int tag = m_mc->send_request(addr, is_read);
  if (tag != -1) {  
    DBG(2, "MC: %s request for %s block 0x%lx (tag#%d) at cycle %ld (req#%d)\n", 
        toRWString(is_read), toString(dt), addr, tag, m_clk->NowTicks(), req);
    // add new request to pending queue
    llmemreq_t* mreq = new llmemreq_t(addr, is_read, dt, req, cb, tag, m_clk->NowTicks());
    m_mc_pending_reqs[tag] = mreq;
    
    // stats update
    if (mreq->is_read) {
      ++m_stats.mc_loads[dt];
    } else {
      ++m_stats.mc_stores[dt];
    }
  }  
  
  return tag;
}

void Controller::mc_request_complete_cb(int tag) {  
  //--  
  llmemreq_t* const mreq = m_mc_pending_reqs.at(tag);  
  DBG(2, "MC: %s response from memory for %s block 0x%lx (tag#%d) at cycle %ld (req#%d)\n", 
      toRWString(mreq->is_read), toString(mreq->dt), mreq->addr, mreq->tag, m_clk->NowTicks(), mreq->req);
  
  // stats update
  if (mreq->is_read) {
    m_stats.mc_read_latency += m_clk->NowTicks() - mreq->cycle;
    ++m_stats.mc_serviced_loads;
  } else {
    m_stats.mc_write_latency += m_clk->NowTicks() - mreq->cycle;
    ++m_stats.mc_serviced_stores;
  } 
   
  // process completed mc requests
  if (mreq->cb)
    mreq->cb->onComplete(mreq->tag);
  else
    this->request_complete_cb(mreq->tag, false);
  delete mreq;
  
  // remove from pending queue
  m_mc_pending_reqs.erase(tag);
}

void Controller::handle_request(int, uarch::NetworkPacket *pkt) {
  if (pkt->type == g_config.credit_msg_type) {
    // increment downstream credits 
    ++m_downstream_credits;
    assert(m_downstream_credits <= g_config.downstream_credits);
  } else {    
    // build memory request
    assert(pkt->type == g_config.mem_msg_type);
    assert(pkt->dst == m_nodeid);               
    auto* const msg = reinterpret_cast<mcp_cache_namespace::Mem_msg*>(pkt->data);  
    int64_t gaddr = msg->get_addr();
    int64_t laddr = m_mc_map->get_local_addr(gaddr);
    assert(laddr <= g_config.sys_mem); // ensure address in bound
    assert(0 == (laddr & EA_BLOCK_SIZE_MASK)); // ensure block-aligned address    
    memreq_t* const mreq = new memreq_t(
          laddr, 
          msg->is_read(),
          (msg->op_type == mcp_cache_namespace::OpEvict),
          (msg->op_type == mcp_cache_namespace::OpPrefetch) ? EA_PREFETCH_MASK : msg->onchip_mask,
          pkt->get_src(), 
          pkt->get_src_port(), 
          m_clk->NowTicks()
      ); 
    if (mreq->is_evict) {
      if (g_config.disable_evictions_handling) {
        // ignore eviction requests
        this->send_credit_downstream();
        // delete the packet
        delete pkt;
        return;
      } else {
        DBG(1, "received eviction request from node %d:%d for block 0x%lx at cycle %ld\n", 
           mreq->src_nodeid, mreq->src_port, mreq->addr, m_clk->NowTicks());
      }
    } else {
      DBG(1, "received %s request from node %d:%d for block 0x%lx at cycle %ld\n", 
         toRWString(mreq->is_read), mreq->src_nodeid, mreq->src_port, mreq->addr, m_clk->NowTicks());
    }
    // find read-after-write (RAW) duplicate
    auto iter = std::find_if(m_incoming_reqs.begin(), m_incoming_reqs.end(), 
        [mreq](const std::list<memreq_t*>::value_type& v)->bool { 
      return v->addr == mreq->addr; 
    });
    // add to input buffer    
    m_incoming_reqs.push_back(mreq);
  }
  // delete the packet
  delete pkt;
}

void Controller::schedule_request() {     
  int tag;  
  assert(!m_incoming_reqs.empty());
  memreq_t* const mreq = m_incoming_reqs.front();
  
  // submit the request, ensure RAW dependency    
  if (m_eacore) {
    tag = m_eacore->submit_request(mreq->addr, 
                                   mreq->is_read, 
                                   mreq->onchip_mask,
                                   mreq->is_evict ? EA_EVICT_BLOCK : EA_USER_BLOCK,     
                                   mreq->src_nodeid,
                                   nullptr, 
                                   m_req_complete_cb);
  } else { 
    tag = this->submit_mem_request(mreq->addr, mreq->is_read, EA_USER_BLOCK, 0, nullptr);  
  }
  if (tag != -1 
   || mreq->is_prefetch()) {  
    // stats update
    if (mreq->is_read) {
      ++m_stats.loads[mreq->src_nodeid];
    } else {
      ++m_stats.stores[mreq->src_nodeid];
    }
    
    // notify remote link to update it network credit
    // A network credit update should be sent on the next clock tick after receiving the request.
    this->send_credit_downstream();
    
    // remove input entry
    m_incoming_reqs.pop_front();
    
    // add to list    
    m_pending_reqs[tag].push_back(mreq);  
    
    if (tag == -1) {
      // add requests to reply queue
      assert(mreq->is_prefetch());
      this->request_complete_cb(-1, false);
    }
  }
}

void Controller::request_complete_cb(int tag, bool is_speculative) {
  if (is_speculative)
    return;
  
  // process all requests associated with the completed tag 
  for (memreq_t* mreq : m_pending_reqs.at(tag)) {
    if (mreq->is_evict) {
      DBG(1, "completed eviction request for block 0x%lx at cycle %ld\n", 
          mreq->addr, m_clk->NowTicks());
    } else {
      DBG(1, "completed %s request for block 0x%lx at cycle %ld\n", 
          toRWString(mreq->is_read), mreq->addr, m_clk->NowTicks());
    }
    
    // update statistics
    if (mreq->is_read) {
      m_stats.read_latency += m_clk->NowTicks() - mreq->cycle;
      ++m_stats.serviced_loads;
    } else {
      m_stats.write_latency += m_clk->NowTicks() - mreq->cycle;
      ++m_stats.serviced_stores;
    }
    
    if (mreq->is_read) {    
      // add requests to reply queue
      m_outgoing_replies.push_back(mreq);
    } else { 
      // delete request
      delete mreq;
    }
  }
  
  // release from queue
  m_pending_reqs.erase(tag);
}

void Controller::process_replies() {
  assert(m_outgoing_replies.size() > 0 && m_downstream_credits > 0);  
  memreq_t* const mreq = m_outgoing_replies.front();
  assert(mreq->is_read);
  
  // convert back to global address
  int64_t gaddr = m_mc_map->get_local_addr(mreq->addr);
  
  // build reply packet
  manifold::uarch::NetworkPacket* const pkt =
          new manifold::uarch::NetworkPacket(g_config.mem_msg_type);
  pkt->set_src(m_nodeid);
  pkt->set_src_port(PORT0);
  pkt->set_dst(mreq->src_nodeid);
  pkt->set_dst_port(mreq->src_port); 
  
  pkt->data_size = sizeof(mcp_cache_namespace::Mem_msg);  
  auto* const msg = reinterpret_cast<mcp_cache_namespace::Mem_msg*>(pkt->data);  
  msg->set_mem_response();
  msg->set_addr(gaddr);
  msg->set_type(mreq->is_prefetch() ? mcp_cache_namespace::OpPrefetch : mcp_cache_namespace::OpMemLd);
  msg->set_dst(mreq->src_nodeid);
  msg->set_dst_port(mreq->src_port);  
  msg->set_src(m_nodeid);
  msg->set_src_port(PORT0);
  
  // send reply packet
  Send(PORT0, pkt);   
  
  // update downstream credits
  --m_downstream_credits;
  
  // delete request
  delete mreq;  
  
  // release from queue
  m_outgoing_replies.pop_front();
}

void Controller::send_credit_downstream() {
  // notify remote link to increment its downstream credits
  manifold::uarch::NetworkPacket* const pkt 
      = new manifold::uarch::NetworkPacket(g_config.credit_msg_type);
  this->Send(PORT0, pkt);
}

void Controller::print_config(std::ostream& out) {
  //--
}
  
void Controller::print_stats(std::ostream& out) {
  
  m_mc->print_stats(out);
  
  if (m_eacore)
    m_eacore->print_stats(out);
  
  out << "*****   PageVault   *****" << endl;
  
  out << "  Reads per source:" << endl;
  uint64_t total_loads = 0;
  for (auto stat : m_stats.loads) {
    out << "    " << stat.first << ": " << stat.second << endl;
    total_loads += stat.second;
  }
  
  out << "  Writes per source:" << endl;
  uint64_t total_stores = 0;
  for (auto stat : m_stats.stores) {
    out << "    " << stat.first << ": " << stat.second << endl;
    total_stores += stat.second;
  }
  
  out << "  Total Reads Requests = " << total_loads << endl;
  out << "  Total Writes Requests = " << total_stores << endl;
  
  uint64_t submited_requests = total_loads + total_stores;
  uint64_t serviced_requests = m_stats.serviced_loads + m_stats.serviced_stores;
  out << "  Total requests serviced = " << (serviced_requests * 100.0) / submited_requests << "%" << endl;
  
  out << "  Avg Read Latency = "
      << (double)m_stats.read_latency / m_stats.serviced_loads << " cycles" << endl;
  
  out << "  Avg Write Latency = "
      << (double)m_stats.write_latency / m_stats.serviced_stores << " cycles" << endl;
  
  uint64_t total_mc_loads = 0;
  for (auto stat : m_stats.mc_loads) {
    out << "  MC " << toString(stat.first) << " Read Requests = " << stat.second << endl;
    total_mc_loads += stat.second;
  }
  
  uint64_t total_mc_stores = 0;
  for (auto stat : m_stats.mc_stores) {
    out << "  MC " << toString(stat.first) << " Write Requests = " << stat.second << endl;
    total_mc_stores += stat.second;
  }
  
  out << "  Total MC Read Requests = " << total_mc_loads << endl;
  out << "  Total MC Write Requests = " << total_mc_stores << endl;
  
  uint64_t mc_submited_requests = total_mc_loads + total_mc_stores;
  uint64_t mc_serviced_requests = m_stats.mc_serviced_loads + m_stats.mc_serviced_stores;
  out << "  Total MC requests serviced = " << (mc_serviced_requests * 100.0) / mc_submited_requests << "%" << endl;
  
  out << "  Avg MC Read Latency = "
      << (double)m_stats.mc_read_latency / m_stats.mc_serviced_loads << " cycles" << endl;
  
  out << "  Avg MC Write Latency = "
      << (double)m_stats.mc_write_latency / m_stats.mc_serviced_stores << " cycles" << endl;

  out << "  MC Usefull Traffic = "
      << (serviced_requests * 100.0) / mc_serviced_requests << "%" << endl; 
}

} // namespace pagevault
} // namespace manifold
