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

#include "sensor.h"

Define_Module(Sensor);

void Sensor::initialize()
{
    // init parameters
    P = getParentModule()->par("P");
    id = this->getIndex(); // return the index of current module
    N = getParentModule()->par("Nnodes");

    double edge = getParentModule()->par("edge");
    range = sqrt(2*pow(edge,2));

    bitrate = par("bitrate");

    Eelec = this->par("Eelec");
    Eamp = this->par("Eamp");
    Ecomp = this->par("Ecomp");
    gamma = this->par("gamma");

    energy = this->par("energy");
    WATCH(energy);

    BS = getParentModule()->getSubmodule("baseStation");

    // setup internal events
    startRound_e = new cMessage("start-round", START_ROUND);
    rcvdADV_e = new cMessage("received-ADV", RCVD_ADV);
    rcvdSCHED_e = new cMessage("check-SCHED", RCVD_SCHED);
    rcvdJoin_e = new cMessage("check-JOIN-or-DATA", RCVD_JOIN);
    rcvdData_e = new cMessage("received-DATA", RCVD_DATA);
    startTX_e = new cMessage("startTX", START_TX);


    // Setup position
    bool noRepeatPos = true;
    do{
        noRepeatPos = true;
        x = intuniform(getParentModule()->par("minX"), getParentModule()->par("edge"));
        y = intuniform(getParentModule()->par("minY"), getParentModule()->par("edge"));
        // check that no other nodes has the same coordinates
        for(unsigned int n = 0; n < N; n++){
           cModule * mod = retrieveNode(n);
           unsigned int modx = mod->par("posX");
           unsigned int mody = mod->par("posY");
           if((modx == x) && (mody == y)){
               noRepeatPos = false;
           }
        }
    }while((!noRepeatPos));
    // update parameters
    this->par("posX") = x;
    this->par("posY") = y;

    energySignal = registerSignal("energy");

    scheduleAt(0,startRound_e);
}

void Sensor::finish()
{
    cancelAndDelete(startRound_e);
    cancelAndDelete(rcvdADV_e);
    cancelAndDelete(rcvdJoin_e);
    cancelAndDelete(rcvdData_e);
    cancelAndDelete(startTX_e);
}

void Sensor::reset()
{
    getDisplayString().setTagArg("i", 0, "old/ball"); // UI feedback
    role = SENSOR;
    msgBuf.clear();
    CH_id = -1;         // Cluster-Head id
    clusterN = 0;  // used by CH to keep track of the num. of nodes in the cluster
    cancelEvent(rcvdADV_e);
    cancelEvent(rcvdJoin_e);
    cancelEvent(rcvdData_e);
    cancelEvent(startTX_e);

}

void Sensor::handleMessage(cMessage *msg)
{
    if(role != DEAD) // if the node is still alive, react to messages, otherwise just drop them
    {
        switch(msg->getKind())
        {
            case START_ROUND:
                // start a new round in LEACH
                selfElection(); // new election
                // schedule the next round after roundTime
                scheduleAt(simTime()+roundTime,startRound_e);
                break;

            /******** Non-CH cases *********/
            case ADV_M:
                if(role == SENSOR)
                    msgBuf.push_back(msg); // insert ADV into the message buffer
                break;

            case RCVD_ADV:
                // wake up after timeout to check received ADVs
                chooseCH();
                break;

            case SCHED_M:
                // setup data send interval
                setupDataTX((mSchedule *) msg);
                break;

            case START_TX:
                sendData();
                break;

            /******** CH cases *********/
            case JOIN_M:
                msgBuf.push_back(msg); // insert JOIN into the message buffer
                break;

            case RCVD_JOIN:
                // wake up after timeout to check received ADVs
                if(msgBuf.size() > 0)
                    createTXSched();
                else{

                    // if no JOIN/DATA has been received (i.e. no one joined or all nodes in the cluster died)
                    // just act as a normal node (i.e. Orphan)
                    reset();
                    //scheduleAt(simTime(), startTX_e);
                    initOrphan();
                }
                break;

            case DATA_M:
                handleData(msg);
                break;

            case RCVD_DATA:
                compressAndSendToBS();
                break;

            /********** alternative CH **************/

            case CENTER_M:
                //setup CH environment variables
                alreadyCH = true;   // node excludes itself from next election
                role = CH;
                clusterN = ((mCenterCH *) msg)->getClusterN();
                getDisplayString().setTagArg("i", 0, "old/ball2"); // UI feedback
                // setup a timer to keep radio in IDLE mode and receive all data (TDMA)
                // Timeout will take in account the propagation delay for SCHED msg to reach destination and to receive back all data sequentially
                scheduleAt(simTime() + (((mCenterCH *) msg)->getSCHEDDelay()) + (((mCenterCH *) msg)->getIDLETime()) + EPSILON, rcvdData_e);
                #ifdef ACCOUNT_CH_SETUP
                // account for energy during IDLE time
                EnergyMgmt(RX, 0, clusterN*DATA_M_SIZE);
                #endif
                break;



        }
    }
    else
    {
        // node is dead
        if(!msg->isSelfMessage()){
            // drop all the msg from other modules
            cancelAndDelete(msg);
        }
    }

}

/******************* SENSOR functions **********************/
double Sensor::T(unsigned int n)    // T(n) threshold function
{
    unsigned int r = par("round"); // get current round

    if(!alreadyCH)
        return P/(1-P*(r % 1/P));
    else
        return 0;
}

void Sensor::selfElection()
{

    unsigned int r = par("round"); // NOTE: par("round") starts at -1
    par("round") = r+1;
    if ((r+1) == 0) roundTime = getParentModule()->par("roundTime");
    if(r+1 > 0) reset(); //reset all the structures before starting new round

    r = par("round");
    if((r % 1/P) == 0) alreadyCH = false; // reset current node status

    //compute Threshold function
    double th = T(id);
    double chance = uniform(0,1);
    if (chance < th)
    {
        // self-elected as Cluster-Head (CH)
        //proceed to Advertisement Phase
        EV << "I am Cluster-Head!\n";
        advertisementPhase();
    }
    else
    {
        // not CH.
        // start waiting for ADVs (consider max distance for timeout)
        scheduleAt(simTime() + propagationDelay(ADV_M_SIZE, MAX_DIST(range))+EPSILON, rcvdADV_e);
#ifdef ACCOUNT_CH_SETUP
        // add ENERGY CONSUMPTION FOR THE AMOUNT OF TIME WE ARE IN IDLE STATE
        EnergyMgmt(RX, 0, ADV_M_SIZE);
#endif
    }
}

void Sensor::chooseCH()
{
    CH_dist = std::numeric_limits<double>::infinity();
    CH_id = -1;

    for(unsigned int i = 0; i < msgBuf.size(); i++){
        mAdvertisement *ADV = (mAdvertisement *) msgBuf.at(i);
        // check distance of sender
        // use euclidean distance to simulate RSSI
        double dist = distance(ADV->getId());
        if(dist < CH_dist){
            CH_dist = dist;
            CH_id = ADV->getId(); // select CH based on distance/RSSI
        }
        EV << "ADV received from " << ADV->getId() << " distance is " << dist << "\n";
        cancelAndDelete(ADV);
    }

    if(CH_id > -1){
        // CH has been chosen
        EV << "CH designed is " << CH_id << "\n";
        msgBuf.clear(); // empty the msg buffer

        double delay = propagationDelay(JOIN_M_SIZE, CH_dist);
        // notify CH
        mJoin *JOIN = new mJoin("join-cluster", JOIN_M);
        JOIN->setId(id);
        cModule *CH = retrieveNode(CH_id);
        sendDirect(JOIN, delay, 0, CH->gate("in"));
#ifdef ACCOUNT_CH_SETUP
        // account for energy transmission based on distance
        EnergyMgmt(TX, CH_dist, JOIN_M_SIZE);
#endif

    } else {

        EV << "[ORPHAN NODE] No ADV has been received. \n";

        initOrphan();
        //scheduleAt(simTime(), startTX_e);
    }
}

void Sensor::initOrphan()
{
    // set BS as CH
    CH_id = BS_ID;
#ifdef USE_BS_DIST
    CH_dist = BS_DIST(x,y);
#else
    CH_dist = MAX_DIST(range);
#endif
    // notify the BS that we are going to join it's cluster
    mJoin *JOIN = new mJoin("join-cluster", JOIN_M);
    double delay = propagationDelay(JOIN_M_SIZE, CH_dist);
    JOIN->setId(id);
    sendDirect(JOIN, delay, 0, BS->gate("in"));
#ifdef ACCOUNT_CH_SETUP
    // account for energy transmission based on distance
    EnergyMgmt(TX, CH_dist, JOIN_M_SIZE);
#endif
}

void Sensor::setupDataTX(mSchedule *SCHED){

    unsigned int r = par("round");
    if(r == SCHED->getRound()){

        if(par("DistAwareCH")){
            if(CH_id != SCHED->getCHId()){
                CH_id = SCHED->getCHId();   // re-set the CH information if a better one has been designed by original CH
                CH_dist = distance(CH_id);
            }
        }

        // setup transmission time as the slot duration times my turn
        scheduleAt(simTime()+(SCHED->getDuration()*SCHED->getTurn()), startTX_e);
        cancelAndDelete(SCHED);
    }

}

void Sensor::sendData(){
    mData *DATA = new mData("data", DATA_M);
    DATA->setId(id);
    DATA->setRound(par("round"));
    if(CH_id > -1){
        // if node has CH
        double delay = propagationDelay(DATA_M_SIZE, CH_dist);
        cModule *CH;
        if(CH_id != BS_ID)
            CH = retrieveNode(CH_id);
        else
            CH = BS;
        sendDirect(DATA, delay, 0, CH->gate("in"));
        // ACCOUNT FOR DATA TRANSMISSION
        EnergyMgmt(TX, CH_dist, DATA_M_SIZE);

#ifndef ONE_TX_PER_ROUND
        // setup a timeout to receive a SCHED message for the next transmission
        double tout = propagationDelay(SCHED_M_SIZE,CH_dist);
        scheduleAt(simTime()+2*tout, rcvdSCHED_e);
        //TODO FINISH TO SETUP THE TIMEOUT (ALSO FOR THE FIRST SCHED) (only needed if multiple TX per round happens)
#endif
    }
    /*else
    {
        // if node doesn't have CH, just send directly to BS
        double delay = propagationDelay(DATA_M_SIZE, MAX_DIST(range));
        EnergyMgmt(TX, MAX_DIST(range), DATA_M_SIZE);
        scheduleAt(simTime()+delay, startTX_e); // schedule next autonomous send

    }*/
}


/**************** CLUSTER HEAD (CH) functions *********************/
void Sensor::broadcastADV()
{
    double ADV_delay = propagationDelay(ADV_M_SIZE, MAX_DIST(range)); // we consider maximum distance to reach all possible nodes

    for(unsigned int n = 0; n < N; n++){
        if(n != id){
            cModule * sensor = retrieveNode(n);
            mAdvertisement *ADV = new mAdvertisement("CH_advertisement", ADV_M);
            ADV->setId(id);
            sendDirect(ADV, ADV_delay, 0,  sensor->gate("in"));
        }
    }

#ifdef ACCOUNT_CH_SETUP
    // in this case, we consider an amount of energy to send a signal that
    // covers the entire sensed area
    EnergyMgmt(TX, MAX_DIST(range), ADV_M_SIZE);
#endif
    // set a timeout to receive JOIN messages
    // we consider a timeout equal to the maximum distance (i.e. range*2) propagation delay for both ADV to reach sensors
    // and for the JOIN msg to reach back at CH
    double JOIN_delay = propagationDelay(JOIN_M_SIZE, MAX_DIST(range));
    scheduleAt(simTime() + ADV_delay+JOIN_delay+EPSILON, rcvdJoin_e);

#ifdef ACCOUNT_CH_SETUP
    // ACCOUNT FOR ENERGY SPENT WHILE IN IDLE STATE to receive JOIN messages
    EnergyMgmt(RX, 0, JOIN_M_SIZE);
#endif


}

void Sensor::advertisementPhase()
{
    alreadyCH = true;   // node excludes itself from next election
    role = CH;
    broadcastADV(); // broadcast ADV message
    getDisplayString().setTagArg("i", 0, "old/ball2"); // UI feedback
}

bool pairCompareBoth( const std::pair<double, double> &firstEl, const std::pair<double,double> &secondEl)
{
    return ((firstEl.first < secondEl.first) && (firstEl.second < secondEl.second));
}

bool pairCompareDist( const std::pair<double, double> &firstEl, const std::pair<double,double> &secondEl)
{
    return (firstEl.first < secondEl.first);
}

bool pairCompareEnergy( const std::pair<double, double> &firstEl, const std::pair<double,double> &secondEl)
{
    return (firstEl.second < secondEl.second);
}

void Sensor::createTXSched()
{
    clusterN = msgBuf.size();

#ifdef CH_SLOT_MAXDIST_IN_CLUSTER
    // in order to adjust power of transmission, first keep track of the max_distance of nodes among the ones in the cluster
    sensor_max_dist = -1 * std::numeric_limits<double>::infinity();
    for(unsigned int i = 0; i < msgBuf.size(); i++){
        mJoin *JOIN = (mJoin *) msgBuf.at(i);
        double dist = distance(JOIN->getId());
        if(dist > sensor_max_dist){
            sensor_max_dist = dist;
        }
    }

    // each node has to be assigned a temporal slot, based on msg DATA size they send and max propagation delay in the cluster
    double slot = propagationDelay(DATA_M_SIZE, sensor_max_dist);
    double SCHED_delay = propagationDelay(SCHED_M_SIZE, sensor_max_dist);
#else
    // the TDMA is equal for all cluster (doesn't depend on the max node distance in the cluster, but on the max propagation delay in the network)
    // this is to better compare energy efficiency with respect to Direct Transmission approach
    double slot = propagationDelay(DATA_M_SIZE, MAX_DIST(range));
    double SCHED_delay = propagationDelay(SCHED_M_SIZE, MAX_DIST(range));
#endif


    // ****************************************************
    // ***************AVOID TOO CLOSE CH STRATEGY**********
    // ****************************************************
    if(par("DistAwareCH") || par("EnergyAwareCH"))
    {
        int center_id = id;

        std::vector<std::pair<double, double>> DistBatt;
        std::vector<std::pair<int, std::pair<double, double>>> List_IDFeat;

        double sumDist = 0;
        // first set myself
        for(unsigned int y = 0; y < msgBuf.size(); y++){
            mJoin *JOIN = (mJoin *) msgBuf.at(y);
            sumDist += distance(JOIN->getId());
        }
        double max_energy = par("energy");
        std::pair<double, double> me(sumDist,max_energy - energy);
        DistBatt.push_back(me);
        std::pair<int, std::pair<double, double>> meSupport(id,me);
        List_IDFeat.push_back(meSupport);

        // then check among other nodes in the cluster if there's one better centered
        // in order to avoid too close CH and more homogeneous transmissions
        for(unsigned int i = 0; i < msgBuf.size(); i++){
            sumDist = 0;
            mJoin *JOIN1 = (mJoin *) msgBuf.at(i);
            for(unsigned int y = 0; y < msgBuf.size(); y++){
                mJoin *JOIN2 = (mJoin *) msgBuf.at(y);
                sumDist += distance2s(JOIN1->getId(),JOIN2->getId());
            }

            Sensor *sensor = check_and_cast<Sensor *>(retrieveNode(JOIN1->getId()));


            EV << "SumDist for " << JOIN1->getId() << " = " << sumDist << " - energy = " << sensor->getEnergy() << "\n";
            std::pair<double,double> s(sumDist, max_energy - sensor->getEnergy());
            DistBatt.push_back(s);
            std::pair<int, std::pair<double, double>> sSupport(JOIN1->getId(),s);
            List_IDFeat.push_back(sSupport);

        }

        if(par("DistAwareCH") && par("EnergyAwareCH"))
            // sort based on both energy and sum of distances
            std::sort(DistBatt.begin(),DistBatt.end(),pairCompareBoth);
        else if(par("DistAwareCH"))
            std::sort(DistBatt.begin(),DistBatt.end(),pairCompareDist);
        else if(par("EnergyAwareCH"))
            std::sort(DistBatt.begin(),DistBatt.end(),pairCompareEnergy);


        // let's find the original id by comparing the first new element
        for(unsigned l = 0; l < List_IDFeat.size(); l++)
        {
            if(List_IDFeat.at(l).second == DistBatt.at(0)) center_id = List_IDFeat.at(l).first;
        }

        EV << "min SumDist is " << center_id << "\n";

        if(center_id != id)
        {
            // if we ended up selecting another cluster, reset our CH role

            alreadyCH = false;   // include again for a new election
            role = SENSOR;
            getDisplayString().setTagArg("i", 0, "old/ball"); // UI feedback

            //setup new CH information
            CH_id = center_id;
            CH_dist = distance(center_id);

            // make the new CH aware of it's new role and handle the incoming data
            // send a message (with the num. of sensors in the cluster)
            //      to just collect the data received after TDMA schedule, compress and send to BS
            mCenterCH *CENTER = (mCenterCH *) new mCenterCH("alternative-CH", CENTER_M);
            CENTER->setClusterN(clusterN);
            CENTER->setIDLETime(clusterN*slot);
            CENTER->setSCHEDDelay(SCHED_delay);

            cModule *sensor = retrieveNode(CH_id);
            EV << "informing new CH \n";
            sendDirect(CENTER, 0, 0, sensor->gate("in"));

            // send to sensors their turn, as if I was in the turn of the new CH
            for(unsigned int i = 0; i < msgBuf.size(); i++){
                mJoin *JOIN = (mJoin *) msgBuf.at(i);
                mSchedule *SCHED = (mSchedule *) new mSchedule("schedule-info", SCHED_M);
                SCHED->setTurn(i);
                SCHED->setDuration(slot);
                SCHED->setRound(par("round"));
                SCHED->setCHId(center_id); // this specifies where to send the DATA

                if(JOIN->getId() != center_id){ // all except the new clusterhead
                    cModule *sensor = retrieveNode(JOIN->getId());
                    EV << "sending schedule to " << JOIN->getId() << "\n";
                    sendDirect(SCHED, SCHED_delay, 0, sensor->gate("in"));
                }else{ // instead of clusterhead, send its turn to me
                    EV << "sending schedule to MYSELF (NOT CH ANYMORE)\n";
                    scheduleAt(simTime()+SCHED_delay, SCHED);
                }
                cancelAndDelete(JOIN);
            }

            msgBuf.clear(); // empty buffer
            #ifdef ACCOUNT_CH_SETUP
            //account for energy this transmission based on distance
            EnergyMgmt(TX, sensor_max_dist, SCHED_M_SIZE);
            #endif

        }
        else
        {
            // just send their SCHED information (i.e. their turn to transmit) as usual LEACH
            for(unsigned int i = 0; i < msgBuf.size(); i++){
                mJoin *JOIN = (mJoin *) msgBuf.at(i);
                mSchedule *SCHED = (mSchedule *) new mSchedule("schedule-info", SCHED_M);
                SCHED->setTurn(i);
                SCHED->setDuration(slot);
                SCHED->setRound(par("round"));
                SCHED->setCHId(id); // this specifies where to send the DATA (ourselves in this case)
                cModule *sensor = retrieveNode(JOIN->getId());
                EV << "sending schedule to " << JOIN->getId() << "\n";
                sendDirect(SCHED, SCHED_delay, 0, sensor->gate("in"));
                cancelAndDelete(JOIN);
            }

            msgBuf.clear(); // empty buffer
            #ifdef ACCOUNT_CH_SETUP
            //account for energy this transmission based on distance
            EnergyMgmt(TX, sensor_max_dist, SCHED_M_SIZE);
            #endif


            double IDLE_duration = clusterN*slot;
            // setup a timer to keep radio in IDLE mode and receive all data (TDMA)
            // Timeout will take in account the propagation delay for SCHED msg to reach destination and to receive back all data sequentially
            scheduleAt(simTime() + SCHED_delay + IDLE_duration + EPSILON, rcvdData_e);
            #ifdef ACCOUNT_CH_SETUP
            // account for energy during IDLE time
            EnergyMgmt(RX, 0, clusterN*DATA_M_SIZE);
            #endif
        }

    }
    else
    // ****************************************************
    // *************** TRADITIONAL LEACH *****************
    // ****************************************************
    {

        // now send their SCHED information (i.e. their turn to transmit)
        for(unsigned int i = 0; i < msgBuf.size(); i++){
            mJoin *JOIN = (mJoin *) msgBuf.at(i);
            mSchedule *SCHED = (mSchedule *) new mSchedule("schedule-info", SCHED_M);
            SCHED->setTurn(i);
            SCHED->setDuration(slot);
            SCHED->setRound(par("round"));
            SCHED->setCHId(id);
            cModule *sensor = retrieveNode(JOIN->getId());
            EV << "sending schedule to " << JOIN->getId() << "\n";
            sendDirect(SCHED, SCHED_delay, 0, sensor->gate("in"));
            cancelAndDelete(JOIN);
        }

        msgBuf.clear(); // empty buffer
        #ifdef ACCOUNT_CH_SETUP
        //account for energy this transmission based on distance
        EnergyMgmt(TX, sensor_max_dist, SCHED_M_SIZE);
        #endif


        double IDLE_duration = clusterN*slot;
        // setup a timer to keep radio in IDLE mode and receive all data (TDMA)
        // Timeout will take in account the propagation delay for SCHED msg to reach destination and to receive back all data sequentially
        scheduleAt(simTime() + SCHED_delay + IDLE_duration + EPSILON, rcvdData_e);
        #ifdef ACCOUNT_CH_SETUP
        // account for energy during IDLE time
        EnergyMgmt(RX, 0, clusterN*DATA_M_SIZE);
        #endif
    }



}

void Sensor::compressAndSendToBS()
{
    //compress all data received
    EnergyMgmt(COMPRESS, 0, clusterN*DATA_M_SIZE);

    //send to base station
    //compute energy to send data considering maximum distance (i.e. highest energy)

    //unsigned int data_aggr_size = ceil((clusterN*DATA_M_SIZE)/COMP_FACTOR);
    unsigned int data_aggr_size = DATA_M_SIZE; // we just assume all the same packet size transmitted to BS after compression

#ifdef USE_BS_DIST
    EnergyMgmt(TX, BS_DIST(x,y), data_aggr_size);
#else
    EnergyMgmt(TX, MAX_DIST(range), data_aggr_size);
#endif

#ifndef ONE_TX_PER_ROUND
    // set-up the next transmission
    // in this case, we avoid to clear the buffer. We're gonna exploit polymorphism to
    // use DATA packets as JOIN packets in the new schedule creation (they both have id field).
    // This is useful for keeping track of nodes that are still sending DATA (in case someone died) and adjust
    // the TDMA schedule accordingly
#ifdef USE_BS_DIST
    double delay = propagationDelay(data_aggr_size, BS_DIST(x,y));
#else
    double delay = propagationDelay(data_aggr_size, MAX_DIST(range));
#endif
    scheduleAt(simTime()+delay,rcvdJoin_e); // schedule next transmission after Aggregated data has been (virtually) sent
#endif
}

void Sensor::handleData(cMessage *msg)
{
    unsigned int r = par("round");
    mData *DATA = (mData *) msg;
    if ((role == CH) && (r == DATA->getRound())){
        msgBuf.push_back(msg); // insert DATA into the message buffer
        EV << "received data from " << msg->getSenderModuleId() - 2 << "\n";
    }
}

/********* ENERGY functions **********/
// energy consumption to transmit k bit ad distance d
double Sensor::EnergyTX(unsigned int k, double d)
{
    return ((Eelec * k) + Eamp * k * pow(d,2));
}

// energy consumption to receive k bit
double Sensor::EnergyRX(unsigned int k)
{
    return Eelec * k;
}

// energy consumption to aggregate n messages of k bits (kN = k* n)
double Sensor::EnergyCompress(unsigned int kN)
{
    return Ecomp * kN;
}



void Sensor::EnergyMgmt(compState state, double d, unsigned int k)
{
    double cost = 0;    // cost of operation init

    switch(state)
    {
        case TX:
            cost = EnergyTX(k,d);
            EV << "TX cost is " << cost << " and energy is " << energy << " " << (cost < energy) << "\n";
            break;
        case RX:
            cost = EnergyRX(k);
            EV << "RX cost is " << cost << " and energy is " << energy << " " << (cost < energy) << "\n";
            break;
        case COMPRESS:
            cost = EnergyCompress(k);
            EV << "Compression cost is " << cost << " and energy is " << energy << " " << (cost < energy) << "\n";
            break;
    }

    emit(energySignal, energy);

    if (cost < energy)
    {
        // if we have enough energy, subtract the cost of operation from the actual energy
        energy -= cost;
        char buf[256];
        sprintf(buf, "energy %.2f\n", energy);
        getDisplayString().setTagArg("t", 0, buf);
    }
    else
    {
        //this operation will make the node die, so we can simply declare it as dead
        role = DEAD;
        EV << "Node " << id << " is DEAD.\n";
        getDisplayString().setTagArg("i", 0, "old/ball"); // UI feedback
        getDisplayString().setTagArg("i2", 0, "old/x_cross");
        cancelEvent(startRound_e);
        unsigned int Ndead = getParentModule()->par("Ndead");
        getParentModule()->par("Ndead") = Ndead+1;
        if (Ndead+1 == N) endSimulation(); // stop simulation if all nodes are dead
        int r = getParentModule()->par("round");
        if (Ndead+1 == 1) recordScalar("firstNodeDead", r);
    }
}


/********* Utilities ************/
cModule* Sensor::retrieveNode(unsigned int n)
{
   char modName[32];
   sprintf(modName,"node[%d]", n);
   return (cModule *) getModuleByPath(modName);
}

double Sensor::propagationDelay(unsigned int msg_size, double dist)
{
    // Compute the propagation delay based on packet size and distance
    // that is the time when the last bit of message is received
    double Dp = msg_size / bitrate; // packet duration
    return dist/C + Dp;  // propagation delay
}

double Sensor::distance(unsigned int id)
{
    cModule *sensor = retrieveNode(id);
    int sx = sensor->par("posX");
    int sy = sensor->par("posY");
    double dx = x - ((double) sx);
    double dy = y - ((double) sy);
    return sqrt( pow(dx,2) + pow(dy,2));
}

double Sensor::distance2s(unsigned int id1, unsigned int id2)
{
    cModule *sensor1 = retrieveNode(id1);
    int sx1 = sensor1->par("posX");
    int sy1 = sensor1->par("posY");
    cModule *sensor2 = retrieveNode(id2);
    int sx2 = sensor2->par("posX");
    int sy2 = sensor2->par("posY");
    double dx = ((double) sx1) - ((double) sx2);
    double dy = ((double) sy1) - ((double) sy2);
    return sqrt( pow(dx,2) + pow(dy,2));
}

double Sensor::getEnergy()
{
    return energy;
}

