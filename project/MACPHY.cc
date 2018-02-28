// DWSL - full radio stack

#include "MACPHY.hh"
#include <math.h>

MACPHY* ext_mp_ptr;
NET* ext_net_ptr;

// Implementing symmetric key generation via wireless physical layer from
// "Secure and Robust Symmetric Key Generation Using Physical Layer Techniques
// Under Various Wireless Environments" Cem Sahin, Brandon Katz, Kapil R. Dandekar
unsigned char checkKeyStrength(const unsigned char* key) {
    return 1;
}

int rxCallback(
        unsigned char *  _header,
        int              _header_valid,
        unsigned char *  _payload,
        unsigned int     _payload_len,
        int              _payload_valid,
        framesyncstats_s _stats,
        void *           _userdata,
        liquid_float_complex* G,
        liquid_float_complex* G_hat,
        unsigned int M
        )
{
    static const unsigned int N = 10; // confidence constant
    static unsigned char b_init = 0; // bool value to initialize the key and CTI
    static unsigned int p_idx = 0; // keeps track of which packet we are on
    static unsigned char key[1000]; // 1000 arbitrarily chosen to be greater than M
    static float CTI[2*N][1000];

    // Initialize the key to all 0's
    if(!b_init) {
	memset(key, '\0', 1000*sizeof(unsigned char));
	b_init = 1;
    }

    if(_header_valid)
    {
        // let first header byte be node id
        // let second header byte be source id
        if(_header[0]==ext_net_ptr->node_id)
        {
            // first two btyes of padded payload will be packet length
            unsigned int source_id = _header[1];
            if(_payload_valid)
            {
                unsigned int packet_length = ((_payload[0] << 8)|(_payload[1]));
                if(packet_length==0)
                {
                    return 1;
                }
                unsigned int num_written = ext_net_ptr->tt->cwrite((char*)(_payload+ext_mp_ptr->padded_bytes),packet_length);
                unsigned int packet_id = (_header[2] << 8) | _header[3];
		unsigned int loop, i, j;
		FILE* fp = fopen("generated_keys.txt", "a");
                //printf("Written %u bytes (PID %u) from %u",num_written,packet_id,source_id);
                if(M>0)
                {
		    // Obtain CSI and save it to CTI
		    for(loop=0; loop < M; loop++) {
			CTI[p_idx][loop] = sqrt(pow(std::real(G[loop]),2) + pow(std::imag(G[loop]),2));
		    }
		    p_idx++;
		    if(p_idx == 2*N) {
			// Play the game to generate the key
			unsigned int temp;
			for(i=0; i<240; i++) {
			    temp = 0;
			    for(j=1; j<2*N; j++) {
				if(CTI[j][i] > CTI[j-1][i]) {
				    temp++;
				}
			    }
			    if(temp >= N) {
				key[i] = '1';
			    }
			    else {
				key[i] = '0';
			    }
			}
			if(checkKeyStrength(key)) {
			    printf("Key generated\n");
			    fprintf(fp, "%s\n", key);
			}
			p_idx = 0;
		    }
                }
                else
                {
                    printf("\n");
                }
		fclose(fp);
            }
            else
            {
                printf("PAYLOAD INVALID\n");   
            }
        }
    }
    else
    {
        printf("HEADER INVALID\n");
    }

    return 0;
}

void run_demod(std::vector<std::complex<float> >* usrp_double_buff,unsigned int thread_idx)
{
    for(std::vector<std::complex<float> >::iterator double_it=usrp_double_buff->begin();double_it!=usrp_double_buff->end();double_it++)
    {
        std::complex<float> usrp_sample = *double_it;
        ((ext_mp_ptr->mcrx_list)->at(thread_idx))->Execute(&usrp_sample,1);
    }
    delete usrp_double_buff;
}

void rx_worker(unsigned int rx_thread_pool_size)
{
    const size_t max_samps_per_packet = ext_mp_ptr->usrp->get_device()->get_max_recv_samps_per_packet();

    // keep track of demod threads
    unsigned int ii;
    std::thread threads[rx_thread_pool_size];
    bool thread_joined[rx_thread_pool_size];
    for(ii=0;ii<rx_thread_pool_size;ii++)
    {
        thread_joined[ii] = true;
    }

    while(ext_mp_ptr->continue_running)
    {
        for(ii=0;ii<rx_thread_pool_size;ii++)
        {
            // figure out number of samples for the next slot
            size_t num_samps_to_deliver = (size_t)((ext_mp_ptr->usrp->get_rx_rate())*(ext_mp_ptr->slot_size))+(ext_mp_ptr->usrp->get_rx_rate()*(ext_mp_ptr->pad_size))*2.0;

            // init counter for samples and allocate double buffer
            size_t uhd_num_delivered_samples = 0;
            std::vector<std::complex<float> >* this_double_buff = new std::vector<std::complex<float> >;

            // calculate time to wait for streaming (precisely time beginning of each slot)
            double time_now;
            double wait_time;
            double full;
            double frac;
            uhd::time_spec_t uhd_system_time_now = ext_mp_ptr->usrp->get_time_now(0);
            time_now = (double)(uhd_system_time_now.get_full_secs()) + (double)(uhd_system_time_now.get_frac_secs());
            wait_time = ext_mp_ptr->frame_size - 1.0*fmod(time_now,ext_mp_ptr->frame_size)-(ext_mp_ptr->pad_size);
            frac = modf(time_now+wait_time,&full);

            // make new streamer with timing calc
            uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_MORE);
            stream_cmd.stream_now = false;
            stream_cmd.time_spec = uhd::time_spec_t((time_t)full,frac);
            ext_mp_ptr->rx_stream->issue_stream_cmd(stream_cmd);

            uhd::rx_metadata_t rx_md;

            while(uhd_num_delivered_samples<num_samps_to_deliver)
            {
                // create new vector to hold samps for this demod process
                std::vector<std::complex<float> > this_rx_buff(max_samps_per_packet);
                size_t this_num_delivered_samples = ext_mp_ptr->usrp->get_device()->recv(
                        &this_rx_buff.front(), this_rx_buff.size(), rx_md,
                        uhd::io_type_t::COMPLEX_FLOAT32,
                        uhd::device::RECV_MODE_ONE_PACKET
                        );
    
                for(unsigned int kk=0;kk<this_num_delivered_samples;kk++)
                {
                    this_double_buff->push_back(this_rx_buff[kk]);
                }

                //this_double_buff.push_back(this_rx_buff);
                uhd_num_delivered_samples += this_num_delivered_samples;
            }

            if(!thread_joined[ii])
            {
                threads[ii].join();
                thread_joined[ii] = true;
            }
            threads[ii] = std::thread(run_demod,this_double_buff,ii);
            thread_joined[ii] = false;
            
        }
    }
}

MACPHY::MACPHY(NET* net,
               double center_freq,
               double bandwidth,
               unsigned int padded_bytes,
               float tx_gain,
               float rx_gain,
               double frame_size,
               unsigned int rx_thread_pool_size,
               float pad_size,
               unsigned int packets_per_slot)
{
    ext_net_ptr = net;

    this->net = net;
    this->num_nodes_in_net = net->num_nodes_in_net;
    this->node_id = net->node_id;
    this->nodes_in_net = net->nodes_in_net;
    this->padded_bytes = padded_bytes;
    this->frame_size = frame_size;
    this->slot_size = frame_size/((double)(num_nodes_in_net));
    this->rx_thread_pool_size = rx_thread_pool_size;
    this->pad_size = pad_size;
    this->packets_per_slot = packets_per_slot;
    this->tx_transport_size = 256;

    // usrp general setup
    uhd::device_addr_t dev_addr;
    this->usrp = uhd::usrp::multi_usrp::make(dev_addr);
    this->usrp->set_rx_antenna("TX/RX");
    this->usrp->set_tx_antenna("TX/RX");
    this->usrp->set_tx_gain(tx_gain);
    this->usrp->set_rx_gain(rx_gain);
    this->usrp->set_tx_freq(center_freq);
    this->usrp->set_rx_freq(center_freq);
    this->usrp->set_rx_rate(2*bandwidth);
    this->usrp->set_tx_rate(2*bandwidth);

    // set time relative to system NTP time
    // (mod it down to fit in double precision)
    long usec;
    long sec;
    timeval tv;
    gettimeofday(&tv,0);
    usec = tv.tv_usec;
    sec = tv.tv_sec;
    this->usrp->set_time_now(uhd::time_spec_t((double)(sec%10)+((double)usec)/1e6));

    // usrp streaming setup
    uhd::stream_args_t rx_stream_args("fc32");
    this->rx_stream = this->usrp->get_rx_stream(rx_stream_args);
    uhd::stream_args_t tx_stream_args("fc32");
    this->tx_stream = this->usrp->get_tx_stream(tx_stream_args);

    // modem setup (list is for parallel demodulation)
    unsigned char* p = NULL;
    mctx = new multichanneltx(1, 480, 6, 4,p); 
    mcrx_list = new std::vector<multichannelrx*>;
    for(unsigned int jj=0;jj<rx_thread_pool_size;jj++)
    {
        framesync_callback callback[1];
        callback[0] = rxCallback;
        void* userdata[1];
        userdata[0] = NULL;
        multichannelrx* this_mcrx = new multichannelrx(1,480,6,4,(unsigned char*)NULL,userdata,callback);
        mcrx_list->push_back(this_mcrx);
    }

    this->continue_running = true;

    ext_mp_ptr = this;
}

MACPHY::~MACPHY()
{
    continue_running = false;
    delete mctx;
    for(std::vector<multichannelrx*>::iterator it=mcrx_list->begin();it!=mcrx_list->end();it++)
    {
        delete *it;
    }
    delete mcrx_list;
}

// OFDM PHY
void MACPHY::readyOFDMBuffer()
{
    tx_double_buff.clear();
    unsigned int packet_count = 0;
    int last_packet = -1;
    while((packet_count<packets_per_slot) && (net->tx_packets.size()>0))
    //for(packet_count=0;packet_count<packets_per_slot;packet_count++)
    {
        // construct next header and padded payload
        unsigned int packet_length;
        unsigned int dest_id;
        tx_packet_t* tx_packet = net->get_next_packet();
        packet_length = tx_packet->payload_size;
        if(packet_length>0)
        {
            if(last_packet!=(tx_packet->packet_id))
            {
                last_packet = tx_packet->packet_id;
                unsigned char* packet_payload = tx_packet->payload;
                //printf("STUFFING PACKET %u\n",(packet_payload[42+8]<<24)|(packet_payload[42+9]<<16)|(packet_payload[42+10]<<8)|(packet_payload[42+11]));
                dest_id = tx_packet->destination_id;
                unsigned char* padded_packet = new unsigned char[packet_length+padded_bytes];
                unsigned char header[8];
                memmove(padded_packet+padded_bytes,packet_payload,packet_length);
                padded_packet[0] = (packet_length>>8) & 0xff;
                padded_packet[1] = (packet_length) & 0xff;
                header[0] = dest_id;
                header[1] = node_id;
                header[2] = ((tx_packet->packet_id)>>8) & 0xff;
                header[3] = (tx_packet->packet_id) & 0xff;
                for(unsigned int ii=4;ii<8;ii++)
                {
                    header[ii] = 0&0xff;
                }
                mctx->UpdateData(0,header,padded_packet,padded_bytes+packet_length,LIQUID_MODEM_QPSK,LIQUID_FEC_CONV_V27,LIQUID_FEC_RS_M8);

                // populate usrp buffer
                unsigned int mctx_buffer_length = 2;
                std::complex<float> mctx_buffer[mctx_buffer_length];
                std::vector<std::complex<float> >* usrp_tx_buff = new std::vector<std::complex<float> >(tx_transport_size); 
                unsigned int num_generated_samples=0;
                while(!mctx->IsChannelReadyForData(0))
                {
                    mctx->GenerateSamples(mctx_buffer);
                    for(unsigned int jj=0;jj<mctx_buffer_length;jj++)
                    {
                        usrp_tx_buff->at(num_generated_samples) = (0.2f*mctx_buffer[jj]);
                        num_generated_samples++;
                    }
                    if(num_generated_samples==tx_transport_size)
                    {
                        tx_double_buff.push_back(usrp_tx_buff);
                        usrp_tx_buff = new std::vector<std::complex<float> >(tx_transport_size);
                        num_generated_samples = 0;
                    }
                }
                if(num_generated_samples>0)
                {
                    tx_double_buff.push_back(usrp_tx_buff);
                    num_generated_samples = 0;
                }
                delete[] padded_packet;
                delete tx_packet;
            }
        }
    }
}

// TDMA MAC
void MACPHY::TX_TDMA_OFDM()
{
    double time_now;
    double frame_pos;
    double wait_time;
    double full;
    double frac;
    uhd::tx_metadata_t tx_md;

    uhd::time_spec_t uhd_system_time_now = this->usrp->get_time_now(0);
    time_now = (double)(uhd_system_time_now.get_full_secs()) + (double)(uhd_system_time_now.get_frac_secs());

    frame_pos = fmod(time_now,frame_size);
    wait_time = ((node_id)*slot_size) - frame_pos;
    if(wait_time<0) 
    {
        printf("MISS\n");
        wait_time+=(frame_size);
    }
    frac = modf(time_now+wait_time,&full);
    tx_md.time_spec = uhd::time_spec_t((time_t)full,frac);
    tx_md.has_time_spec = true;
    tx_md.start_of_burst = false;
    tx_md.end_of_burst = false;

    // tx timed burst
    if(tx_double_buff.size()>0)
    {
        for(std::vector<std::vector<std::complex<float> >* >::iterator it=tx_double_buff.begin();it!=tx_double_buff.end();it++)
        {
            // tx that packet (each buffer in the double buff is one packet)
            tx_stream->send(&((*it)->front()),(*it)->size(),tx_md);
            delete *it;
        }
    }

    // clear buffer
    tx_md.start_of_burst = false;
    tx_md.end_of_burst = true;
    tx_stream->send("",0,tx_md,0.0);
    
    // readyNextBuffer
    readyOFDMBuffer();

    // wait out the rest of the slot
    uhd_system_time_now = this->usrp->get_time_now(0);
    double new_time_now =(double)(uhd_system_time_now.get_full_secs()) + (double)(uhd_system_time_now.get_frac_secs()); 
    while((new_time_now-(time_now+wait_time))<(frame_size-pad_size))
    {
        usleep(10);
        uhd_system_time_now = this->usrp->get_time_now(0);
        new_time_now =(double)(uhd_system_time_now.get_full_secs()) + (double)(uhd_system_time_now.get_frac_secs()); 
    }
}
