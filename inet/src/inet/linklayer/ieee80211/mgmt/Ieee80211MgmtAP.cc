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
#include <thread>
#include <chrono>
#include <utility>
#include <functional>
#include <atomic>
#include <random>

#include "inet/linklayer/ieee80211/mgmt/Ieee80211MgmtAP.h"

#include "inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h"
#include "inet/linklayer/common/Ieee802Ctrl.h"

#ifdef WITH_ETHERNET
#include "inet/linklayer/ethernet/EtherFrame.h"
#endif // ifdef WITH_ETHERNET

#include "inet/physicallayer/ieee80211/packetlevel/Ieee80211Radio.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/NotifierConsts.h"

namespace inet {

namespace ieee80211 {

using namespace std;
using namespace physicallayer;

Define_Module(Ieee80211MgmtAP);
Register_Class(Ieee80211MgmtAP::NotificationInfoSta);

static std::ostream& operator<<(std::ostream& os, const Ieee80211MgmtAP::STAInfo& sta)
{
    os << "state:" << sta.status;
    return os;
}

Ieee80211MgmtAP::~Ieee80211MgmtAP()
{
    cancelAndDelete(beaconTimer);
}

void Ieee80211MgmtAP::initialize(int stage)
{
    Ieee80211MgmtAPBase::initialize(stage);
    endToEndDelay=0;
    packetCount=0;
    numSent=0;
    numReceived=0;
    preTime = 0;
    PIRTotal = 0;
    MAX_delay=0;
    MAX_PIR=0;
    MIN_PIR=10000;
    intervalCount = 0;
    addreceivers(19);
    //threadFuncA();
    //threadFuncB();

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
        WATCH(numAuthSteps);
        WATCH_MAP(staList);

        //TBD fill in supportedRates

        // subscribe for notifications
        cModule *radioModule = getModuleFromPar<cModule>(par("radioModule"), this);
        radioModule->subscribe(Ieee80211Radio::radioChannelChangedSignal, this);

        // start beacon timer (randomize startup time)
        beaconTimer = new cMessage("beaconTimer");

        //std::thread t1(&Ieee80211MgmtAP::threadFuncA, this);
        //std::thread t2(&Ieee80211MgmtAP::threadFuncB, this);
        //t1.join();
        //t2.join();

    }
    else if (stage == INITSTAGE_LINK_LAYER) {
        if (isOperational)
            scheduleAt(simTime() + uniform(0, beaconInterval), beaconTimer);
    }

    path = "results/random/poisson_1per_20sec/AC_03/singleSide_data_density_55_1200s_simu2.csv"; //simu1,simu2,..,simu5
    //path_AC0 = "results/xx_nodes_AC0.csv";
    //path_AC1 = "results/xx_nodes_AC1.csv";
    pathBrief = "results/random/poisson_1per_20sec/AC_03/singleSide_msgSent_density_55_1200s_simu2.csv"; //simu1,..,simu5

    if(fileAll.is_open()!=true){
        fileAll.open(path,std::ios::app);
    }
    /*
    if(file_AC1.is_open()!=true){
        file_AC1.open(path_AC1,std::ios::app);
    }
    if(file_AC0.is_open()!=true){
        file_AC0.open(path_AC0,std::ios::app);
    }
    */
}

void Ieee80211MgmtAP::handleTimer(cMessage *msg)
{
    if (msg == beaconTimer) {

        //sendBeacon(3); //for ac0 pass 0, for ac1 pass 1, ...
        //scheduleAt(simTime()+beaconInterval, beaconTimer);

        if (round == 1){

            simtime_t firstTrans = simTime();
            ////// only uncomment when ac0 transmitting //////
            sendBeacon(0);
            scheduleAt(firstTrans+beaconInterval, beaconTimer);
            //////////////////////////////////////////////////

            starter = int(sendStart)+1 - int(firstTrans.dbl());

            std::default_random_engine generator;
            std::poisson_distribution<int> distribution(3);

            for (int i=starter; i<starter+nrolls; i=i+int(period)){

                int number = distribution(generator);
                EV << "number: " << number << endl;
                totalpoisson = totalpoisson + number;
                if (number == 0){
                    continue;
                }
                if (number == 1){
                    list_ac3.push_back(firstTrans.dbl() + double(i) + period/2);
                    //list_ac2.push_back(firstTrans.dbl() + double(i) + period/2 + 0.030);
                    //list_ac3.push_back(firstTrans.dbl() + double(i) + period/2 - 0.030);
                    EV << firstTrans.dbl() + double(i) + period/2 << endl;
                    continue;
                }
                double gap = period/number;
                list_ac3.push_back(firstTrans.dbl() + double(i) + gap/2.0);
                //list_ac2.push_back(firstTrans.dbl() + double(i) + gap/2.0 + 0.030);
                //list_ac3.push_back(firstTrans.dbl() + double(i) + gap/2.0 - 0.030);
                double prev_elem = firstTrans.dbl() + double(i) + gap/2.0;
                EV << prev_elem << endl;
                for (int j=1; j<number; ++j){
                    list_ac3.push_back(prev_elem + gap);
                    //list_ac2.push_back(prev_elem + gap + 0.030);
                    //list_ac3.push_back(prev_elem + gap - 0.030);
                    prev_elem = prev_elem + gap;
                    EV << prev_elem << endl;
                }
            }
            EV << totalpoisson << endl;
            list_ac3.push_back(2711.0);
            //list_ac2.push_back(2711.0);
            //list_ac3.push_back(2711.0);

            /*//// uncomment for ac1 /////
            scheduleAt(list_ac1.front(), beaconTimer);
            list_ac1.pop_front();
            /////////////////////////////

            ///// uncomment for ac2 /////
            scheduleAt(list_ac2.front(), beaconTimer);
            list_ac2.pop_front();
            /////////////////////////////

            ///// uncomment for ac3 /////
            scheduleAt(list_ac3.front(), beaconTimer);
            list_ac3.pop_front();
            *////////////////////////////

        }else{
            /*//// uncomment for ac0 //////
            sendBeacon(0);
            scheduleAt(simTime()+beaconInterval, beaconTimer);
            //////////////////////////////

            ///// uncomment for ac1 /////
            sendBeacon(1);
            scheduleAt(list_ac1.front(), beaconTimer);
            list_ac1.pop_front();
            /////////////////////////////

            ///// uncomment for ac2 /////
            sendBeacon(2);
            scheduleAt(list_ac2.front(), beaconTimer);
            list_ac2.pop_front();
            //////////////////////////////

            ///// uncomment for ac3 /////
            sendBeacon(3);
            scheduleAt(list_ac3.front(), beaconTimer);
            list_ac3.pop_front();
            *//////////////////////////////


            if (whichClass == 0){
                sendBeacon(0);
                nextTime = simTime()+beaconInterval;
            //}else if(whichClass == 1){
            //    sendBeacon(1);
            //}else if(whichClass == 2){
            //    sendBeacon(2);
            }else{
                sendBeacon(3);
            }

            //double next = std::min({nextTime.dbl(), list_ac1.front(), list_ac2.front(), list_ac3.front()});
            double next = std::min({nextTime.dbl(), list_ac3.front()});
            if (next == nextTime.dbl()){
                whichClass = 0;
                scheduleAt(next, beaconTimer);
            //}else if(next == list_ac1.front()){
            //    whichClass = 1;
            //    scheduleAt(next, beaconTimer);
            //    list_ac1.pop_front();
            //}else if(next == list_ac2.front()){
            //   whichClass = 2;
            //    scheduleAt(next, beaconTimer);
            //    list_ac2.pop_front();
            }else{
                whichClass = 3;
                scheduleAt(next, beaconTimer);
                list_ac3.pop_front();
            }
            //scheduleAt(numSent*beaconInterval, beaconTimer);

        }
        round += 1;

    }
    else {
        throw cRuntimeError("internal error: unrecognized timer '%s'", msg->getName());
    }
}

void Ieee80211MgmtAP::handleUpperMessage(cPacket *msg)
{
    Ieee80211DataFrame *frame = encapsulate(msg);
    MACAddress macAddr = frame->getReceiverAddress();
    if (!macAddr.isMulticast()) {
        auto it = staList.find(macAddr);
        if (it == staList.end() || it->second.status != ASSOCIATED) {
            EV << "STA with MAC address " << macAddr << " not associated with this AP, dropping frame\n";
            delete frame;    // XXX count drops?
            return;
        }
    }

    sendDown(frame);
}

void Ieee80211MgmtAP::handleCommand(int msgkind, cObject *ctrl)
{
    throw cRuntimeError("handleCommand(): no commands supported");
}

void Ieee80211MgmtAP::receiveSignal(cComponent *source, simsignal_t signalID, intval_t value, cObject *details)
{
    Enter_Method_Silent();
    if (signalID == Ieee80211Radio::radioChannelChangedSignal) {
        EV << "updating channel number\n";
        channelNumber = value;
    }
}

Ieee80211MgmtAP::STAInfo *Ieee80211MgmtAP::lookupSenderSTA(Ieee80211ManagementFrame *frame)
{
    auto it = staList.find(frame->getTransmitterAddress());
    return it == staList.end() ? nullptr : &(it->second);
}

void Ieee80211MgmtAP::sendManagementFrame(Ieee80211ManagementFrame *frame, const MACAddress& destAddr)
{
    frame->setFromDS(true);
    frame->setReceiverAddress(destAddr);
    frame->setAddress3(myAddress);
    sendDown(frame);
}

void Ieee80211MgmtAP::sendBeacon(int accessClass)
{
    if (simTime() > fileWritingStart){

        Ieee80211BeaconFrame *frame = new Ieee80211BeaconFrame("Beacon");
        Ieee80211BeaconFrameBody& body = frame->getBody();

        cModule *host = getContainingNode(this);
        IMobility *mobility = check_and_cast<IMobility *>(host->getSubmodule("mobility"));
        Coord position = mobility->getCurrentPosition();
        Coord speed = mobility->getCurrentSpeed();

        frame->addPar("S_POSX")=position.x;
        frame->addPar("S_POSY")=position.y;
        frame->addPar("S_POSZ")=position.z;
        frame->addPar("S_SPD")=speed.x;
        //currentSpeed = double(frame->par("S_SPD"));

        body.setSSID(ssid.c_str());
        body.setSupportedRates(supportedRates);
        body.setBeaconInterval(beaconInterval);
        body.setChannelNumber(channelNumber);
        body.setBodyLength(8 + 2 + 2 + (2 + ssid.length()) + (2 + supportedRates.numRates)); // add 256 for Vendor Elements

        frame->setByteLength(28 + body.getBodyLength() + 256); /////////////changed 256 to 521
        frame->setReceiverAddress(MACAddress::BROADCAST_ADDRESS);
        frame->setFromDS(true);

        if (accessClass == 0){
            //if (simTime() > fileWritingStart){
            numSent++;
            //}
            frame->addPar("ID") = numSent;
            frame->addPar("currentAccessClass") = accessClass;
            EV << "numSent = " << numSent << endl;
            EV << "Generating AC0 beacon at " << simTime() << endl;
            sendDown(frame);

        }else if(accessClass == 1){
            //if (simTime() > fileWritingStart){
            numSentLC++;
            //}
            frame->addPar("ID") = numSentLC;
            frame->addPar("currentAccessClass") = accessClass;
            EV << "numSentLC = " << numSentLC << endl;
            EV << "Generating AC1 beacon at " << simTime() << endl;
            sendDown(frame);

        }else if(accessClass == 2){
            //if (simTime() > fileWritingStart){
            numSentSB++;
            //}
            frame->addPar("ID") = numSentSB;
            frame->addPar("currentAccessClass") = accessClass;
            EV << "numSentSB = " << numSentSB << endl;
            EV << "Generating AC2 beacon at " << simTime() << endl;
            sendDown(frame);

        }else{
            //if (simTime() > fileWritingStart){
            numSentEV++;
            //}
            frame->addPar("ID") = numSentEV;
            frame->addPar("currentAccessClass") = accessClass;
            EV << "numSentEV = " << numSentEV << endl;
            EV << "Generating AC3 beacon at " << simTime() << endl;
            sendDown(frame);
        }
    }


    /*
    /////////////////////// Emergency vehicle alert //////////////////////
    if (ssid == "SSID"){
        frame->addPar("ID") = numSentEV;
        frame->addPar("currentAccessClass") = 3;
        numSentEV++;
        EV << "Generating EV beacon at " << simTime() << endl;
        EV << "current speed " << currentSpeed << endl;
        sendDown(frame);
    //////////////////////////////////////////////////////////////////////
    }else{
    */
    /*//////////////////////////// Break alert ///////////////////////////
    EV << "current speed " << currentSpeed << endl;
    if (intervalCount != 0){
        EV << "previous speed " << preSpeed << endl;

        deceleration = (currentSpeed - preSpeed) * 100;
        EV << "current deceleration " << deceleration << endl;
    }

    if (deceleration < -4){
        frame->addPar("ID") = numSentSB;
        frame->addPar("currentAccessClass") = 2;
        numSentSB++;
        ifSBAlertSent = true;
        EV << "Generating Sudden brake alert at " << simTime() << endl;
        sendDown(frame);
    }
    /*////////////////////////////////////////////////////////////////////
    /*//////////////////////// Lane changing alert ///////////////////////
    if (intervalCount != 0){
        EV << "previous y position " << preYPosition << endl;
    }else{
        switch(int(frame->par("S_POSY"))){

        case 38:
        lane = false;
        EV << "lane is 0" << endl;
        break;

        case 34:
        lane = true;
        EV << "lane is 1" << endl;
        break;
        }
    }

    EV << "current y position " << double(frame->par("S_POSY")) << endl;

    if (double(frame->par("S_POSY")) != preYPosition and intervalCount != 0){
        if (double(frame->par("S_POSY")) < preYPosition and lane != true){
            lane = true;
            frame->addPar("currentLane") = 0;
            frame->addPar("targetLane") = 1;

        }else if(double(frame->par("S_POSY")) > preYPosition and lane != false){
            lane = false;
            frame->addPar("currentLane") = 1;
            frame->addPar("targetLane") = 0;
        }

        frame->addPar("ID") = numSentLC;
        frame->addPar("currentAccessClass") = 1;
        numSentLC++;
        ifLCAlertSent = true;
        EV << "Generating Lane changing alert at " << simTime() << endl;
        sendDown(frame);
    }
    ////////////////////////////////////////////////////////////////////*/
    //if (intervalCount % 10 == 0 and ifSBAlertSent == false){
    /*
    //if (intervalCount % 10 == 0){
    frame->addPar("ID") = numSent;
    frame->addPar("currentAccessClass") = 4;
    numSent++;
    EV << "numSent = " << numSent << endl;
    EV << "Generating Beacon at " << simTime() << endl;
    sendDown(frame);
    //}
    //preSpeed = currentSpeed;
    //preYPosition = double(frame->par("S_POSY"));
    //intervalCount++;
    //ifSBAlertSent = false;
    */
}

void Ieee80211MgmtAP::handleDataFrame(Ieee80211DataFrame *frame)
{
    // check toDS bit
    if (!frame->getToDS()) {
        // looks like this is not for us - discard
        EV << "Frame is not for us (toDS=false) -- discarding\n";
        delete frame;
        return;
    }

    // handle broadcast/multicast frames
    if (frame->getAddress3().isMulticast()) {
        EV << "Handling multicast frame\n";

        if (isConnectedToHL)
            sendToUpperLayer(frame->dup());

           return;
    }

    // look up destination address in our STA list
    auto it = staList.find(frame->getAddress3());
    if (it == staList.end()) {
        // not our STA -- pass up frame to relayUnit for LAN bridging if we have one
        if (isConnectedToHL) {
            sendToUpperLayer(frame);
        }
        else {
            EV << "Frame's destination address is not in our STA list -- dropping frame\n";
            delete frame;
        }
    }
    else {
        // dest address is our STA, but is it already associated?
        if (it->second.status == ASSOCIATED)
            distributeReceivedDataFrame(frame); // send it out to the destination STA
        else {
            EV << "Frame's destination STA is not in associated state -- dropping frame\n";
            delete frame;
        }
    }
}

void Ieee80211MgmtAP::handleAuthenticationFrame(Ieee80211AuthenticationFrame *frame)
{
    int frameAuthSeq = frame->getBody().getSequenceNumber();
    EV << "Processing Authentication frame, seqNum=" << frameAuthSeq << "\n";

    // create STA entry if needed
    STAInfo *sta = lookupSenderSTA(frame);
    if (!sta) {
        MACAddress staAddress = frame->getTransmitterAddress();
        sta = &staList[staAddress];    // this implicitly creates a new entry
        sta->address = staAddress;
        sta->status = NOT_AUTHENTICATED;
        sta->authSeqExpected = 1;
    }

    // reset authentication status, when starting a new auth sequence
    // The statements below are added because the L2 handover time was greater than before when
    // a STA wants to re-connect to an AP with which it was associated before. When the STA wants to
    // associate again with the previous AP, then since the AP is already having an entry of the STA
    // because of old association, and thus it is expecting an authentication frame number 3 but it
    // receives authentication frame number 1 from STA, which will cause the AP to return an Auth-Error
    // making the MN STA to start the handover process all over again.
    if (frameAuthSeq == 1) {
        if (sta->status == ASSOCIATED)
            sendDisAssocNotification(sta->address);
        sta->status = NOT_AUTHENTICATED;
        sta->authSeqExpected = 1;
    }

    // check authentication sequence number is OK
    if (frameAuthSeq != sta->authSeqExpected) {
        // wrong sequence number: send error and return
        EV << "Wrong sequence number, " << sta->authSeqExpected << " expected\n";
        Ieee80211AuthenticationFrame *resp = new Ieee80211AuthenticationFrame("Auth-ERROR");
        resp->getBody().setStatusCode(SC_AUTH_OUT_OF_SEQ);
        sendManagementFrame(resp, frame->getTransmitterAddress());
        delete frame;
        sta->authSeqExpected = 1;    // go back to start square
        return;
    }

    // station is authenticated if it made it through the required number of steps
    bool isLast = (frameAuthSeq + 1 == numAuthSteps);

    // send OK response (we don't model the cryptography part, just assume
    // successful authentication every time)
    EV << "Sending Authentication frame, seqNum=" << (frameAuthSeq + 1) << "\n";
    Ieee80211AuthenticationFrame *resp = new Ieee80211AuthenticationFrame(isLast ? "Auth-OK" : "Auth");
    resp->getBody().setSequenceNumber(frameAuthSeq + 1);
    resp->getBody().setStatusCode(SC_SUCCESSFUL);
    resp->getBody().setIsLast(isLast);
    // XXX frame length could be increased to account for challenge text length etc.
    sendManagementFrame(resp, frame->getTransmitterAddress());

    delete frame;

    // update status
    if (isLast) {
        if (sta->status == ASSOCIATED)
            sendDisAssocNotification(sta->address);
        sta->status = AUTHENTICATED;    // XXX only when ACK of this frame arrives
        EV << "STA authenticated\n";
    }
    else {
        sta->authSeqExpected += 2;
        EV << "Expecting Authentication frame " << sta->authSeqExpected << "\n";
    }
}

void Ieee80211MgmtAP::handleDeauthenticationFrame(Ieee80211DeauthenticationFrame *frame)
{
    EV << "Processing Deauthentication frame\n";

    STAInfo *sta = lookupSenderSTA(frame);
    delete frame;

    if (sta) {
        // mark STA as not authenticated; alternatively, it could also be removed from staList
        if (sta->status == ASSOCIATED)
            sendDisAssocNotification(sta->address);
        sta->status = NOT_AUTHENTICATED;
        sta->authSeqExpected = 1;
    }
}

void Ieee80211MgmtAP::handleAssociationRequestFrame(Ieee80211AssociationRequestFrame *frame)
{
    EV << "Processing AssociationRequest frame\n";

    // "11.3.2 AP association procedures"
    STAInfo *sta = lookupSenderSTA(frame);
    if (!sta || sta->status == NOT_AUTHENTICATED) {
        // STA not authenticated: send error and return
        Ieee80211DeauthenticationFrame *resp = new Ieee80211DeauthenticationFrame("Deauth");
        resp->getBody().setReasonCode(RC_NONAUTH_ASS_REQUEST);
        sendManagementFrame(resp, frame->getTransmitterAddress());
        delete frame;
        return;
    }

    delete frame;

    // mark STA as associated
    if (sta->status != ASSOCIATED)
        sendAssocNotification(sta->address);
    sta->status = ASSOCIATED;    // XXX this should only take place when MAC receives the ACK for the response

    // send OK response
    Ieee80211AssociationResponseFrame *resp = new Ieee80211AssociationResponseFrame("AssocResp-OK");
    Ieee80211AssociationResponseFrameBody& body = resp->getBody();
    body.setStatusCode(SC_SUCCESSFUL);
    body.setAid(0);    //XXX
    body.setSupportedRates(supportedRates);
    sendManagementFrame(resp, sta->address);
}

void Ieee80211MgmtAP::handleAssociationResponseFrame(Ieee80211AssociationResponseFrame *frame)
{
    dropManagementFrame(frame);
}

void Ieee80211MgmtAP::handleReassociationRequestFrame(Ieee80211ReassociationRequestFrame *frame)
{
    EV << "Processing ReassociationRequest frame\n";

    // "11.3.4 AP reassociation procedures" -- almost the same as AssociationRequest processing
    STAInfo *sta = lookupSenderSTA(frame);
    if (!sta || sta->status == NOT_AUTHENTICATED) {
        // STA not authenticated: send error and return
        Ieee80211DeauthenticationFrame *resp = new Ieee80211DeauthenticationFrame("Deauth");
        resp->getBody().setReasonCode(RC_NONAUTH_ASS_REQUEST);
        sendManagementFrame(resp, frame->getTransmitterAddress());
        delete frame;
        return;
    }

    delete frame;

    // mark STA as associated
    sta->status = ASSOCIATED;    // XXX this should only take place when MAC receives the ACK for the response

    // send OK response
    Ieee80211ReassociationResponseFrame *resp = new Ieee80211ReassociationResponseFrame("ReassocResp-OK");
    Ieee80211ReassociationResponseFrameBody& body = resp->getBody();
    body.setStatusCode(SC_SUCCESSFUL);
    body.setAid(0);    //XXX
    body.setSupportedRates(supportedRates);
    sendManagementFrame(resp, sta->address);
}

void Ieee80211MgmtAP::handleReassociationResponseFrame(Ieee80211ReassociationResponseFrame *frame)
{
    dropManagementFrame(frame);
}

void Ieee80211MgmtAP::handleDisassociationFrame(Ieee80211DisassociationFrame *frame)
{
    STAInfo *sta = lookupSenderSTA(frame);
    delete frame;

    if (sta) {
        if (sta->status == ASSOCIATED)
            sendDisAssocNotification(sta->address);
        sta->status = AUTHENTICATED;
    }
}

void Ieee80211MgmtAP::handleBeaconFrame(Ieee80211BeaconFrame *frame)
{
    if (frame->getCreationTime() > fileWritingStart){
        packetCount+=1;

        simtime_t eed=frame->getArrivalTime()-frame->getCreationTime();
        endToEndDelay+=eed;

        Ieee80211BeaconFrameBody& body = frame->getBody();
        const char *s = body.getSSID();
        std::string frame_ssid(s);

        numReceived++;
        std::string data0;//= std::to_string(numReceived);
        data0 = data0 + (std::to_string(int(frame->par("ID")))); // Packet id
        data0 = data0 + "," + (std::to_string(int(frame->getByteLength())));// Packet size
        data0 = data0 + "," + (frame_ssid);// Sender ssid
        data0 = data0 + "," + (ssid);// Receiver ssid
        data0 = data0 + "," + (std::to_string(double(frame->par("RSSID")))); // RSSI
        data0 = data0 + "," + (frame->getArrivalTime().str()); // Arrival Time
        data0 = data0 + "," + (eed.str()); // End to End Delay
        data0 = data0 + "," + (std::to_string(double(frame->par("S_POSX"))));
        data0 = data0 + "," + (std::to_string(double(frame->par("S_POSY"))));
        data0 = data0 + "," + (std::to_string(double(frame->par("S_SPD")))); // Sender speed

        cModule *receiver = getContainingNode(this);
        IMobility *mobility = check_and_cast<IMobility *>(receiver->getSubmodule("mobility"));
        Coord receiverPosition = mobility->getCurrentPosition();
        Coord receiverSpeed = mobility->getCurrentSpeed();

        data0 = data0 + "," + (std::to_string(double(receiverPosition.x)));
        data0 = data0 + "," + (std::to_string(double(receiverPosition.y)));
        data0 = data0 + "," + (std::to_string(double(receiverSpeed.x)));
        data0 = data0 + "," + (std::to_string(double(frame->par("WaitInerval"))/1000000));
        data0 = data0 + "," + (std::to_string(int(frame->par("collision_number"))));
        data0 = data0 + "," + (std::to_string(int(frame->par("currentAccessClass")))); //access class

        if ( int(frame->par("currentAccessClass")) == 0 ){
            if ( int(frame->par("ID")) < 4001 ){
                lastReceived = int(frame->par("ID"));
                fileAll << data0 << endl;
            }
        }else{
            fileAll << data0 << endl;
        }

    }

    if (int(frame->par("currentAccessClass")) == 1){
        EV << "Lane change alert" << endl;
    }else if(int(frame->par("currentAccessClass")) == 2){
        EV << "Sudden break alert" << endl;
    }else if(int(frame->par("currentAccessClass")) == 3){
        EV << "Emergency vehicle alert" << endl;
    }

//    if(ssid!="SSID" and frame_ssid!="SSID"){
//        fileAll<<data<<endl;
//    }

//    if(ssid=="node_00"){
//        fileAll<<data<<endl;
//    }


    dropManagementFrame(frame);
}

void Ieee80211MgmtAP::handleProbeRequestFrame(Ieee80211ProbeRequestFrame *frame)
{
    EV << "Processing ProbeRequest frame\n";

    if (strcmp(frame->getBody().getSSID(), "") != 0 && strcmp(frame->getBody().getSSID(), ssid.c_str()) != 0) {
        EV << "SSID `" << frame->getBody().getSSID() << "' does not match, ignoring frame\n";
        dropManagementFrame(frame);
        return;
    }

    MACAddress staAddress = frame->getTransmitterAddress();
    delete frame;

    EV << "Sending ProbeResponse frame\n";
    Ieee80211ProbeResponseFrame *resp = new Ieee80211ProbeResponseFrame("ProbeResp");
    Ieee80211ProbeResponseFrameBody& body = resp->getBody();
    body.setSSID(ssid.c_str());
    body.setSupportedRates(supportedRates);
    body.setBeaconInterval(beaconInterval);
    body.setChannelNumber(channelNumber);
    sendManagementFrame(resp, staAddress);
}

void Ieee80211MgmtAP::handleProbeResponseFrame(Ieee80211ProbeResponseFrame *frame)
{
    dropManagementFrame(frame);
}

void Ieee80211MgmtAP::sendAssocNotification(const MACAddress& addr)
{
    NotificationInfoSta notif;
    notif.setApAddress(myAddress);
    notif.setStaAddress(addr);
    emit(NF_L2_AP_ASSOCIATED, &notif);
}

void Ieee80211MgmtAP::sendDisAssocNotification(const MACAddress& addr)
{
    NotificationInfoSta notif;
    notif.setApAddress(myAddress);
    notif.setStaAddress(addr);
    emit(NF_L2_AP_DISASSOCIATED, &notif);
}

void Ieee80211MgmtAP::start()
{
    Ieee80211MgmtAPBase::start();
    scheduleAt(simTime() + uniform(0, beaconInterval), beaconTimer);
}///

void Ieee80211MgmtAP::stop()
{
    cancelEvent(beaconTimer);
    staList.clear();
    Ieee80211MgmtAPBase::stop();
    EV<<"**************************Now I am in stop method**************"<<endl;
}

void Ieee80211MgmtAP::finish()
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
    fileBrief<<ssid+","+std::to_string(lastReceived)+","+std::to_string(numSentLC)+","+std::to_string(numSentSB)+","+std::to_string(numSentEV)<<endl;
    fileBrief.close();

}

} // namespace ieee80211

} // namespace inet

