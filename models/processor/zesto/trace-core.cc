#include "trace-core.h"
#include "zesto-opts.h"
#include "zesto-fetch.h"
#include "zesto-decode.h"
#include "zesto-alloc.h"
#include "zesto-exec.h"
#include "zesto-commit.h"
#include "zesto-bpred.h"

#include "kernel/manifold-decl.h"

#include <sstream>

namespace manifold {
namespace zesto {

trace_core_t::trace_core_t(const int core_id, char * config, const char * trace) : core_t(core_id, config)
{
  char buf[16];
  sim.trace = trace;

  current_thread->pin_trace = fopen(sim.trace,"r");

  if(!current_thread->pin_trace)
    fatal("could not open PIN trace file %s",sim.trace);

  use_stored_nextPC = false;

  md_addr_t nextPC = 0;
  fetch_next_pc(&nextPC,this);

  current_thread->regs.regs_PC = nextPC;
  current_thread->regs.regs_NPC = nextPC;
  fetch->PC = current_thread->regs.regs_PC;
  store_nextPC = nextPC;
}



bool trace_core_t::fetch_next_pc(md_addr_t *nextPC, struct core_t * core)
{	
  trace_core_t* tcore = dynamic_cast<trace_core_t*>(core);
  assert(tcore != 0);

  if(tcore->use_stored_nextPC == true)
    *nextPC = tcore->store_nextPC;
  else
  {
    if(!feof(tcore->current_thread->pin_trace))
    {
      fscanf(tcore->current_thread->pin_trace,"%llx", nextPC);
      tcore->store_nextPC = *nextPC;
    }
    else
    {
      if(tcore->current_thread->active)
      {
        fclose(tcore->current_thread->pin_trace);
        //fprintf(stderr,"\n# End of PIN trace reached for core%d\n",tcore->id);
      fprintf(stderr,"\n# End0 of PIN trace reached for core%d, tick= %ld\n",tcore->id, tcore->get_clock()->NowTicks());
        tcore->store_nextPC = *nextPC = NULL;
        tcore->current_thread->active = false;
        tcore->fetch->bpred->freeze_stats();
        tcore->exec->freeze_stats();
        //tcore->print_stats();
        manifold::kernel::Manifold :: Terminate();
      }
    }
  }
  return false;
}	


void trace_core_t::fetch_inst(md_inst_t *inst, struct mem_t *mem, const md_addr_t pc, core_t * const core)
{
  unsigned  int i,len,memops;
  trace_core_t* tcore = dynamic_cast<trace_core_t*>(core);
  assert(tcore != 0);

  fscanf(tcore->current_thread->pin_trace,"%d ",&len);
  fscanf(tcore->current_thread->pin_trace,"%d ",&memops);

  inst->vaddr=pc;//store_nextPC[core_id];	
  inst->paddr=pc;//store_nextPC[core_id];
  inst->qemu_len=len;

#ifdef ZDEBUG
  fprintf(stdout,"\n[%lld][Core%d]Trace DequeuedPC: 0x%llx   ",core->sim_cycle,core->id,inst->vaddr);
#endif

  for (i = 0; i < len; i++) 
  {
    if(feof(tcore->current_thread->pin_trace))
    {
      fclose(tcore->current_thread->pin_trace);
      //fprintf(stderr,"\n# End of PIN trace reached for core%d\n",tcore->id);
      fprintf(stderr,"\n# End1 of PIN trace reached for core%d, tick= %ld\n",tcore->id, tcore->get_clock()->NowTicks());
      tcore->store_nextPC = NULL;
      tcore->current_thread->active = false;
      tcore->fetch->bpred->freeze_stats();
      tcore->exec->freeze_stats();
      //tcore->print_stats();
      manifold::kernel::Manifold :: Terminate();
      return;
    }
    fscanf(tcore->current_thread->pin_trace,"%x ",&(inst->code[i]));
#ifdef ZDEBUG
    fprintf(stdout," %x", inst->code[i]);
#endif
  }

  inst->mem_ops.mem_vaddr_ld[0]=0;
  inst->mem_ops.mem_vaddr_ld[1]=0;
  inst->mem_ops.mem_vaddr_str[0]=0;
  inst->mem_ops.mem_vaddr_str[1]=0;
  if(memops)
  {
    unsigned long long ch;
    inst->mem_ops.memops=memops;
    fscanf(tcore->current_thread->pin_trace," %llx ",&ch);

    while(true)
    {
      if(feof(tcore->current_thread->pin_trace))
      {
        fclose(tcore->current_thread->pin_trace);
        //fprintf(stderr,"\n# End of PIN trace reached for core%d\n",tcore->id);
        fprintf(stderr,"\n# End2 of PIN trace reached for core%d, tick= %ld\n",tcore->id, tcore->get_clock()->NowTicks());
        tcore->store_nextPC = NULL;
        tcore->current_thread->active = false;
        tcore->fetch->bpred->freeze_stats();
        tcore->exec->freeze_stats();
        //tcore->print_stats();
        manifold::kernel::Manifold :: Terminate();
	return;
      }
      if(ch==0)
      {
        if(inst->mem_ops.mem_vaddr_ld[0]==0)
        {
          fscanf(tcore->current_thread->pin_trace,"%llx %d",&(inst->mem_ops.mem_vaddr_ld[0]), &(inst->mem_ops.ld_size[0]) );
          inst->mem_ops.mem_paddr_ld[0]= inst->mem_ops.mem_vaddr_ld[0];
          inst->mem_ops.ld_dequeued[0]=false;
          inst->mem_ops.memops++;
        }
        else
        {
          fscanf(tcore->current_thread->pin_trace,"%llx %d",&(inst->mem_ops.mem_vaddr_ld[1]), &(inst->mem_ops.ld_size[1]) );
          inst->mem_ops.mem_paddr_ld[1]= inst->mem_ops.mem_vaddr_ld[1];
          inst->mem_ops.ld_dequeued[1]=false;
          inst->mem_ops.memops++;
        }
      }
      else if (ch == 1)
      {
        if(inst->mem_ops.mem_vaddr_str[0] == 0)
        {
          fscanf(tcore->current_thread->pin_trace,"%llx %d",&(inst->mem_ops.mem_vaddr_str[0]), &(inst->mem_ops.str_size[0]) );
          inst->mem_ops.mem_paddr_str[0]= inst->mem_ops.mem_vaddr_str[0];
          inst->mem_ops.str_dequeued[0]=false;
          inst->mem_ops.memops++;	
        }
        else
        {
          fscanf(tcore->current_thread->pin_trace,"%llx %d",&(inst->mem_ops.mem_vaddr_str[1]), &(inst->mem_ops.str_size[1]) );
          inst->mem_ops.mem_paddr_str[1]= inst->mem_ops.mem_vaddr_str[1];
          inst->mem_ops.str_dequeued[1]=false;
          inst->mem_ops.memops++;
        }
      }
      else
      {
        tcore->store_nextPC = ch;
        tcore->use_stored_nextPC = true;
        break;
      }
      fscanf(tcore->current_thread->pin_trace," %llx ",&ch);
    }
  }
  else
    tcore->use_stored_nextPC = false;
}




void trace_core_t::emergency_recovery(void)
{
  struct Mop_t Mop_v;
  struct Mop_t * Mop = & Mop_v;

  unsigned int insn_count = stat.eio_commit_insn;

  if((stat.eio_commit_insn == last_emergency_recovery_count)&&(num_emergency_recoveries > 0))
    fatal("After previous attempted recovery, thread %d is still getting wedged... giving up.",current_thread->id);

  memset(Mop,0,sizeof(*Mop));
  fprintf(stderr, "### Emergency recovery for thread %d, resetting to inst-count: %lld\n", current_thread->id,stat.eio_commit_insn);

  /* reset core state */
  stat.eio_commit_insn = 0;
  oracle->complete_flush();
  commit->recover();
  exec->recover();
  alloc->recover();
  decode->recover();
  fetch->recover(/*new PC*/0);

  use_stored_nextPC = false;  //this line is the only difference from the base class version

  md_addr_t nextPC;
  
  fetch_next_pc(&nextPC, this);
  
  current_thread->regs.regs_PC=nextPC;
  current_thread->regs.regs_NPC=nextPC;
  fetch->PC = current_thread->regs.regs_PC;
  fetch->bogus = false;
  num_emergency_recoveries++;
  last_emergency_recovery_count = stat.eio_commit_insn;

  /* reset stats */
  fetch->reset_bpred_stats();

  stat.eio_commit_insn = insn_count;

  oracle->hosed = false;
}




//####################################################################
// loop_trace_core_t
//####################################################################
loop_trace_core_t :: loop_trace_core_t(const int core_id, char * config, const char * trace) : trace_core_t(core_id, config, trace),
    tracefile_name(trace)
{
    //do nothing
}



bool loop_trace_core_t :: fetch_next_pc(md_addr_t *nextPC, struct core_t * core)
{	
    loop_trace_core_t* tcore = dynamic_cast<loop_trace_core_t*>(core);
    assert(tcore != 0);

    if(tcore->use_stored_nextPC == true) {
	*nextPC = tcore->store_nextPC;
    }
    else {
	if(feof(tcore->current_thread->pin_trace)) { //if EOF reached
cout << " core " << id << " reached eof\n";
	    fclose(tcore->current_thread->pin_trace);

	    tcore->current_thread->pin_trace = fopen(tracefile_name, "r"); //reopen
	    if(!current_thread->pin_trace)
		fatal("could not open PIN trace file %s", tracefile_name);
	}

        fscanf(tcore->current_thread->pin_trace,"%llx", nextPC);
        tcore->store_nextPC = *nextPC;
    }

    return false;
}	


void loop_trace_core_t :: fetch_inst(md_inst_t *inst, struct mem_t *mem, const md_addr_t pc, core_t * const core)
{
  unsigned  int i,len,memops;
  loop_trace_core_t* tcore = dynamic_cast<loop_trace_core_t*>(core);
  assert(tcore != 0);

  fscanf(tcore->current_thread->pin_trace,"%d ",&len);
  fscanf(tcore->current_thread->pin_trace,"%d ",&memops);

  inst->vaddr=pc;//store_nextPC[core_id];	
  inst->paddr=pc;//store_nextPC[core_id];
  inst->qemu_len=len;

#ifdef ZDEBUG
  fprintf(stdout,"\n[%lld][Core%d]Trace DequeuedPC: 0x%llx   ",core->sim_cycle,core->id,inst->vaddr);
#endif

  for (i = 0; i < len; i++) 
  {
    if(feof(tcore->current_thread->pin_trace))
    {
      fclose(tcore->current_thread->pin_trace);

      fatal("End of trace file reached while reading instruction byte code");
    }
    fscanf(tcore->current_thread->pin_trace,"%x ",&(inst->code[i]));
#ifdef ZDEBUG
    fprintf(stdout," %x", inst->code[i]);
#endif
  }

  inst->mem_ops.mem_vaddr_ld[0]=0;
  inst->mem_ops.mem_vaddr_ld[1]=0;
  inst->mem_ops.mem_vaddr_str[0]=0;
  inst->mem_ops.mem_vaddr_str[1]=0;
  if(memops)
  {
    unsigned long long ch;
    inst->mem_ops.memops=memops;
    fscanf(tcore->current_thread->pin_trace," %llx ",&ch);

    int count = 0; //count the number of mem ops

    while(true)
    {
      if(feof(tcore->current_thread->pin_trace)) {
          assert(0); //should not happen: last instruction is a jump
      }

      if(ch==0) {
        count++;
        if(inst->mem_ops.mem_vaddr_ld[0]==0) {
          fscanf(tcore->current_thread->pin_trace,"%llx %d",&(inst->mem_ops.mem_vaddr_ld[0]), &(inst->mem_ops.ld_size[0]) );
          inst->mem_ops.mem_paddr_ld[0]= inst->mem_ops.mem_vaddr_ld[0];
          inst->mem_ops.ld_dequeued[0]=false;
          inst->mem_ops.memops++;
        }
        else {
          fscanf(tcore->current_thread->pin_trace,"%llx %d",&(inst->mem_ops.mem_vaddr_ld[1]), &(inst->mem_ops.ld_size[1]) );
          inst->mem_ops.mem_paddr_ld[1]= inst->mem_ops.mem_vaddr_ld[1];
          inst->mem_ops.ld_dequeued[1]=false;
          inst->mem_ops.memops++;
        }
      }
      else if (ch == 1) {
        count++;
        if(inst->mem_ops.mem_vaddr_str[0] == 0) {
          fscanf(tcore->current_thread->pin_trace,"%llx %d",&(inst->mem_ops.mem_vaddr_str[0]), &(inst->mem_ops.str_size[0]) );
          inst->mem_ops.mem_paddr_str[0]= inst->mem_ops.mem_vaddr_str[0];
          inst->mem_ops.str_dequeued[0]=false;
          inst->mem_ops.memops++;	
        }
        else {
          fscanf(tcore->current_thread->pin_trace,"%llx %d",&(inst->mem_ops.mem_vaddr_str[1]), &(inst->mem_ops.str_size[1]) );
          inst->mem_ops.mem_paddr_str[1]= inst->mem_ops.mem_vaddr_str[1];
          inst->mem_ops.str_dequeued[1]=false;
          inst->mem_ops.memops++;
        }
      }
      else {
        tcore->store_nextPC = ch;
        tcore->use_stored_nextPC = true;
        break;
      }
      fscanf(tcore->current_thread->pin_trace," %llx ",&ch);
    }//while
  }
  else //if instruction has no mem op, to get next pc requires a read
    tcore->use_stored_nextPC = false;
}






//####################################################################
// mf_trace_core_t
// Each core has multiple trace files.
//####################################################################
mf_trace_core_t :: mf_trace_core_t(const int core_id, char * config, const char * trace) :
    trace_core_t(core_id, config, (string(trace)+"_0").c_str()), tracefile_base_name(trace), tf_idx(0)
{
cout << "id= " << id << " trace= " << trace << "  basename= " << tracefile_base_name << endl;
cout.flush();
    //do nothing
}



bool mf_trace_core_t :: fetch_next_pc(md_addr_t *nextPC, struct core_t * core)
{	
    mf_trace_core_t* tcore = dynamic_cast<mf_trace_core_t*>(core);
    assert(tcore != 0);

    if(tcore->use_stored_nextPC == true) {
	*nextPC = tcore->store_nextPC;
    }
    else {
	if(!feof(tcore->current_thread->pin_trace)) {
	    fscanf(tcore->current_thread->pin_trace,"%llx", nextPC);
	    tcore->store_nextPC = *nextPC;
        }
	else { //reached end of trace file
	    if(tcore->current_thread->active) {
		if(handle_eof()) {
		    fscanf(tcore->current_thread->pin_trace,"%llx", nextPC);
		    tcore->store_nextPC = *nextPC;
		}
		else {
		    terminate();
		}
	    }
	}
    }
    return false;
}	


void mf_trace_core_t::fetch_inst(md_inst_t *inst, struct mem_t *mem, const md_addr_t pc, core_t * const core)
{
  unsigned  int i,len,memops;
  mf_trace_core_t* tcore = dynamic_cast<mf_trace_core_t*>(core);
  assert(tcore != 0);

  fscanf(tcore->current_thread->pin_trace,"%d ",&len);
  fscanf(tcore->current_thread->pin_trace,"%d ",&memops);

  inst->vaddr=pc;//store_nextPC[core_id];	
  inst->paddr=pc;//store_nextPC[core_id];
  inst->qemu_len=len;

#ifdef ZDEBUG
  fprintf(stdout,"\n[%lld][Core%d]Trace DequeuedPC: 0x%llx   ",core->sim_cycle,core->id,inst->vaddr);
#endif

  for (i = 0; i < len; i++) 
  {
    if(feof(tcore->current_thread->pin_trace))
    {
      fclose(tcore->current_thread->pin_trace);

      fatal("End of trace file reached while reading instruction byte code");
      return;
    }
    fscanf(tcore->current_thread->pin_trace,"%x ",&(inst->code[i]));
#ifdef ZDEBUG
    fprintf(stdout," %x", inst->code[i]);
#endif
  }

  inst->mem_ops.mem_vaddr_ld[0]=0;
  inst->mem_ops.mem_vaddr_ld[1]=0;
  inst->mem_ops.mem_vaddr_str[0]=0;
  inst->mem_ops.mem_vaddr_str[1]=0;
  if(memops)
  {
    unsigned long long ch;
    inst->mem_ops.memops=memops;
    fscanf(tcore->current_thread->pin_trace," %llx ",&ch);

    while(true)
    {
      if(feof(tcore->current_thread->pin_trace))
      {
        if(handle_eof()) {
	    fscanf(tcore->current_thread->pin_trace," %llx ",&ch);
	    continue;
	}
	else {
	    terminate();
	    return;
	}
      }
      if(ch==0)
      {
        if(inst->mem_ops.mem_vaddr_ld[0]==0)
        {
          fscanf(tcore->current_thread->pin_trace,"%llx %d",&(inst->mem_ops.mem_vaddr_ld[0]), &(inst->mem_ops.ld_size[0]) );
          inst->mem_ops.mem_paddr_ld[0]= inst->mem_ops.mem_vaddr_ld[0];
          inst->mem_ops.ld_dequeued[0]=false;
          inst->mem_ops.memops++;
        }
        else
        {
          fscanf(tcore->current_thread->pin_trace,"%llx %d",&(inst->mem_ops.mem_vaddr_ld[1]), &(inst->mem_ops.ld_size[1]) );
          inst->mem_ops.mem_paddr_ld[1]= inst->mem_ops.mem_vaddr_ld[1];
          inst->mem_ops.ld_dequeued[1]=false;
          inst->mem_ops.memops++;
        }
      }
      else if (ch == 1)
      {
        if(inst->mem_ops.mem_vaddr_str[0] == 0)
        {
          fscanf(tcore->current_thread->pin_trace,"%llx %d",&(inst->mem_ops.mem_vaddr_str[0]), &(inst->mem_ops.str_size[0]) );
          inst->mem_ops.mem_paddr_str[0]= inst->mem_ops.mem_vaddr_str[0];
          inst->mem_ops.str_dequeued[0]=false;
          inst->mem_ops.memops++;	
        }
        else
        {
          fscanf(tcore->current_thread->pin_trace,"%llx %d",&(inst->mem_ops.mem_vaddr_str[1]), &(inst->mem_ops.str_size[1]) );
          inst->mem_ops.mem_paddr_str[1]= inst->mem_ops.mem_vaddr_str[1];
          inst->mem_ops.str_dequeued[1]=false;
          inst->mem_ops.memops++;
        }
      }
      else
      {
        tcore->store_nextPC = ch;
        tcore->use_stored_nextPC = true;
        break;
      }
      fscanf(tcore->current_thread->pin_trace," %llx ",&ch);
    }
  }
  else
    tcore->use_stored_nextPC = false;
}



//! @return true if we can continue with next trace file; false if we have reached eof of the last file
bool mf_trace_core_t :: handle_eof()
{
cout << "core " << id << " eof\n";
    fclose(current_thread->pin_trace);

    //open the next file
    stringstream ss;
    ss << "_" << tf_idx;
    string current_tf = tracefile_base_name + ss.str();

    stringstream ss1;
    tf_idx++;
    ss1 << "_" << tf_idx;
    string next_tf = tracefile_base_name + ss1.str();


    //compress current file
    system((string("gzip ") + current_tf).c_str());

    //uncompress next file
    system((string("gunzip ") + next_tf + ".gz").c_str());


    current_thread->pin_trace = fopen(next_tf.c_str(),"r");
    if(!current_thread->pin_trace)
    cerr << "error open " << next_tf.c_str() << "\n";

    return (current_thread->pin_trace != 0);

}


void mf_trace_core_t :: terminate()
{
    fprintf(stderr,"\n# End of PIN trace reached for core%d, tick= %ld\n",id, get_clock()->NowTicks());
    store_nextPC = NULL;
    current_thread->active = false;
    fetch->bpred->freeze_stats();
    exec->freeze_stats();
    manifold::kernel::Manifold :: Terminate();
}


} // namespace zesto
} // namespace manifold

