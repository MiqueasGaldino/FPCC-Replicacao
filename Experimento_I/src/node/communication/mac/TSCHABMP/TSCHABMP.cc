#include "TSCHABMP.h"
#include <vector>
#include <string.h>

Define_Module(TSCHABMP);

void inline TSCHABMP::medianValue(uint8_t node_addr) {
    int temp;
    if(ind == 1) rssi_median[1][node_addr] = rssi_median[0][node_addr];
    else if(ind == 2) rssi_median[1][node_addr] = (rssi_median[0][node_addr]+rssi_median[1][node_addr])/2;
    else{
        for(int i2 = 0; i2 < 3; i2++) {
            for(int j2 = 0; j2 < 2; j2++) {
                if(rssi_median[j2][node_addr] > rssi_median[j2+1][node_addr]) {
                    temp = rssi_median[j2][node_addr];
                    rssi_median[j2][node_addr] = rssi_median[j2+1][node_addr];
                    rssi_median[j2+1][node_addr] = temp;
                }
            }
        }
    }
}

void inline TSCHABMP::mediana(uint8_t node_addr) {
    for (int i1 = 0; i1 < ESTWINDOW; i1++) {
        ind = 0;
        for (int j1 = (i1 - 1); j1 <= (i1 + 1); j1++) {
            if (j1 >= 0 && j1 < ESTWINDOW) {
                rssi_median[ind++][node_addr] = rssi_values[j1][node_addr];
            }
        }
         medianValue(node_addr);
         rssi_values_avg[i1][node_addr] = rssi_median[1][node_addr];
         avg_s[node_addr] += rssi_values_avg[i1][node_addr];
    }
    avg_s[node_addr] = (double)avg_s[node_addr]/ESTWINDOW;
    avg_s[node_addr] = avg_s[node_addr]/255.0;
}

void TSCHABMP::calculatePRRu(uint8_t node_addr) { 
    if(avg_s[node_addr] > 0.5) {
        prr_u[node_addr] = 1.0;
    }
    else{
        prr_u[node_addr] = 526.56*pow(avg_s[node_addr],6) - 1333.2*pow(avg_s[node_addr],5) + 1315.5*pow(avg_s[node_addr],4) - 639.22*pow(avg_s[node_addr],3)+159.31*avg_s[node_addr]*avg_s[node_addr] - 19.251*avg_s[node_addr] + 0.9287;
        prr_u[node_addr] = 1.0-prr_u[node_addr];
    }
}

void TSCHABMP::calculatePRRd(uint8_t node_addr) {
    prr_d[node_addr] = prr_d[node_addr]*0.6 +  (0.0014*avg_dup[node_addr]*avg_dup[node_addr] - 0.0742*avg_dup[node_addr] + 0.9889)*0.4;
}

void TSCHABMP::startup() {
	setTimerDrift(0.0001);
	double time_offset = simTime().dbl();
	this->slotSize = par("slotSize").doubleValue()/1000.0;
	this->superFrameSize = par("superFrameSize");
	this->allocatedSlots = par("allocatedSlots").stdstringValue();
	this->slots = new bool[this->superFrameSize];
	this->numberOfAttempts = par("numberOfAttempts").longValue();
	memset(this->slots,0,this->superFrameSize*sizeof(bool));
	int k = 0;
	for(int i = 0; i < this->superFrameSize; i++) {
		this->slots[i] = (SELF_MAC_ADDRESS == ((int)(this->allocatedSlots[k]-'0')*10 + (int)(this->allocatedSlots[k+1]-'0')));
		k = k+2;
	}
	//setTimer(SWITCH_CHANNEL, (1.0-time_offset)/0.0001); //switch the channel every 10mss
	
	this->isCoordinator = par("isCoordinator");
	this->channelDiversity = par("channelDiversity");
	
	if(isCoordinator) {	
		setTimer(SEND_BEACON, (1.0-time_offset)/0.0001); //initialoffset for sending a beacon
		setTimer(WC_UP, 600000);
		setTimer(LQ_EST, 50010);
	}
	else{
		// O slotSize, nesse caso, 10.0, foi retirado do protocolo de Ruan (A B M P).
		// O slotSize definido no TSCH segue outro padrão. Como Ruan utilizou beacons, preferi colocar
		// esse valor diretamente abaixo. Espero lembrar disso no futuro.
		setTimer(BEACON_TIMEOUT_RESTART, ((QTDNODES*10*10.0)/0.1));	
	}
	this->beacon_cont = 0;
	this->slotCont = 0;
	this->ASN = 0;
	this->ack_bitmap = 0;
	
	for(int i = 0; i < QTDNODES; i++) {
		this->fail_cont[i] = 0;
		this->window[i] = 0;
		this->channel[i] = 0;
		this->id_ant[i] = 0;
		this->cont_interval[i] = 0;
		this->prr_u[i] = 1;
		this->avg_dup[i] = 0;
		this->prr_d[i] = 1;
	}
	
	this->prrThreshold = par("prrThreshold").doubleValue();
	this->beaconThreshold = par("beaconThreshold").doubleValue();
	this->channel_bitmap = 0;
	this->ch_beacon = 0;
	
	this->macBufferSize = par("macBufferSize").longValue();
	this->cont_interval_tx = 0;
	
	this->beacon_id = 0;
	this->blacklist = 0x00FF;
	for(int i = 0; i < 8; i++) {
		this->blacklist_aux[i] = i;
	}
	for(int i = 0; i < 8; i++) {
		map_chtobeacon[i] = i;
	}
	for(int i = 8; i < 16; i++) {
		map_chtobeacon[i] = -1;
	}
	this->first_ch = 0;
	for(int i = 0; i < 16; i++)
		this->ch_quality[i] = 1;
	this->blacklist_size = 8;
	this->waiting_beacon = true;
	this->beaconReceived = false;
	memset(this->cont_restart,0,QTDNODES*sizeof(int));
	memset(this->switch_channel,0,QTDNODES*sizeof(bool));
	this->new_value = false;
	this->waiting_b0 = true;
	this->b0_error = 0;
	memset(this->cont_packet_total,0,16*sizeof(int));
	memset(this->cont_packet_pernode,0,(16*QTDNODES)*sizeof(int));
	memset(this->cont_dup,0,16*sizeof(int));
	memset(this->cont_dup_pernode,0,(16*QTDNODES)*sizeof(int));
	this->start_change = false;	
}


/* Handle packet received from upper (network) layer.
 * We need to create a MAC packet, (here it can just be the generic MacPacket)
 * and encapsulate the received network packet before forwarding it to RadioLayer
 */

void TSCHABMP::fromNetworkLayer(cPacket * pkt, int destination)
{
	//Put the packet on the buffer. In this implementation the buffer has no limit
	packet *p = new packet;
	p->pkt = pkt;
	p->dest = destination;
	p->ack = true;
	buffer.push_back(p);
	if(buffer.size() > macBufferSize) {
		buffer.pop_front();
	}
}


void TSCHABMP::fromNetworkLayer(cPacket * pkt, int destination, bool ack)
{
	//Put the packet on the buffer. In this implementation the buffer has no limit
	packet *p = new packet;
	p->pkt = pkt;
	p->dest = destination;
	p->ack = ack;
	buffer.push_back(p);
	if(buffer.size() > macBufferSize) {
		buffer.pop_front();
	}
}

/* Handle packet received from lower (radio) layer.
 * We accept packets from all MAC protocols (cast to the base class MacPacket)
 * Then we filter by the destination field. By default we set the generic
 * destination field to broadcast when we encapsulate a NET packet. If a
 * specific protocol does not change that field then bypassMAC will be
 * operating in a promiscuous mode.
 */
void TSCHABMP::fromRadioLayer(cPacket * pkt, double rssi, double lqi)
{
	TSCHABMPPacket *macPkt = dynamic_cast <TSCHABMPPacket*>(pkt);
	if (macPkt == NULL)
		return;
	
	if(macPkt->getKindTSCHABMP() == BEACON_PACKET && waiting_beacon) {
		trace() << "Beacon received...";
		cancelTimer(BEACON_TIMEOUT_RESTART);
		
		cont_restart[SELF_MAC_ADDRESS] = 0;
		slotCont = 0;
		SF_offset = macPkt->getSequenceNumber();
	   
		//setTimer(BEGIN_CFP, (((8*slotSize)-0.212)/0.1)); //in this implementation the capReduction is enabled, and the nodes go directly to the CFP.
		
		// O slotSize, nesse caso, 10.0, foi retirado do protocolo de Ruan (A B M P).
		// O slotSize definido no TSCH segue outro padrão. Como Ruan utilizou beacons, preferi colocar
		// esse valor diretamente abaixo. Espero lembrar disso no futuro.
		setTimer(SWITCH_CHANNEL, ((10.0-0.212)/0.1));
		
		// O slotSize, nesse caso, 10.0, foi retirado do protocolo de Ruan (A B M P).
		// O slotSize definido no TSCH segue outro padrão. Como Ruan utilizou beacons, preferi colocar
		// esse valor diretamente abaixo. Espero lembrar disso no futuro.
		if(channelDiversity == 2)
			setTimer(BEACON_TIMEOUT, (((QTDNODES+1)*10.0)/0.1));
		
		beacon_id = macPkt->getBeaconId();
		
		trace() << "Mudei aqui, depois verificar...";
		if(!start_change && macPkt->getChangebl()) {
			//começa a mudar na próxima
			start_change = true;
			old_first_ch = first_ch;
			old_blacklist = blacklist;		
		}
		
		blacklist = macPkt->getBlacklist();
		channel[SELF_MAC_ADDRESS] = macPkt->getChannel()[SELF_MAC_ADDRESS];
		first_ch = macPkt->getFirstch();
		
		//trace() << "Received beacon " << beacon_id << " Channel: " << channel[SELF_MAC_ADDRESS] << " Firstch: " << first_ch << " Blacklist: " << blacklist;
		
		waiting_beacon = false;
		beaconReceived = true;
		
		//trace() << "Received ack, bitmap: " << macPkt->getAckBitmap();
				
		int ackbm = macPkt->getAckBitmap();
		
		bool txSuccess = false;
		
		if((ackbm & (1 << (SELF_MAC_ADDRESS-1)))) {
			txSuccess = true;
		}
		
		if(txSuccess && buffer_ret.size() > 0) {
			//trace() << "Received ack for the last packet";
			buffer_ret.pop_front();
		}
		else if(txSuccess){
			//trace() << "Received ack for the last packet - G2 ";
		}

	}
	else if (macPkt->getDestination() == SELF_MAC_ADDRESS ||
	    macPkt->getDestination() == BROADCAST_MAC_ADDRESS)
	{	
	
		if(macPkt->getKindTSCHABMP() == DATA_PACKET) { //DATA PACKET
			
			int p_id = beacon_id%blacklist_size;
			cont_packet_total[p_id]++;
			cont_packet_pernode[p_id][macPkt->getSource()]++;
			//trace() << "Received packet beacon " << p_id << " Total: " << cont_packet_total[p_id];
			
			if(macPkt->getB0error()) {
				cont_dup[0]++;
				cont_dup_pernode[0][macPkt->getSource()]++;
			//	trace() << "Perdeu b0, canal " << first_ch;
			}
			
			this->cont_interval[macPkt->getSource()] = 0;
			
			ack_bitmap = ack_bitmap | (1 << (slotCont-1));
		
			trace() << "Received packet: " << macPkt->getSequenceNumber() << "  from: " << macPkt->getSource() << " Power: " << rssi;
		
			//LQ calculation
			if(macPkt->getSequenceNumber() == id_ant[macPkt->getSource()]) {				
				int ant = beacon_id-2;
				if(ant == -1) ant = blacklist_size-1;
				else if(ant == - 2) ant = blacklist_size-2;
				ant = ant%blacklist_size;
				//trace() << "Duplicado - anterior " <<  ant;
				cont_dup[ant]++;
				cont_dup_pernode[ant][macPkt->getSource()]++;
			}
			id_ant[macPkt->getSource()] = macPkt->getSequenceNumber();
						
		    node_addr = macPkt->getSource();
		//	trace() << "Node_addr: " << node_addr;
		    rssi_values[contann[node_addr]%ESTWINDOW][node_addr] = (double)((rssi+94.0)/0.25);
		    contann[node_addr]++;
		    if(contann[node_addr] == ESTWINDOW || startedEstimation) {
				avg_s[node_addr] = 0;
			    mediana(node_addr);
			    calculatePRRu(node_addr);
			 //   trace() << "Calc LQu " << node_addr <<" "<< (prr_u[node_addr]);
				new_value = true;
				startedEstimation = true;
			}
			//trace() << "Ack_bitmap: " << ack_bitmap;
			toNetworkLayer(decapsulatePacket(macPkt));
	    }
	
	}
}

void TSCHABMP::timerFiredCallback(int index)
{
	switch (index){
		case WC_UP: { //only in the mode with channel hopping for the beacons		
			RadioControlCommand *radioCmd = new RadioControlCommand();
			radioCmd->setKind(RADIO_CONTROL_COMMAND);
			radioCmd->setRadioControlCommandKind(WC_UPD);
	 		send(radioCmd, "toRadioModule");	
			setTimer(WC_UP, 600000);
			//break;
	    }
		case LQ_EST: { //performs a new Link Quality Estimation
			for(int i = 1; i < QTDNODES; i++) {		
			   // trace() << "LQu " << i <<" "<< (prr_u[i]);
				if(new_value && prr_u[i] < prrThreshold) {
				//	trace() << "1 - Change channel of " << i << ": " << channel[i];
					switch_channel[i] = true;
					contann[i] = 0;
					prr_u[i] = 1;
					startedEstimation = false;
		 		}
			}
			setTimer(LQ_EST, 20000);
			break;	
		}
		case BEACON_TIMEOUT_RESTART: { 
			RadioControlCommand *radioCmd = new RadioControlCommand();
			radioCmd->setKind(RADIO_CONTROL_COMMAND);
			radioCmd->setRadioControlCommandKind(SET_CARRIER_FREQ);
			first_ch = (first_ch+1)%16;
			channel[0] = first_ch; //TODO: customize beacon ID -> here is set to 0
			double new_channel = 2405.0 + 5*first_ch;
			radioCmd->setParameter(new_channel);
	 		send(radioCmd, "toRadioModule");	
			//trace() << "Restart state, waiting on channel " << first_ch;
			setTimer(BEACON_TIMEOUT_RESTART, ((QTDNODES*10*slotSize)/0.1));	
			break;
		}
		case BEACON_TIMEOUT: { //only in the mode with channel hopping for the beacons
			//trace() << "Timeout expired, lost beacon";
			
			if(beacon_id%blacklist_size == 0) {
				if(buffer_ret.size() > 0) {
					b0_error = 1;
					buffer_ret.pop_front();
				//	trace() << "Erro na retransmissão, não recebeu beacon 0";
				}
			}
			
			cont_restart[SELF_MAC_ADDRESS]++;
			
			if(cont_restart[SELF_MAC_ADDRESS] == MAX_RESTART) {
				RadioControlCommand *radioCmd = new RadioControlCommand();
				radioCmd->setKind(RADIO_CONTROL_COMMAND);
				radioCmd->setRadioControlCommandKind(SET_CARRIER_FREQ);
				double new_channel = 2405.0 + 5*first_ch;
				channel[0] = first_ch; //TODO: customize beacon ID -> here is set to 0
				radioCmd->setParameter(new_channel);
		 		send(radioCmd, "toRadioModule");	
				setTimer(BEACON_TIMEOUT_RESTART, ((QTDNODES*10*slotSize)/0.1));	
			//	trace() << "Restart state, waiting on channel " << first_ch;
				break;
			}
			
			if(start_change) {
				RadioControlCommand *radioCmd = new RadioControlCommand();
				radioCmd->setKind(RADIO_CONTROL_COMMAND);
				radioCmd->setRadioControlCommandKind(SET_CARRIER_FREQ);
			
				beacon_id++;
			
				int ch_beacon = beacon_id%blacklist_size;
			
				int indch = old_first_ch;
				for(int i = 0; i < ch_beacon; i++) {
					indch = (indch+1)%16;
					while(!((old_blacklist >> indch)&1))
						indch = (indch+1)%16;
				}
			
				double new_channel = 2405.0 + 5*indch;
			
				channel[0] = indch; //TODO: customize beacon ID -> here is set to 0
				radioCmd->setParameter(new_channel);
	 		
				send(radioCmd, "toRadioModule");			
				setTimer(BEACON_TIMEOUT, (((QTDNODES+1)*slotSize)/0.1));	
			}
			else{
				RadioControlCommand *radioCmd = new RadioControlCommand();
				radioCmd->setKind(RADIO_CONTROL_COMMAND);
				radioCmd->setRadioControlCommandKind(SET_CARRIER_FREQ);
			
				beacon_id++;
			
				int ch_beacon = beacon_id%blacklist_size;
			
				int indch = first_ch;
				for(int i = 0; i < ch_beacon; i++) {
					indch = (indch+1)%16;
					while(!((blacklist >> indch)&1))
						indch = (indch+1)%16;
				}
			
				double new_channel = 2405.0 + 5*indch;
			
				channel[0] = indch; //TODO: customize beacon ID -> here is set to 0
				radioCmd->setParameter(new_channel);
	 		
				send(radioCmd, "toRadioModule");			
				setTimer(BEACON_TIMEOUT, (((QTDNODES+1)*slotSize)/0.1));
			}
			
			break;
		}
		case SEND_BEACON: {	
			
			if(beacon_id%blacklist_size == 0) {
				
				RadioControlCommand *radioCmd = new RadioControlCommand();
				radioCmd->setKind(RADIO_CONTROL_COMMAND);
				radioCmd->setRadioControlCommandKind(SET_CARRIER_FREQ);
						
				double new_channel = 2405.0 + 5*first_ch;
			
				channel[0] = first_ch; //TODO: customize beacon ID -> here is set to 0
				
				radioCmd->setParameter(new_channel);			
		 		send(radioCmd, "toRadioModule");
				
				if(start_change) {
					start_change = false;
				}
				else {
					
					//INCLUDE ANALYSIS OF DEEP FADING PER NODE
					
					for(int i = 0; i < blacklist_size; i++) {
						float N = (float)cont_packet_total[i];
						if((N + cont_dup[i]) > (QTDNODES*10)) { //10*QTDNODES (pacotes recebidos + duplicações notadas)
							bad_ch = blacklist_aux[i];
							if(N > 0) {
								ch_quality[bad_ch] = 0.4*((N+cont_dup[i])/N) + 0.6*ch_quality[bad_ch];
							}
							else {
								ch_quality[bad_ch] = 0.4*numberOfAttempts + 0.6*ch_quality[bad_ch];
							}
							//trace() << "Average number of ret beacon " << i << " and ch: " << bad_ch << " for : " << N << " packets and " << cont_dup[i] << " dup: " << ch_quality[bad_ch] ;
					
							float v = 1.0/ch_quality[bad_ch];
							
						 /*   avg_dup[node_addr] = cont_dup[i]/(N/10.0);
						    calculatePRRd(node_addr);
						    avg_dup[node_addr] = 0;
						    trace() << "LQd: " << (prr_d[node_addr]);
							trace() << "v: " << v;*/
		
							bool deep_backward = false;
							for(int l = 0; l < QTDNODES; l++) {
								float N_b = (float)cont_packet_pernode[i][l];
								if(N_b > 10) {
									N_b = (float)(N_b+cont_dup_pernode[i][l])/N_b;
									N_b = 1.0/N_b;
									//trace() << "No " << l << " beacon " << i << " Nb " << N_b;
									if(N_b < (beaconThreshold/2.0)) {
										//trace() << "A qualidade para o no " << l << " e beacon " << i << " esta ruim " << N_b;
										deep_backward = true;
									}
									cont_dup_pernode[i][l] = cont_packet_pernode[i][l] = 0;
								}		
							}
		
							if(v < beaconThreshold || deep_backward) {
								//COMO FAZER A MUDANCA SO NO OUTRO CICLO?
								start_change = true;
								//ARMAZENAR UM ESTADO NO TRANSMISSOR E NO RECEPTOR
								old_blacklist = blacklist;
								old_first_ch = first_ch;
							
								float min_v = 10;
								int j = (bad_ch+1)%16;
								int best_ch = j;
							//	trace() << "Blacklist " << blacklist;
								while(1) {
									//trace() << "Analisando canal " << j << " " << (blacklist >> j) << " " << ch_quality[j];	
									if(((blacklist >> j)&1) == 0 && ch_quality[j] < min_v) {
										min_v = ch_quality[j];
										best_ch = j;
									//	trace() << "Melhor por enquanto " << j;
										
									}
									j = (j+1)%16;
									if(min_v == 1 || j == bad_ch) {
										break;
									}
								}
								//trace() << "Saiu " << bad_ch << " e entrou " << best_ch;
								if(bad_ch == first_ch) {
									int nc = (first_ch+1)%16;
									int best_1ch = nc;
									min_v = 10;
									while(1) {
										if(((blacklist >> nc)&1) == 1 && ch_quality[nc] < min_v) {
											min_v = ch_quality[nc];
											best_1ch = nc;
										}
										if(min_v == 1 || nc == first_ch) {
											break;
										}
										nc = (nc+1)%16;
									}
									first_ch = best_1ch;
									//trace() << "Novo first_channel " << first_ch;
								}
								blacklist = blacklist ^ (1 << bad_ch);
								blacklist = blacklist | (1 << best_ch);
								blacklist_aux[0] = first_ch; //fica de fora durante um tempo
								int indch = (first_ch+1)%16;
								int k = 1;
								while(1) {
									if((blacklist>>indch)&1) {
										blacklist_aux[k] = indch;
										k++;
									}
									if(k == 16) {
										break;
									}
									indch = (indch+1)%16;
								}							
								ch_quality[best_ch] = 1;
				
								for(int k = 0; k < blacklist_size; k++) { 
									cont_packet_total[k] = cont_dup[k] = 0;
								}
								break;
							
							}
							cont_packet_total[i] = cont_dup[i] = 0;
						}
					}
				}	
			}
			else {
				if(!start_change) {
				
					RadioControlCommand *radioCmd = new RadioControlCommand();
					radioCmd->setKind(RADIO_CONTROL_COMMAND);
					radioCmd->setRadioControlCommandKind(SET_CARRIER_FREQ);
			
					int ch_beacon = blacklist_aux[beacon_id%blacklist_size];
			
					/*int indch = first_ch;
					for(int i = 0; i < ch_beacon; i++) {
						indch = (indch+1)%16;
						while(!((blacklist >> indch)&1))
							indch = (indch+1)%16;
					}*/
			
					double new_channel = 2405.0 + 5*ch_beacon;
			
					channel[0] = ch_beacon; //TODO: customize beacon ID -> here is set to 0
			
					radioCmd->setParameter(new_channel);			
			 		send(radioCmd, "toRadioModule");
				}
				else{
					RadioControlCommand *radioCmd = new RadioControlCommand();
					radioCmd->setKind(RADIO_CONTROL_COMMAND);
					radioCmd->setRadioControlCommandKind(SET_CARRIER_FREQ);
			
					int ch_beacon = beacon_id%blacklist_size;
			
					int indch = old_first_ch;
					for(int i = 0; i < ch_beacon; i++) {
						indch = (indch+1)%16;
						while(!((old_blacklist >> indch)&1))
							indch = (indch+1)%16;
					}
			
					double new_channel = 2405.0 + 5*indch;
			
					channel[0] = indch; //TODO: customize beacon ID -> here is set to 0
			
					radioCmd->setParameter(new_channel);			
			 		send(radioCmd, "toRadioModule");
				}
			}
			TSCHABMPPacket *macBeacon; 
			macBeacon = new TSCHABMPPacket("TSCHABMP beacon", MAC_LAYER_PACKET);
			macBeacon->setSource(SELF_MAC_ADDRESS);
			macBeacon->setDestination(BROADCAST_MAC_ADDRESS);
			macBeacon->setSequenceNumber(beacon_cont++);
			macBeacon->setKindTSCHABMP(BEACON_PACKET);
			macBeacon->setBlacklist(blacklist);
			macBeacon->setFirstch(first_ch);
			
			macBeacon->setChangebl(start_change);
		    /*if(start_change) {
				trace() << "enviando o antigo...";
				macBeacon->setBlacklist(old_blacklist);
				macBeacon->setFirstch(old_first_ch);
			}*/
		
			if(beacon_id%blacklist_size == 0) {
				for(int i = 1; i < QTDNODES; i++) {
					if(switch_channel[i]) {
						channel[i] = (channel[i]+1)%16;
					//	trace() << "Change channel of " << i << ": " << channel[i];
						switch_channel[i] = false;	
						window[i] = 0;
						new_value = false;
					}
				}
			}
		
			macBeacon->setChannel(channel);	
			macBeacon->setBeaconId(beacon_id);
			macBeacon->setAckBitmap(ack_bitmap);
			
			for(int i = 1; i < QTDNODES; i++) {
				cont_interval[i]++;
				if(cont_interval[i] >= 40) { //TODO: tornar parametrizavel (vou colocar duas vezes o periodo da APP -> 40 para 1 packet/s)
					//trace() << "Change channel of " << i << ": " << channel[i] << " due to deep fading";
					switch_channel[i] = true;
					cont_interval[i] = 0;
				}
			}			
			
			trace() << "Transmittiing beacon " << macBeacon->getSequenceNumber() << " on channel: " << channel[0];
			SF_offset = macBeacon->getSequenceNumber();
				
			toRadioLayer(macBeacon);
			toRadioLayer(createRadioCommand(SET_STATE, TX));
			beacon_id++;
			
			// O slotSize, nesse caso, 10.0, foi retirado do protocolo de Ruan (A B M P).
			// O slotSize definido no TSCH segue outro padrão. Como Ruan utilizou beacons, preferi colocar
			// esse valor diretamente abaixo. Espero lembrar disso no futuro.
			setTimer(SEND_BEACON, ((QTDNODES*10.0)/0.1));	//32 -> two superframes
			
			// O slotSize, nesse caso, 10.0, foi retirado do protocolo de Ruan (A B M P).
			// O slotSize definido no TSCH segue outro padrão. Como Ruan utilizou beacons, preferi colocar
			// esse valor diretamente abaixo. Espero lembrar disso no futuro.
			setTimer(SWITCH_CHANNEL, (10.0/0.1)); //goes to time slot 1
			
			slotCont = 0;
			ack_bitmap = 0;
			
			break;
		}
		case SWITCH_CHANNEL: { 
			if(!isCoordinator && !beaconReceived) {
				break;
			}
			//string addr(SELF_NETWORK_ADDRESS,1);
			//cont_ch[(int)(addr[0]-'0')]++;
			//double new_channel = 2405.0 + (cont_ch[(int)(addr[0]-'0')]%16)*5;
			//trace() << "Sim Time: " << simTime().dbl();
			//trace() << "Clock: " << getClock().dbl();						
			RadioControlCommand *radioCmd = new RadioControlCommand();
			radioCmd->setKind(RADIO_CONTROL_COMMAND);
			radioCmd->setRadioControlCommandKind(SET_CARRIER_FREQ);
			
			//int indch = ((int)(allocatedSlots[slotCont]-'0') + (ASN/superFrameSize))%16;
			//for star netowks only the Coordinator (node 0) receive packets. If a node with other ID receives the packet, it is necessary to sum the ID too.
			//(ASN/superFrameSize) = SLOTFRAMECOUNTER
			int indch = (ASN + (ASN/superFrameSize))%16;
			
			//double new_channel = 2405.0 + (ASN%16)*5;
			double new_channel = 2405.0 + indch*5;
							
			radioCmd->setParameter(new_channel);
	 		send(radioCmd, "toRadioModule");
			
			//setTimer(SWITCH_CHANNEL, (slotSize/0.0001));
			
			//buffer of packets to be retransmitted
			if(buffer_ret.size() > 0 && slots[slotCont]) {
				packet_ret *pr = buffer_ret.front();
				buffer_ret.pop_front();
	
				if(pr->cont < this->numberOfAttempts) {
					packet_ret *pr2 = new packet_ret;
					pr2->cont = pr->cont+1;
			
					pr2->packet = pr->packet->dup();
					pr2->packet->setSource(pr->packet->getSource());
					pr2->packet->setDestination(pr->packet->getDestination());
					pr2->packet->setAckReq(pr->packet->getAckReq());
					pr2->packet->setKindTSCHABMP(DATA_PACKET);
					buffer_ret.push_back(pr2);
					waiting_beacon = true;
					trace () << "Listening to ack";
					//trace() << "Colocando pacote para retransmissão depois da ret...";
										
				}	
				
				trace() << "Retransmitting packet " << pr->packet->getSequenceNumber() << " on: " << indch << "  slot: " << slotCont;
			//	pr->packet->setAckReq(false);
				toRadioLayer(pr->packet);
				toRadioLayer(createRadioCommand(SET_STATE, TX));
				
			}
			//buffer of new packets
			else if(buffer.size() > 0 && slots[slotCont]) {
				TSCHABMPPacket *macFrame = new TSCHABMPPacket("TSCHABMP packet", MAC_LAYER_PACKET);
				packet *p = buffer.front();
				cPacket *pkt = p->pkt;
				int destination = p->dest;
				buffer.pop_front();
				encapsulatePacket(macFrame, pkt);
				macFrame->setSource(SELF_MAC_ADDRESS);
				macFrame->setDestination(destination);
				macFrame->setAckReq(p->ack);
				macFrame->setKindTSCHABMP(DATA_PACKET);
				
				macFrame->setB0error(b0_error);
				b0_error = 0;
				
				if(p->ack && this->numberOfAttempts > 0) {
					packet_ret *pr = new packet_ret;
					pr->packet = macFrame->dup();
					pr->packet->setSource(SELF_MAC_ADDRESS);
					pr->packet->setDestination(destination);
					pr->packet->setAckReq(p->ack);
					pr->packet->setKindTSCHABMP(DATA_PACKET);
					pr->cont = 1;
					buffer_ret.push_back(pr);
					waiting_beacon = true;
					trace () << "Listening to ack";
					//trace() << "Colocando pacote para retransmissão...";
				}
				trace() << "Transmitting packet " << macFrame->getSequenceNumber() << " on: " << indch << "  slot: " << slotCont;
				toRadioLayer(macFrame);
				toRadioLayer(createRadioCommand(SET_STATE, TX));
			}
			slotCont++;
			if(slotCont < (QTDNODES-1)) { //31 -> two superframes (extract the 8 from the CAP)
				// O slotSize, nesse caso, 10.0, foi retirado do protocolo de Ruan (A B M P).
				// O slotSize definido no TSCH segue outro padrão. Como Ruan utilizou beacons, preferi colocar
				// esse valor diretamente abaixo. Espero lembrar disso no futuro.
				setTimer(SWITCH_CHANNEL, ((10.0)/0.1));
			}
			else if (!isCoordinator) {
				// O slotSize, nesse caso, 10.0, foi retirado do protocolo de Ruan (A B M P).
				// O slotSize definido no TSCH segue outro padrão. Como Ruan utilizou beacons, preferi colocar
				// esse valor diretamente abaixo. Espero lembrar disso no futuro.
				setTimer(END_FRAME, ((10.0-1)/0.1));
			}
			if(slotCont == superFrameSize){
				slotCont = 0;
			}

			ASN++;
			break;
		}
		case END_FRAME: {	
			beacon_id++;
			if(beacon_id%blacklist_size == 0) {
				waiting_beacon = true;
				beaconReceived = false;
				RadioControlCommand *radioCmd = new RadioControlCommand();
				radioCmd->setKind(RADIO_CONTROL_COMMAND);
				radioCmd->setRadioControlCommandKind(SET_CARRIER_FREQ);
				double new_channel = 2405.0 + 5*first_ch;
				radioCmd->setParameter(new_channel);
		 		send(radioCmd, "toRadioModule");	
				//trace() << "Going to first beacon, channel" << first_ch;	
				if(start_change) {
					//usar a nova configuração
					start_change = false;
				}
			}
			else if(waiting_beacon){
				beaconReceived = false;
				RadioControlCommand *radioCmd = new RadioControlCommand();
				radioCmd->setKind(RADIO_CONTROL_COMMAND);
				radioCmd->setRadioControlCommandKind(SET_CARRIER_FREQ);
				
			/*	int ch_beacon = beacon_id%blacklist_size;
		
				int indch = first_ch;
				for(int i = 0; i < ch_beacon; i++) {
					indch = (indch+1)%16;
					while(!((blacklist >> indch)&1))
						indch = (indch+1)%16;
				}
		
				double new_channel = 2405.0 + 5*indch;
		
				channel[0] = indch; //TODO: customize beacon ID -> here is set to 0
				
				radioCmd->setParameter(new_channel);*/
				
				if(start_change) {	
					int ch_beacon = beacon_id%blacklist_size;
					int indch = old_first_ch;
					for(int i = 0; i < ch_beacon; i++) {
						indch = (indch+1)%16;
						while(!((old_blacklist >> indch)&1))
							indch = (indch+1)%16;
					}
					double new_channel = 2405.0 + 5*indch;
					channel[0] = indch; //TODO: customize beacon ID -> here is set to 0
					radioCmd->setParameter(new_channel);
		
				}
				else{
					int ch_beacon = beacon_id%blacklist_size;	
					int indch = first_ch;
					for(int i = 0; i < ch_beacon; i++) {
						indch = (indch+1)%16;
						while(!((blacklist >> indch)&1))
							indch = (indch+1)%16;
					}
					double new_channel = 2405.0 + 5*indch;
					channel[0] = indch; //TODO: customize beacon ID -> here is set to 0
					radioCmd->setParameter(new_channel);
				}
		 		send(radioCmd, "toRadioModule");	
				//trace() << "Going to receive the ack through the beacon, channel " << channel[0];			
			}
			else {
				//trace() << "Skip beacon " << beacon_id;
				setTimer(BEACON_TIMEOUT, (((QTDNODES+1)*slotSize+1)/0.1));	
				setTimer(SWITCH_CHANNEL, ((slotSize+1)/0.1));
				waiting_beacon = false;
				beaconReceived = true;
				slotCont = 0;
			}
			break;
		}
		
	}
}



