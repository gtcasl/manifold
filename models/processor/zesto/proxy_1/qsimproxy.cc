#ifdef USE_QSIM

#include "qsimproxy.h"
#include <assert.h>
#include <string>
#define RUN_COUNT 500

using namespace std;

//! @param \c core_id  ID of the core.
//! @param \c conifg  Name of the config file.
//! @param \c cpuid  QSim cpu ID assigned to this core. Note this may be different from core ID. 
qsimproxy::qsimproxy(const QsimClient_Settings& settings, Proxy_Para& proxy_setting)
{
    //client_socket() requires port to be passed as a string, so convert to string first.
    char port_str[20];
    sprintf(port_str, "%d", settings.port);
    inst_count = 0;
    initialized = false;

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

    int rank;
    int core;
    string mach_name;

    conf_file >> rank;
    conf_file >> core;
    conf_file >> mach_name;

    int core_count = 0;
    while (!conf_file.eof()) {

    	if (host_n == mach_name) {
	    cpu_id.push_back(core);
	    core_count++;
	}

	conf_file >> rank;
	conf_file >> core;
	conf_file >> mach_name;
    }
    conf_file.close();

    m_Qsim_client.resize(cpu_id.size());
    m_Qsim_queue.resize(cpu_id.size());
    shm_buffers.resize(cpu_id.size());


    //cout<<"size of cpu_id: "<<cpu_id[1]<<endl;
    for (int i = 0; i < cpu_id.size(); i++)
    {
    	m_Qsim_client[i] = new Qsim::Client(client_socket(settings.server, port_str));
    	m_Qsim_queue[i] = new Qsim::ClientQueue(*m_Qsim_client[i], cpu_id[i]);
	shm_buffers[i] = new SharedMemBuffer(proxy_setting.key_file_name, char(cpu_id[i] + 1), proxy_setting.buffer_size);
        shm_buffers[i]->writer_init();
    }

}



qsimproxy:: ~qsimproxy()
{
    for (int i = 0; i < m_Qsim_client.size(); i++)
    {
        delete m_Qsim_queue[i];
        delete m_Qsim_client[i];
        delete shm_buffers[i];
    }
}


void qsimproxy::get_inst()
{
    inst_count = 0;

cerr << "... filling up shm\n";
    int full_count=0;

    while(full_count < cpu_id.size()) {
	for (int i = 0; i < cpu_id.size(); i++) {
            if (!shm_buffers[i]->is_full()) {

	        int rc = m_Qsim_client[i]->run(cpu_id[i], RUN_COUNT);
	        if(rc == 0 && m_Qsim_client[i]->booted(cpu_id[i]) == false) {//no more instructions
                    cerr << "cpu " << cpu_id[i] << " out of insn fetching next pc\n";
                    return;
	        }
	        if (cpu_id[i] == 0) {
	            inst_count += rc;

	            if (inst_count >= 1000000) {
		        m_Qsim_client[i]->timer_interrupt();
		        inst_count = 0;
	            } 
	        }	
		
	        //write the content in m_Qsim_queue to shared memory buffers
	        while (!m_Qsim_queue[i]->empty() && !shm_buffers[i]->is_full()) {
		    shm_buffers[i]->write( m_Qsim_queue[i]->front() );
		    m_Qsim_queue[i]->pop();
	        }
		if (shm_buffers[i]->is_full()) {
cout << "shm for cpu " << cpu_id[i] << " is full = " << shm_buffers[i]->get_buf_num_entries() << "\n";
                    full_count++;
                }
	    }//if	
	}//for	
    }
    cerr << "all segments are full!\n";


    bool need_a_rest = true;
    //double time_for_run = 0; //accumulated time in micro seconds to call run().
    //times_run = 0; //number of times run() is called.
	
    while (true) {
	need_a_rest = true;

	//check if the buffer is low
	for (int i = 0; i < cpu_id.size(); i++) {
	    if (shm_buffers[i]->is_empty() || (shm_buffers[i]->get_buf_max_entries() > shm_buffers[i]->get_buf_num_entries()*2)) {
		//timespec t_start;
		//clock_gettime(CLOCK_REALTIME, &t_start);
		    
		int rc = m_Qsim_client[i]->run(cpu_id[i], RUN_COUNT);
		    
		//timespec t_end;
      		//clock_gettime(CLOCK_REALTIME, &t_end);
      		//double time_for_run = (t_end.tv_sec - t_start.tv_sec) * 1000000.0 + (t_end.tv_nsec - t_start.tv_nsec)/1000.0;
		//times_run++;

		if(rc == 0 && m_Qsim_client[i]->booted(cpu_id[i]) == false) {//no more instructions
                    cerr << "cpu " << cpu_id[i] << " out of insn fetching next pc\n";
                    //???????????????? need to be changed to writer_done to all buffer
                    return;
		}
		if (cpu_id[i] == 0) {
		    inst_count += RUN_COUNT;

		    if (inst_count >= 1000000) {
			m_Qsim_client[i]->timer_interrupt();
			inst_count = 0;
		    }
		}	
		need_a_rest = false;
	    }

	    //write the content in m_Qsim_queue to shared memory buffers
	    while ( !m_Qsim_queue[i]->empty() && !shm_buffers[i]->is_full()) {
		//cout<<"writing inst to shm buffer!!"<<endl;
		shm_buffers[i]->write( m_Qsim_queue[i]->front() );
		m_Qsim_queue[i]->pop();
                //shm_buffers[i]->dbg_print();
	    }    
	}	

	if (need_a_rest) {
	    usleep(1);
        }
    }//end of while 

    //cout<<"everage time to get "<< RUN_COUNT <<" instruction from server is: "<<(time_for_run/times_run)<<endl;
}





#endif //USE_QSIM
