//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef __IMPRO_LEACH_SENSOR_H_
#define __IMPRO_LEACH_SENSOR_H_
#include <limits>
#include <algorithm>
#include <omnetpp.h>
#include "common.h"

using namespace omnetpp;


class Sensor : public cSimpleModule
{
  private:
    unsigned int id;        // sensor id
    unsigned int N;         // nodes in the network
    int x,y;                // coordinates of sensor (m)
    bool alreadyCH = false; // indicates whether the node has elected himself a CH or not in the current round
    double P;               // proportion of CH in the current network

    int CH_id = -1;         // Cluster-Head id
    double CH_dist;         // Cluster-Head distance
    double TXslot;
    double TXturn;

    double sensor_max_dist; // used by CH to adjust power of transmission
    unsigned int clusterN;  // used by CH to keep track of the num. of nodes in the cluster
    nodeRole role = SENSOR;
    double roundTime;

    cModule *BS;

    double C = LIGHTSPEED;
    double bitrate;   // bitrate of sensors
    double range;        // it will be the max communication range of sensors

    double Eelec, Eamp, Ecomp, gamma;  // energy parameters
    double energy;              // initial battery energy

    std::vector<cMessage *> msgBuf;

    cMessage *startRound_e;
    cMessage *startTX_e;    // event used to start DATA TX from sensor nodes
    cMessage *rcvdADV_e;    // event used to wake up and check ADV msgs from CH
    cMessage *rcvdSCHED_e;  // event used to check if we receive a new SCHEDule (i.e. the CH is alive)
    cMessage *rcvdJoin_e;   // event used to wake up and check JOIN msgs from sensor nodes
    cMessage *rcvdData_e;   // event used to wake up and check DATA msgs from sensor nodes

    // TODO when the CH dies, setup a timeout to  get the next SCHED event. If not received, start transmitting to the base.
    // (not needed if we perform only one transmission per round)
    // TODO when sending message considering a certain range, we should not allow message to reach
    // destination when are further from maximum distance
    // by grouping everything in a function we can also compute the delay based on distance
    // (now we don't need it since area is designed to let all sensors talk to each other)

    simsignal_t energySignal;

  protected:
    virtual void initialize();
    virtual void finish();
    virtual void reset();
    virtual void handleMessage(cMessage *msg);

    virtual cModule* retrieveNode(unsigned int n);
    virtual double propagationDelay(unsigned int msg_size, double dist);
    virtual double distance(unsigned int id);
    virtual double distance2s(unsigned int id1, unsigned int id2);
    virtual double T(unsigned int n);
    virtual void advertisementPhase();
    virtual void selfElection();
    virtual void broadcastADV();
    virtual void chooseCH();
    virtual void createTXSched();
    virtual void setupDataTX(mSchedule *SCHED);
    virtual void sendData();
    virtual void initOrphan();
    virtual void compressAndSendToBS();
    virtual void handleData(cMessage *msg);
    virtual double EnergyTX(unsigned int k, double d);
    virtual double EnergyRX(unsigned int k);
    virtual double EnergyCompress(unsigned int kN);
    virtual void EnergyMgmt(compState state, double d, unsigned int k);


  public:
    virtual double getEnergy();
};



#endif
