/****************************************************************************
 *  Copyright: National ICT Australia,  2007 - 2011                         *
 *  Developed at the ATP lab, Networked Systems research theme              *
 *  Author(s): Athanassios Boulis, Yuriy Tselishchev                        *
 *  This file is distributed under the terms in the attached LICENSE file.  *
 *  If you do not find this file, copies can be found by writing to:        *
 *                                                                          *
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia             *
 *      Attention:  License Inquiry.                                        *
 *                                                                          *
 ****************************************************************************/

#include "ThroughputTest.h"
#include <cmath>
#include <cstdlib>
#include <ctime>

Define_Module(ThroughputTest);

void ThroughputTest::startup()
{
	packet_rate = par("packet_rate");
	recipientAddress = par("nextRecipient").stringValue();
	startupDelay = par("startupDelay");
	packet_spacing = packet_rate > 0 ? 1 / float (packet_rate) : -1;
	dataSN = 0;


	if (packet_spacing > 0 && recipientAddress.compare(SELF_NETWORK_ADDRESS) != 0)
		setTimer(SEND_PACKET, packet_spacing + startupDelay);
	else
		trace() << "Not sending packets";

	declareOutput("Packets received per node");
}

void ThroughputTest::fromNetworkLayer(DataPacket * rcvPacket,
		const char *source, double rssi, double lqi)
{
	int sequenceNumber = rcvPacket->getSequenceNumber();
	double* data = rcvPacket->getDataPacket();
	
	trace() << "Está aqui";
	trace() << "Endereco: " << recipientAddress.c_str();
	trace() << "Atual: " << SELF_NETWORK_ADDRESS;

	//SINK NODE
	if (recipientAddress.compare(SELF_NETWORK_ADDRESS) == 0) {
		//data packet received
		trace() << "Received packet " << sequenceNumber << " from node " << source << ": " << rssi << " " << lqi << " fromNetworkLayer";
		packets_received++;
		//os dados do pacote estão no array data (para acessar é só indexar data[0], data[1] etc
	} 
	//END NODE
	else {
		
		DataPacket* fwdPacket = rcvPacket->dup();
		// Reset the size of the packet, otherwise the app overhead will keep adding on
		fwdPacket->setByteLength(0);
		toNetworkLayer(fwdPacket, recipientAddress.c_str());
		
	}
	//free(data);
}

void ThroughputTest::timerFiredCallback(int index)
{
	
	switch (index){
		case SEND_PACKET: {

			string addr(SELF_NETWORK_ADDRESS,1);
			
			trace() << "Sending packet #" << dataSN;
			double *data = new double[10];
			data[0] = 0;
			data[1] = prr_beacon_end;
			data[2] = RNP;
			DataPacket* data_packet = createDataPacket(data, dataSN, 80);
			trace() << "Sorvedouro: " << recipientAddress.c_str();
			
			toNetworkLayer(data_packet, recipientAddress.c_str());
			dataSN++;
			setTimer(SEND_PACKET, packet_spacing);
			break;
		}
	}
}



void ThroughputTest::sendAck(int packet_id)
{
	//trace() << "Sending ack for packet " << packet_id;
    double *data = new double[10];
	data[0] = packet_id;
	DataPacket* data_packet = createDataPacket(data, ack_seq, 5);  
	toNetworkLayer(data_packet, (const char*)("1"));
	acks_sent++;
	//trace() << "Acks sent: " << acks_sent;
}


// This method processes a received carrier sense interupt. Used only for demo purposes
// in some simulations. Feel free to comment out the trace command.
void ThroughputTest::handleRadioControlMessage(RadioControlMessage *radioMsg)
{
	switch (radioMsg->getRadioControlMessageKind()) {
		case CARRIER_SENSE_INTERRUPT:
			trace() << "CS Interrupt received! current RSSI value is: " << radioModule->readRSSI();
                        break;
	}
}

