/*
 * dataCollector.cc
 *
 *  Created on: Jul 3, 2020
 *      Author: HP
 */


#include <string.h>
#include <omnetpp.h>

using namespace omnetpp;


class stats : public cSimpleModule
{
    protected:
     // The following redefined virtual function holds the algorithm.
     virtual void initialize() override;
     virtual void handleMessage(cMessage *msg) override;

};

Define_Module(stats);

void stats::initialize(){




}






void stats::handleMessage(cMessage *msg)
{
    simtime_t eed = simTime() - msg->getCreationTime();
    EV << "eed : " << eed << endl;
}




























