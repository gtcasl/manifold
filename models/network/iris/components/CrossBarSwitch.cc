#include "CrossBarSwitch.h"

using namespace std;
using namespace manifold;
using namespace iris;

//! the constructor
//! initializing all the vector to given size and assign initial value to them
//! @param \c i_p  #of nodes connected to the switch, # of credits, the link bandwidth in bits
CrossBarSwitch::CrossBarSwitch(const CB_swith_init_params* i_p): 
                ports(i_p->no_nodes), credits(i_p->credits),
                input_buffer_state(i_p->no_nodes),
                order_record(i_p->no_nodes),
                channel_state_record(i_p->no_nodes, false),
                in_buffer(i_p->no_nodes),
                downstream_credits(i_p->no_nodes),
                cumu_port_delay(i_p->no_nodes),
                port_pkt_count(i_p->no_nodes),
                port_lst_pkt_out(i_p->no_nodes)
{
    //initial the value of each parameter
    for (uint i = 0; i < downstream_credits.size(); i++)
        downstream_credits[i] = credits;
        
    for (uint i = 0; i < cumu_port_delay.size(); i++)
        cumu_port_delay[i] = 0; 
      
    for (uint i = 0; i < port_pkt_count.size(); i++)
        port_pkt_count[i] = 0;  
        
    for (uint i = 0; i < port_lst_pkt_out.size(); i++)
        port_lst_pkt_out[i] = 0;    
        
    for (uint i = 0; i < input_buffer_state.size(); i++) 
    {
        input_buffer_state[i].input_port = -1;
        input_buffer_state[i].output_port = -1;
        input_buffer_state[i].pkt_in_time = 0;
    }      
        
    stat_packets_in = 0;
    stat_packets_out = 0;
    stat_flits_in = 0;
    stat_flits_out = 0; 
    switch_ave_delay = 0;   
}

//! the deconstructor
CrossBarSwitch::~CrossBarSwitch()
{
}

//! response for putting flits into buffer and add the downstream credit
//! @param \c port  The port # where the data come from
//! @param \c data  The link data come from interface which can be two types: credit and flit
void CrossBarSwitch::handle_inf_arrival( int inputid, LinkData* data)
{
    switch ( data->type ) {
        case FLIT:
            {
                if (data->f->type == HEAD)
                {
                    //generate the order record for out queue to guarantee the network follow FIFO principal
                    order_record[static_cast<HeadFlit*>(data->f)->dst_id].push_back(inputid);
                    //generate the record when header flit coming
                    stat_packets_in++;
                }
                
                stat_flits_in++;
                in_buffer[inputid].push_back(data->f);
                break;
            }

        case CREDIT:
            {
                //add credit back when credit coming
                downstream_credits[inputid]++;
                assert(downstream_credits[inputid] <= (uint)credits);
                break;
            }

        default:	
            assert(0);
            break;
    }	
    // delete ld.. if ld contains a flit it is not deleted. see destructor
    delete data;
}

void CrossBarSwitch::tick (void)
{
    //set the output port
    for (uint i = 0; i < ports; i++)
    {
        if ((in_buffer[i].size() > 0) &&
            (in_buffer[i].front()->type == HEAD))
        {
             //modify the state record when next flit to be sent is header flit
             input_buffer_state[i].input_port = i;
             input_buffer_state[i].output_port = static_cast<HeadFlit*>(in_buffer[i].front())->dst_id;
             input_buffer_state[i].pkt_in_time = manifold::kernel::Manifold::NowTicks();
        }
    }
    
    //send the flit base on FIFO principal
    for (uint i = 0; i < ports; i++)
    {
        if ((order_record[i].size() > 0) && 
            (in_buffer[order_record[i].front()].size() > 0) &&  //there is flit to be sent
            (i == input_buffer_state[order_record[i].front()].output_port) && //the outgoing port matches the record
            (downstream_credits[i] > 0)) //the output port has credits
        {
            Flit* f = in_buffer[order_record[i].front()].front();
            in_buffer[order_record[i].front()].pop_front();
            
            //generate and send the flit to destination
            LinkData* ld = new LinkData();
            ld->type = FLIT;
            ld->src = -1;
            f->virtual_channel = 0;
            ld->f = f;
            ld->vc = 10;
            Send(i, ld);
            stat_flits_out++;
            
            //deduce the credit
            downstream_credits[i]--;
            
            //generate and send the credit back to source
            LinkData* ldc = new LinkData();
            ldc->type = CREDIT;
            ldc->src = -1;
            ldc->vc = 0;
            Send(order_record[i].front(), ldc);
            
            //if the flit is tail delete its erecord indicate a pkt has been sent, and generate state records
            if (f->type == TAIL)
            {
                uint lat = manifold::kernel::Manifold::NowTicks()- input_buffer_state[order_record[i].front()].pkt_in_time;
                port_lst_pkt_out[i] = manifold::kernel::Manifold::NowTicks();
                cumu_port_delay[i] += lat;
                switch_ave_delay += lat;
                order_record[i].pop_front();
                port_pkt_count[i]++;
                stat_packets_out++;
            }
        }    
    }
}

//! nothing to do at tock
void CrossBarSwitch::tock (void)
{
    return;
}

//! output the record in switch
std::string CrossBarSwitch::print_stats ( void )
{
    std::stringstream str;
    str 
        << "\n CrossBarSwitch packets_in: " << stat_packets_in
        << "\n CrossBarSwitch packets_out: " << stat_packets_out
        << "\n CrossBarSwitch flits_in: " << stat_flits_in
        << "\n CrossBarSwitch flits_out: " << stat_flits_out
        << "\n CrossBarSwitch avg_router_latency: " << (switch_ave_delay+0.0)/stat_packets_out
        ;
    str << "\n CrossBarSwitch per_port_pkts_out: ";
    for ( uint i=0; i<ports; i++)
        str << port_pkt_count[i] << " ";
        

    str << "\n CrossBarSwitch per_port_avg_lat: ";
    for ( uint i=0; i<ports; i++)
        if ( port_pkt_count[i] ) 
            str << double(cumu_port_delay[i])/double(port_pkt_count[i]) << " ";
        else
            str  << "0 ";
            
    str << "\n CrossBarSwitch per_port_last_pkt_out: ";
    for ( uint i=0; i<ports; i++)
        str << port_lst_pkt_out[i] << " ";
        
    if ( ( stat_packets_in - stat_packets_out ) > ports ) str << "\n ERROR pkts_in-pkts_out > buffering available";
    str << std::endl;

    return str.str();
}
