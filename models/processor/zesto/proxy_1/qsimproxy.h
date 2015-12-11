#ifndef MANIFOLD_ZESTO_QSIMPROXY
#define MANIFOLD_ZESTO_QSIMPROXY

//#include <sys/types.h>
//#include <sys/ipc.h>
//#include <sys/shm.h>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include "qsim-client.h"
#include "../shm.h"
#include "unistd.h"
#include <time.h>

struct QsimClient_Settings {
    QsimClient_Settings(const char* s, int p) : server(s), port(p) {}

    const char* server;
    int port;
};

struct Proxy_Para {
    char* key_file_name;
    long int buffer_size;
    char* map_file_name;
};

class qsimproxy
{
public:
    qsimproxy(const QsimClient_Settings& settings, Proxy_Para& proxy_setting);
    ~qsimproxy();

    void get_inst();

#ifndef PROXY_TEST
protected:
#endif
 
#ifndef PROXY_TEST
private:
#endif
    std::vector<Qsim::Client*> m_Qsim_client;
    std::vector<Qsim::ClientQueue*> m_Qsim_queue;
    std::vector<SharedMemBuffer*> shm_buffers;
    std::vector<int> cpu_id;
    long int inst_count;
    bool initialized;

};




#endif // MANIFOLD_ZESTO_QSIMCLIENT_CORE
