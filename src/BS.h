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

#ifndef __IMPRO_LEACH_BS_H_
#define __IMPRO_LEACH_BS_H_

#include <omnetpp.h>
#include "common.h"

using namespace omnetpp;

/**
 * Base Station class
 */
class BS : public cSimpleModule
{
  private:

    unsigned int N;         // nodes in the network
    int x,y;                // coordinates of sensor (m)
    double roundTime;
    unsigned int r;
    double C = LIGHTSPEED;
    double bitrate;   // bitrate of sensors
    double range;        // it will be the max communication range of sensors
    unsigned int clusterN;  // used by BD to keep track of the num. of nodes in the cluster
    double sensor_max_dist; // used by CH to adjust power of transmission

    cMessage *startRound_e;
    cMessage *rcvdJoin_e;   // event used to wake up and check JOIN msgs from sensor nodes


    std::vector<cMessage *> msgBuf;


  protected:
    virtual void initialize();
    virtual void finish();
    virtual void handleMessage(cMessage *msg);
    virtual cModule* retrieveNode(unsigned int n);
    virtual double propagationDelay(unsigned int msg_size, double dist);
    virtual void broadcast(cMessage *msg, double delay);
    virtual void createTXSched();
    virtual void handleData(cMessage *msg);

};

#endif
