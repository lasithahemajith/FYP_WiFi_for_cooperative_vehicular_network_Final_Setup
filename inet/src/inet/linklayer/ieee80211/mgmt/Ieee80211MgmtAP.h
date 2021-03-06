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

#ifndef __INET_IEEE80211MGMTAP_H
#define __INET_IEEE80211MGMTAP_H

#include <map>

#include "inet/common/INETDefs.h"

#include "inet/linklayer/ieee80211/mgmt/Ieee80211MgmtAPBase.h"

#include <iostream>
#include <fstream>

namespace inet {

namespace ieee80211 {

using namespace std;

/**
 * Used in 802.11 infrastructure mode: handles management frames for
 * an access point (AP). See corresponding NED file for a detailed description.
 *
 * @author Andras Varga
 */
class INET_API Ieee80211MgmtAP : public Ieee80211MgmtAPBase, protected cListener
{
  public:
    /** State of a STA */
    enum STAStatus { NOT_AUTHENTICATED, AUTHENTICATED, ASSOCIATED };

    std ::fstream myfile;
    std ::string path;
    std ::fstream fileAll;
    std ::string pathAll;
    std ::fstream fileBrief;
    std ::string pathBrief;

    //std ::fstream file_AC0;
    //std ::string path_AC0;
    //std ::fstream file_AC1;
    //std ::string path_AC1;

    //cLongHistogram DELAY_BEACON_HIST; // plot the diagrams
    //cOutVector DELAY_BEACON_VECTOR; // plot the diagrams
    simtime_t endToEndDelay;
    int packetCount;
    int numSent;
    int lastReceived;
    int numSentLC = 0;
    int numSentSB = 0;
    int numSentEV = 0;
    int numReceived;
    int intervalCount;
    double preYPosition;
    double preSpeed;
    double currentSpeed;
    double deceleration;
    bool lane;
    bool ifLCAlertSent = false;
    bool ifSBAlertSent = false;
    int whichClass = 0;
    int totalpoisson = 0;
    simtime_t nextTime;
    simtime_t preTime;
    simtime_t PIRTotal;
    simtime_t MAX_delay;
    simtime_t MIN_delay;
    simtime_t MAX_PIR;
    simtime_t MIN_PIR;

    simtime_t fileWritingStart = 62.44; /////////////////
    double sendStart = 62.44; ///////////// sendStart=fileWritingStart
    int starter;
    //int turn = 0; //which ac's turn

    int round = 1;
    double period = 20.0; // 1.0 or 2.0 or 3.0 or 4.0 or 5.0 (seconds)
    //int nrolls = 1020; // beacon transmitting time (if period=1.0 nrolls=1020, if period=2.0 nrolls=2040, if period=3.0 nrolls=3060, ...)
    int nrolls = 1220; //nrolls = beacon transmitting time + period
    std::list <double> list_ac1;
    std::list <double> list_ac2;
    std::list <double> list_ac3;

    std :: vector <std::string> receivers;
    /** Describes a STA */
    struct STAInfo
    {
        MACAddress address;
        STAStatus status;
        int authSeqExpected;    // when NOT_AUTHENTICATED: transaction sequence number of next expected auth frame
        //int consecFailedTrans;  //XXX
        //double expiry;          //XXX association should expire after a while if STA is silent?
    };

    class NotificationInfoSta : public cObject
    {
        MACAddress apAddress;
        MACAddress staAddress;

      public:
        void setApAddress(const MACAddress& a) { apAddress = a; }
        void setStaAddress(const MACAddress& a) { staAddress = a; }
        const MACAddress& getApAddress() const { return apAddress; }
        const MACAddress& getStaAddress() const { return staAddress; }
    };

    struct MAC_compare
    {
        bool operator()(const MACAddress& u1, const MACAddress& u2) const { return u1.compareTo(u2) < 0; }
    };
    typedef std::map<MACAddress, STAInfo, MAC_compare> STAList;

  protected:
    // configuration
    std::string ssid;
    int channelNumber = -1;
    simtime_t beaconInterval;
    int numAuthSteps = 0;
    Ieee80211SupportedRatesElement supportedRates;

    // state
    STAList staList;    ///< list of STAs
    cMessage *beaconTimer = nullptr;

  public:
    Ieee80211MgmtAP() {}
    virtual ~Ieee80211MgmtAP();

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int) override;

    /** Implements abstract Ieee80211MgmtBase method */
    virtual void handleTimer(cMessage *msg) override;

    /** Implements abstract Ieee80211MgmtBase method */
    virtual void handleUpperMessage(cPacket *msg) override;

    /** Implements abstract Ieee80211MgmtBase method -- throws an error (no commands supported) */
    virtual void handleCommand(int msgkind, cObject *ctrl) override;

    /** Called by the signal handler whenever a change occurs we're interested in */
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, intval_t value, cObject *details) override;
    /** Utility function: return sender STA's entry from our STA list, or nullptr if not in there */
    virtual STAInfo *lookupSenderSTA(Ieee80211ManagementFrame *frame);

    /** Utility function: set fields in the given frame and send it out to the address */
    virtual void sendManagementFrame(Ieee80211ManagementFrame *frame, const MACAddress& destAddr);

    /** Utility function: creates and sends a beacon frame */
    virtual void sendBeacon(int ac);

    /** @name Processing of different frame types */
    //@{
    virtual void handleDataFrame(Ieee80211DataFrame *frame) override;
    virtual void handleAuthenticationFrame(Ieee80211AuthenticationFrame *frame) override;
    virtual void handleDeauthenticationFrame(Ieee80211DeauthenticationFrame *frame) override;
    virtual void handleAssociationRequestFrame(Ieee80211AssociationRequestFrame *frame) override;
    virtual void handleAssociationResponseFrame(Ieee80211AssociationResponseFrame *frame) override;
    virtual void handleReassociationRequestFrame(Ieee80211ReassociationRequestFrame *frame) override;
    virtual void handleReassociationResponseFrame(Ieee80211ReassociationResponseFrame *frame) override;
    virtual void handleDisassociationFrame(Ieee80211DisassociationFrame *frame) override;
    virtual void handleBeaconFrame(Ieee80211BeaconFrame *frame) override;
    virtual void handleProbeRequestFrame(Ieee80211ProbeRequestFrame *frame) override;
    virtual void handleProbeResponseFrame(Ieee80211ProbeResponseFrame *frame) override;

    virtual void finish() override;
    //@}

    void sendAssocNotification(const MACAddress& addr);

    void sendDisAssocNotification(const MACAddress& addr);

    void addreceivers(int num_rec){
        std:: string rec;
        for(int i=1;i<=num_rec;i++){
            rec="receiver_"+std:: to_string(i);
            receivers.push_back(rec);
        }
    }
    /*
    void threadFuncA()
    {
        for (int i=1; i<5; i++){
            EV << "Thread A is executing" << endl;
            //std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    void threadFuncB()
    {
        for (int i=1; i<5; i++){
            EV << "Thread B is executing" << endl;
            //std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    */

    /** lifecycle support */
    //@{

  protected:
    virtual void start() override;
    virtual void stop() override;
    //@}
};

} // namespace ieee80211

} // namespace inet

#endif // ifndef __INET_IEEE80211MGMTAP_H

