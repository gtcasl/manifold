DRAMSim2properlib_LIBRARIES = libDRAMSim2proper.a
DRAMSim2properlibdir = $(libdir)/manifold

libDRAMSim2proper_a_SOURCES = \
        AddressMapping.cpp \
	AddressMapping.h \
	Bank.cpp \
	Bank.h \
	BankState.cpp \
	BankState.h \
	BusPacket.cpp \
	BusPacket.h \
	ClockDomain.cpp \
	ClockDomain.h \
	CommandQueue.cpp \
	CommandQueue.h \
	CSVWriter.h \
	IniReader.cpp \
	IniReader.h \
	MemoryController.cpp \
	MemoryController.h \
	MemorySystem.cpp \
	MemorySystem.h \
	MultiChannelMemorySystem.cpp \
	MultiChannelMemorySystem.h \
	Rank.cpp \
	Rank.h \
	SimulatorObject.cpp \
	SystemConfiguration.h \
	Transaction.cpp \
	Transaction.h

pkginclude_DRAMSim2properdir = $(includedir)/manifold/DRAMSim2

pkginclude_DRAMSim2proper_HEADERS = \
	AddressMapping.h \
	Bank.h \
	BankState.h \
	BusPacket.h \
	Callback.h \
	ClockDomain.h \
	CommandQueue.h \
	CSVWriter.h \
	IniReader.h \
	MemoryController.h \
	MemorySystem.h \
	MultiChannelMemorySystem.h \
	PrintMacros.h \
	Rank.h \
	SimulatorObject.h \
	SystemConfiguration.h \
	Transaction.h

libDRAMSim2proper_a_CPPFLAGS = -DNO_STORAGE -Wall -DDEBUG_BUILD -DLOG_OUTPUT
