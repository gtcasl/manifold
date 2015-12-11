#include <iostream>
#include <stdlib.h>
#include "qsimproxy.h"

using namespace std;

int main(int argc, char** argv)
{
    if(argc != 6) {
        cerr << "Usage: " << argv[0] << " <server>  <server_port>  <shm filename> <shm size> <map filename>\n";
        exit(1);
    }

    QsimClient_Settings qc_settings(argv[1], atoi(argv[2]));
    Proxy_Para p_para;
    p_para.key_file_name = argv[3];
    p_para.buffer_size = atoi(argv[4]);
    p_para.map_file_name = argv[5];

    qsimproxy proxy(qc_settings, p_para);

    proxy.get_inst();
    
    return 0;
}
