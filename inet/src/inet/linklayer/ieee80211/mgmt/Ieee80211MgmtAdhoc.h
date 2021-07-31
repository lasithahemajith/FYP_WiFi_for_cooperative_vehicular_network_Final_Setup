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

#ifndef __INET_IEEE80211MGMTADHOC_H
#define __INET_IEEE80211MGMTADHOC_H

#include "inet/common/INETDefs.h"

//#include "inet/linklayer/ieee80211/mgmt/Ieee80211MgmtBase.h"
#include "inet/linklayer/ieee80211/mgmt/Ieee80211MgmtAPBase.h"
#include <iostream>
#include <fstream>

namespace inet {

namespace ieee80211 {

/**
 * Used in 802.11 ad-hoc mode. See corresponding NED file for a detailed description.
 * This implementation ignores many details.
 *
 * @author Andras Varga
 */
class INET_API Ieee80211MgmtAdhoc : public Ieee80211MgmtBase
{

   public:
    enum STAStatus { NOT_AUTHENTICATED, AUTHENTICATED, ASSOCIATED };
   std ::fstream myfile;
   std ::string path;
   std ::fstream fileAll;
   std ::string pathAll;
   std ::fstream fileBrief;
   std ::string pathBrief;
   simtime_t endToEndDelay;
   int packetCount;
   int numSent;
   int numReceived;
   simtime_t preTime;
   simtime_t PIRTotal;
   simtime_t MAX_delay;
   simtime_t MIN_delay;
   simtime_t MAX_PIR;
   simtime_t MIN_PIR;
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
   std::string ssid;
   int channelNumber = -1;
   simtime_t beaconInterval;
   int numAuthSteps = 0;
   Ieee80211SupportedRatesElement supportedRates;

   // state
   STAList staList;    ///< list of STAs
   cMessage *beaconTimer = nullptr;

    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int) override;

    /** Implements abstract Ieee80211MgmtBase method */
    virtual void handleTimer(cMessage *msg) override;

    /** Implements abstract Ieee80211MgmtBase method */
    virtual void handleUpperMessage(cPacket *msg) override;

    /** Implements abstract Ieee80211MgmtBase method -- throws an error (no commands supported) */
    virtual void handleCommand(int msgkind, cObject *ctrl) override;

    /** Utility function for handleUpperMessage() */
    virtual Ieee80211DataFrame *encapsulate(cPacket *msg);

    /** Utility method to decapsulate a data frame */
    virtual cPacket *decapsulate(Ieee80211DataFrame *frame);

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    /** Utility function: creates and sends a beacon frame */


    ////////////////////////////////////////////////////////////////////////////////////////////////////

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
};

} // namespace ieee80211

} // namespace inet

#endif // ifndef __INET_IEEE80211MGMTADHOC_H

