CXX = g++
QSIM_PREFIX = /usr/local
CPPFLAGS += -g -Wall -DUSE_QSIM -I/usr/include  -I/usr/local/include -I$(QSIM_PREFIX)/include
LDFLAGS += -lrt -L$(QSIM_PREFIX)/lib 
EXECS = proxy

VPATH= ../

ALL: $(EXECS)

proxy: proxy.o qsimproxy.o shm.o
	$(CXX) -o$@ $^ $(LDFLAGS) -lqsim-client -ldl

%.o: %.cc
	@[ -d dep ] || mkdir dep
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MF dep/$*.d -c $< -o $*.o

%.o: %.cpp
	@[ -d dep ] || mkdir dep
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MF dep/$*.d -c $< -o $*.o

-include $(wildcard dep/*.d)

.PHONY: clean
clean:
	rm -f $(EXECS) *.o
	rm -rf dep
