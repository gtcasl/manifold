CXX = mpic++
QSIM_ROOT = ${QSIM_PREFIX}
MODELS_DIR = ../../../models
CPPFLAGS += -Wall -g -DUSE_QSIM -DSTATS -I$(QSIM_ROOT)/include -I ../../.. -I$(MODELS_DIR)/processor -I$(MODELS_DIR)/qsim -I$(MODELS_DIR)/cache -I$(MODELS_DIR)/network -I$(MODELS_DIR)/memory -I$(MODELS_DIR)/cross -std=c++11
LDFLAGS = -lqsim -L$(QSIM_ROOT)/lib -lmcp-iris -L$(MODELS_DIR)/cross/mcp_cache-iris -liris -L$(MODELS_DIR)/network/iris -lmcp-cache -L$(MODELS_DIR)/cache/mcp-cache -lspx -L$(MODELS_DIR)/processor/spx -lqsim_proxy -L$(MODELS_DIR)/qsim/proxy -lqsim_interrupt_handler -L$(MODELS_DIR)/qsim/interrupt_handler -lkitfox_proxy -L$(MODELS_DIR)/kitfox/proxy -lpagevault -L$(MODELS_DIR)/memory/PageVault -lcaffdram -L$(MODELS_DIR)/memory/CaffDRAM -lDRAMSim2 -L$(MODELS_DIR)/memory/DRAMSim2 -lDRAMSim2proper -L$(MODELS_DIR)/memory/DRAMSim2/DRAMSim2-2.2.2  -L../../../kernel -lmanifold -ldl -lrt

VPATH = ../common

EXECS = smp_llp smp_l1l2


ALL: $(EXECS)


smp_llp: smp_llp.o sysBuilder_llp.o sysBuilder_l1l2.o proc_builder.o cache_builder.o mc_builder.o network_builder.o qsim_builder.o
	$(CXX) $^ -o$@  $(LDFLAGS) -lconfig++

smp_l1l2: smp_l1l2.o sysBuilder_llp.o sysBuilder_l1l2.o proc_builder.o cache_builder.o mc_builder.o network_builder.o qsim_builder.o
	$(CXX) $^ -o$@  $(LDFLAGS) -lconfig++


%.o: %.cc
	@[ -d dep ] || mkdir -p dep
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MF dep/$*.d -c $< -o $*.o

-include $(wildcard dep/*.d)

.PHONY: clean
clean:
	rm -f $(EXECS) *.d *.o DBG* core*.out
	rm -rf dep
