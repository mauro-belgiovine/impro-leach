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

#include "BS.h"

Define_Module(BS);

void BS::initialize()
{

    N = getParentModule()->par("Nnodes");

    double edge = getParentModule()->par("edge");
    range = sqrt(2*pow(edge,2));

    bitrate = par("bitrate");

    startRound_e = new cMessage("start-round", START_ROUND);
    rcvdJoin_e = new cMessage("check-JOIN-or-DATA", RCVD_JOIN);
    // let BS set the restart round time for all the network
    getParentModule()->par("roundTime") = 1 + (N * propagationDelay(DATA_M_SIZE, MAX_DIST(range)));

    scheduleAt(0,startRound_e);


}

void BS::handleMessage(cMessage *msg)
{
    unsigned int Ndead = getParentModule()->par("Ndead");

    if(Ndead < N)
    {
        switch(msg->getKind())
        {

            case START_ROUND:
                // start a new round in LEACH

                r = par("round"); // NOTE: par("round") starts at -1
                r++;
                par("round") = r;
                if (r == 0) roundTime = getParentModule()->par("roundTime");
                getParentModule()->par("round") = r; // let only BS node update also the net parameter
                msgBuf.clear();
                cancelEvent(rcvdJoin_e);
                // schedule the next round after roundTime
                scheduleAt(simTime()+roundTime,startRound_e);
                break;

            case RCVD_JOIN:
                // wake up after timeout to check received ADVs
                if(msgBuf.size() > 0)
                    createTXSched();
                break;
            case JOIN_M:
                if (msgBuf.size() == 0)
                {
                    //if it's the first JOIN received, let's compute the schedule
                    msgBuf.push_back(msg);
                    if(!rcvdJoin_e->isScheduled())
                        scheduleAt(simTime()+EPSILON, rcvdJoin_e); //EPSILON needed for messages arrived at the same time
                }
                else
                {
                    // each subsequent JOIN will be handled at the next Schedule creation
                    // (i.e. createTXSched()) in order to increase the schedule
                    msgBuf.push_back(msg);
                }
                break;

            case DATA_M:
                // simply add DATA received to buffer,
                //since they are going to serve as JOIN messages to create the new schedule
                handleData(msg);
                break;
        }
    }
}

void BS::handleData(cMessage *msg)
{
    mData *DATA = (mData *) msg;
    if (r == DATA->getRound()){
        msgBuf.push_back(msg); // insert DATA into the message buffer
        EV << "received data from " << msg->getSenderModuleId() - 2 << "\n";
    }
}

void BS::createTXSched()
{
    clusterN = msgBuf.size();

    // in order to adjust power of transmission, first keep track of the max_distance of nodes among the ones in the cluster
    sensor_max_dist = MAX_DIST(range);

    // each node has to be assigned a temporal slot, based on msg DATA size they send and max propagation delay in the cluster
    double slot = propagationDelay(DATA_M_SIZE, sensor_max_dist);
    double SCHED_delay = propagationDelay(SCHED_M_SIZE, sensor_max_dist);

    // now send their SCHED information (i.e. their turn to transmit)
    for(unsigned int i = 0; i < msgBuf.size(); i++){
        mJoin *JOIN = (mJoin *) msgBuf.at(i);
        mSchedule *SCHED = (mSchedule *) new mSchedule("schedule-info", SCHED_M);
        SCHED->setTurn(i);
        SCHED->setDuration(slot);
        SCHED->setRound(par("round"));
        SCHED->setCHId(BS_ID);
        cModule *sensor = retrieveNode(JOIN->getId());
        EV << "sending schedule to " << JOIN->getId() << "\n";
        sendDirect(SCHED, SCHED_delay, 0, sensor->gate("in"));
        cancelAndDelete(JOIN);
    }

    msgBuf.clear(); // empty buffer



#ifndef ONE_TX_PER_ROUND
    double IDLE_duration = clusterN*slot;
    // setup a timer to keep radio in IDLE mode and receive all data (TDMA)
    // Timeout will take in account the propagation delay for SCHED msg to reach destination and to receive back all data sequentially
    scheduleAt(simTime() + SCHED_delay + IDLE_duration + EPSILON, rcvdJoin_e);
    // after all data are collected, simply recompute the schedule based on new JOIN/DATA received
#endif
}

void BS::finish(){
    recordScalar("endTime", simTime());
    recordScalar("rounds", r);
}

/********* Utilities ************/
cModule* BS::retrieveNode(unsigned int n)
{
   char modName[32];
   sprintf(modName,"node[%d]", n);
   return (cModule *) getModuleByPath(modName);
}


double BS::propagationDelay(unsigned int msg_size, double dist)
{
    // Compute the propagation delay based on packet size and distance
    // that is the time when the last bit of message is received
    double Dp = msg_size / bitrate; // packet duration
    return dist/C + Dp;  // propagation delay
}

void BS::broadcast(cMessage *msg, double delay){
    for(unsigned int n = 0; n < N; n++){
        cModule * sensor = retrieveNode(n);
        sendDirect(msg->dup(), delay, 0,  sensor->gate("in"));
    }
}


