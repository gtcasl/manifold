#ifdef USE_QSIM

#include "qsimproxy.h"
#include <assert.h>
#include <string>
#include <pthread.h>
#define RUN_COUNT 1000

using namespace std;

//static variable

qsimproxy* qsimproxy :: Proxy = 0;


//! @param \c core_id  ID of the core.
//! @param \c conifg  Name of the config file.
//! @param \c cpuid  QSim cpu ID assigned to this core. Note this may be different from core ID. 
qsimproxy::qsimproxy(const QsimClient_Settings& settings, Proxy_Para& proxy_setting)
{
    //client_socket() requires port to be passed as a string, so convert to string first.
    char port_str[20];
    sprintf(port_str, "%d", settings.port);
    inst_count = 0;
    times_run = 0;

    char hostname[1024];
    gethostname(hostname,1023);
    //cout<<hostname<<endl;	
    string host_n(hostname);

    fstream conf_file;
    conf_file.open(proxy_setting.map_file_name);
    if (!conf_file)
    {
	cout<<" Fail to open the configure file "<<endl;
	exit(1);
    } 

    int core;
    string mach_name;

    conf_file >> core;
    conf_file >> mach_name;

    int core_count = 0;
    while (!conf_file.eof()) {

    	if (host_n == mach_name) {
	    cpu_id.push_back(core);
	    core_count++;
	}

	conf_file >> core;
	conf_file >> mach_name;
    }
    conf_file.close();

    m_Qsim_client.resize(cpu_id.size());
    m_Qsim_queue.resize(cpu_id.size());
    shm_buffers.resize(cpu_id.size());


    //cout<<"size of cpu_id: "<<cpu_id[1]<<endl;
    for (unsigned i = 0; i < cpu_id.size(); i++)
    {
    	m_Qsim_client[i] = new Qsim::Client(client_socket(settings.server, port_str));
    	m_Qsim_queue[i] = new Qsim::ClientQueue(*m_Qsim_client[i], cpu_id[i]);
	shm_buffers[i] = new SharedMemBuffer(proxy_setting.key_file_name, char(cpu_id[i] + 1), proxy_setting.buffer_size);
        shm_buffers[i]->writer_init();
    }
}



qsimproxy :: ~qsimproxy()
{
    for (unsigned i = 0; i < m_Qsim_client.size(); i++) {
        delete m_Qsim_queue[i];
        delete m_Qsim_client[i];
        delete shm_buffers[i];
    }
}


void qsimproxy :: start()
{
    pthread_t* threads = new pthread_t [cpu_id.size()];
    int* idx = new int[cpu_id.size()];

    Proxy = this;

    for(int i=0; i<cpu_id.size(); i++) {
        idx[i] = i;
	int rvalue = pthread_create(&threads[i], NULL, Thread_func, (void*)(&idx[i]));
	if (rvalue != 0){
	    cerr<<"fail to create pthread"<<endl;
	    exit(1);
	}
        else
	    cout<<"rvalue = "<<rvalue<<endl;
    }
    for(int i=0; i<cpu_id.size(); i++) {
        pthread_join(threads[i], NULL);
    }
    delete[] threads;
    delete[] idx;
}


 
// static function
void* qsimproxy :: Thread_func(void* arg)
{
    int* idxp = (int*)arg;
    Proxy->get_inst(*idxp);
}


void qsimproxy :: get_inst(int idx)
{
    assert(idx < cpu_id.size());

    if(cpu_id[idx] == 0)
	inst_count = 0;

cerr << "... thread " << idx << " filling up shm\n";

    while (!shm_buffers[idx]->is_full()) {
	int rc = m_Qsim_client[idx]->run(cpu_id[idx], RUN_COUNT);
	if(rc == 0 && m_Qsim_client[idx]->booted(cpu_id[idx]) == false) {//no more instructions
	    cerr << "cpu " << cpu_id[idx] << " out of insn fetching next pc\n";
	    return;
	}
	if (cpu_id[idx] == 0) {
	    inst_count += rc;

	    if (inst_count >= 1000000) {
		m_Qsim_client[idx]->timer_interrupt();
		inst_count = 0;
	    }
	}

	//write the content in m_Qsim_queue to shared memory buffers
	while (!m_Qsim_queue[idx]->empty() && !shm_buffers[idx]->is_full()) {
	    shm_buffers[idx]->write( m_Qsim_queue[idx]->front() );
	    m_Qsim_queue[idx]->pop();
	}
	if (shm_buffers[idx]->is_full()) {
            cout << "shm for cpu " << cpu_id[idx] << " is full = " << shm_buffers[idx]->get_buf_num_entries() << "\n";
	}
	else {
	    sleep(1);
	}
    }//while


    bool need_a_rest = true;
	
    while (true) {
	need_a_rest = true;

	//check if the buffer is low
        if (shm_buffers[idx]->is_empty() || (shm_buffers[idx]->get_buf_max_entries() > shm_buffers[idx]->get_buf_num_entries()*2)) {
	    int rc = m_Qsim_client[idx]->run(cpu_id[idx], RUN_COUNT);

	    if(rc == 0 && m_Qsim_client[idx]->booted(cpu_id[idx]) == false) {//no more instructions
                cerr << "cpu " << cpu_id[idx] << " out of insn fetching next pc\n";
                //???????????????? need to be changed to writer_done to all buffer
                return;
	    }

	    if (cpu_id[idx] == 0) {
	        inst_count += RUN_COUNT;

		if (inst_count >= 1000000) {
		    m_Qsim_client[idx]->timer_interrupt();
		    inst_count = 0;
		}
	    }	
	    need_a_rest = false;
	}

	//write the content in m_Qsim_queue to shared memory buffers
	while ( !m_Qsim_queue[idx]->empty() && !shm_buffers[idx]->is_full()) {
	    //cout<<"writing inst to shm buffer!!"<<endl;
	    shm_buffers[idx]->write( m_Qsim_queue[idx]->front() );
	    m_Qsim_queue[idx]->pop();
            //shm_buffers[i]->dbg_print();
	}    

    usleep(1);
	if (need_a_rest) {
	    usleep(10);
        }
    }//end of while 
#if 0
#endif
}





#endif //USE_QSIM
