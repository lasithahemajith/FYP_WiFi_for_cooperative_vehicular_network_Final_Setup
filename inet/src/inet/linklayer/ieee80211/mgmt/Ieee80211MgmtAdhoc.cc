//
// Copyright (C) 2006 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#include "inet/linklayer/ieee80211/mgmt/Ieee80211MgmtAdhoc.h"
#include "inet/linklayer/common/Ieee802Ctrl.h"
#include "inet/physicallayer/ieee80211/packetlevel/Ieee80211Radio.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/NotifierConsts.h"
#include "inet/linklayer/ieee80211/mgmt/Ieee80211MgmtAP.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h"

#ifdef WITH_ETHERNET
#include "inet/linklayer/ethernet/EtherFrame.h"
#endif // ifdef WITH_ETHERNET

namespace inet {

namespace ieee80211 {

using namespace physicallayer;

Define_Module(Ieee80211MgmtAdhoc);
Register_Class(Ieee80211MgmtAdhoc::NotificationInfoSta);

void Ieee80211MgmtAdhoc::initialize(int stage)
{
    endToEndDelay=0;
    packetCount=0;
    numSent=0;
    numReceived=0;
    preTime = 0;
    PIRTotal = 0;
    MAX_delay=0;
    MAX_PIR=0;
    MIN_PIR=10000;
    Ieee80211MgmtBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        // read params and init vars
        ssid = par("ssid").stringValue();
        beaconInterval = par("beaconInterval");
        numAuthSteps = par("numAuthSteps");
        if (numAuthSteps != 2 && numAuthSteps != 4)
            throw cRuntimeError("parameter 'numAuthSteps' (number of frames exchanged during authentication) must be 2 or 4, not %d", numAuthSteps);
        channelNumber = -1;    // value will arrive from physical layer in receiveChangeNotification()
        WATCH(ssid);
        WATCH(channelNumber);
        WATCH(beaconInterval);
//        WATCH(numAuthSteps);
//        WATCH_MAP(staList);

        //TBD fill in supportedRates

        // subscribe for notifications
        cModule *radioModule = getModuleFromPar<cModule>(par("radioModule"), this);
//        radioModule->subscribe(Ieee80211Radio::radioChannelChangedSignal, this);

        // start beacon timer (randomize startup time)
        beaconTimer = new cMessage("beaconTimer");
    }
    else if (stage == INITSTAGE_LINK_LAYER) {
        if (isOperational)
            scheduleAt(simTime() + uniform(0, beaconInterval), beaconTimer);
    }


    path="results/output/"+ssid+".csv";
    pathAll="results/output/all.csv";
    pathBrief="results/output/brief.csv";

    if(fileAll.is_open()!=true){
        fileAll.open(pathAll,std::ios::app);
    }
}

void Ieee80211MgmtAdhoc::handleTimer(cMessage *msg)
{
    if (msg == beaconTimer) {
//            sendBeacon();
            scheduleAt(simTime() + beaconInterval, beaconTimer);
        }
        else {
            throw cRuntimeError("internal error: unrecognized timer '%s'", msg->getName());
        }
//    ASSERT(false);
}

void Ieee80211MgmtAdhoc::handleUpperMessage(cPacket *msg)
{
    Ieee80211DataFrame *frame = encapsulate(msg);
    sendDown(frame);
}

void Ieee80211MgmtAdhoc::handleCommand(int msgkind, cObject *ctrl)
{
    throw cRuntimeError("handleCommand(): no commands supported");
}

Ieee80211DataFrame *Ieee80211MgmtAdhoc::encapsulate(cPacket *msg)
{
    Ieee80211DataFrameWithSNAP *frame = new Ieee80211DataFrameWithSNAP(msg->getName());

    // copy receiver address from the control info (sender address will be set in MAC)
    Ieee802Ctrl *ctrl = check_and_cast<Ieee802Ctrl *>(msg->removeControlInfo());
    frame->setReceiverAddress(ctrl->getDest());
    frame->setEtherType(ctrl->getEtherType());
    int up = ctrl->getUserPriority();
    if (up >= 0) {
        // make it a QoS frame, and set TID
        frame->setType(ST_DATA_WITH_QOS);
        frame->addBitLength(QOSCONTROL_BITS);
        frame->setTid(up);
    }
    delete ctrl;

    frame->encapsulate(msg);
    return frame;
}

cPacket *Ieee80211MgmtAdhoc::decapsulate(Ieee80211DataFrame *frame)
{
    cPacket *payload = frame->decapsulate();

    Ieee802Ctrl *ctrl = new Ieee802Ctrl();
    ctrl->setSrc(frame->getTransmitterAddress());
    ctrl->setDest(frame->getReceiverAddress());
    int tid = frame->getTid();
    if (tid < 8)
        ctrl->setUserPriority(tid); // TID values 0..7 are UP
    Ieee80211DataFrameWithSNAP *frameWithSNAP = dynamic_cast<Ieee80211DataFrameWithSNAP *>(frame);
    if (frameWithSNAP)
        ctrl->setEtherType(frameWithSNAP->getEtherType());
    payload->setControlInfo(ctrl);

    delete frame;
    return payload;
}

void Ieee80211MgmtAdhoc::handleDataFrame(Ieee80211DataFrame *frame)
{
    sendUp(decapsulate(frame));
}

void Ieee80211MgmtAdhoc::handleAuthenticationFrame(Ieee80211AuthenticationFrame *frame)
{
    dropManagementFrame(frame);
}

void Ieee80211MgmtAdhoc::handleDeauthenticationFrame(Ieee80211DeauthenticationFrame *frame)
{
    dropManagementFrame(frame);
}

void Ieee80211MgmtAdhoc::handleAssociationRequestFrame(Ieee80211AssociationRequestFrame *frame)
{
    dropManagementFrame(frame);
}

void Ieee80211MgmtAdhoc::handleAssociationResponseFrame(Ieee80211AssociationResponseFrame *frame)
{
    dropManagementFrame(frame);
}

void Ieee80211MgmtAdhoc::handleReassociationRequestFrame(Ieee80211ReassociationRequestFrame *frame)
{
    dropManagementFrame(frame);
}

void Ieee80211MgmtAdhoc::handleReassociationResponseFrame(Ieee80211ReassociationResponseFrame *frame)
{
    dropManagementFrame(frame);
}

void Ieee80211MgmtAdhoc::handleDisassociationFrame(Ieee80211DisassociationFrame *frame)
{
    dropManagementFrame(frame);
}

void Ieee80211MgmtAdhoc::handleBeaconFrame(Ieee80211BeaconFrame *frame)
{
//    packetCount+=1;
//
//    simtime_t eed=frame->getArrivalTime()-frame->getCreationTime();
//    endToEndDelay+=eed;
//
//    Ieee80211BeaconFrameBody& body = frame->getBody();
//    const char *s = body.getSSID();
//    std::string frame_ssid(s);
//
//    numReceived++;
//    std::string data;//= std::to_string(numReceived);
//    data=frame->getArrivalTime().str(); // Arrival Time
//    data=data+","+(std::to_string(int(frame->par("ID")))); // Packet id
//    data=data+","+(std::to_string(double(frame->par("RSSID")))); // RSSID
//    data=data+","+(std::to_string(int(frame->getByteLength())));// PAcket size
//    data=data+","+(frame_ssid);// Sender ssid
//    data=data+","+(ssid);// Receiver ssid
//    data=data+","+(eed.str()); // End to End Delay
//    data=data+","+(std::to_string(double(frame->par("S_POSX"))));
//    data=data+","+(std::to_string(double(frame->par("S_POSY"))));
//    data=data+","+(std::to_string(double(frame->par("S_SPD")))); // Sender speed
//
//    cModule *receiver = getContainingNode(this);
//    IMobility *mobility = check_and_cast<IMobility *>(receiver->getSubmodule("mobility"));
//    Coord receiverPosition = mobility->getCurrentPosition();
//    Coord receiverSpeed = mobility->getCurrentSpeed();
//
//    data=data+","+(std::to_string(double(receiverPosition.x)));
//    data=data+","+(std::to_string(double(receiverPosition.y)));
//    data=data+","+(std::to_string(double(receiverSpeed.x)));
//    //"Arrival_Time,Packet_ID,RSSID,Packet_Size,Sender,Receiver,EED,S_POSX,S_POSY,S_SPD,R_POSX,R_POSY,R_SPD"
//
//
//    fileAll<<data<<endl;
    dropManagementFrame(frame);
}

void Ieee80211MgmtAdhoc::handleProbeRequestFrame(Ieee80211ProbeRequestFrame *frame)
{
    dropManagementFrame(frame);
}

void Ieee80211MgmtAdhoc::handleProbeResponseFrame(Ieee80211ProbeResponseFrame *frame)
{
    dropManagementFrame(frame);
}

////////////////////////////////////////////////////////////////////////////////////////////////////


void Ieee80211MgmtAdhoc::finish()
{ // put your scalars to be recorded at the end of the simulation
    //    EV << "finishing" << endl;
//        endToEndDelay=endToEndDelay/packetCount;
//        double pdr=0;
//        double pir=0;
//        if (numReceived!=0 ){
//            pir=PIRTotal.dbl()/(numReceived-1);
//
//        }
//        recordScalar("end to end delay",endToEndDelay);
//        recordScalar("packet count",packetCount);
//        recordScalar("packet delivery ratio",pdr);
//        recordScalar("PIR",pir);
//        recordScalar("received",numReceived);
//        recordScalar("sent",numSent);
//        recordScalar("MAX_DELAY",MAX_delay);
//        recordScalar("MIN_DELAY",MIN_delay);
//        recordScalar("MAX_PIR",MAX_PIR);
//        recordScalar("MIN_PIR",MIN_PIR);
//
//
//        myfile<<"I'm closed"<<endl;
//        myfile.close();
    fileBrief.open(pathBrief,std::ios::app);
    fileBrief<<ssid+","+std::to_string(numSent)<<endl;
    fileBrief.close();

}


///////////////////////////////////////////////////////////////////////////////////////////////////









} // namespace ieee80211

} // namespace inet

