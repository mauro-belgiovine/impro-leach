/*
 * common.h
 *
 *  Created on: Apr 12, 2018
 *      Author: mauro
 */

#ifndef COMMON_H_
#define COMMON_H_

#include "common_m.h"

#define LIGHTSPEED 300*10e6 // 300,000,000 m/s
#define EPSILON 0.000001 // 1 us

#define ADV_M_SIZE 128 // size of ADV message (bit)
#define JOIN_M_SIZE 128 // size of JOIN message (bit)
#define SCHED_M_SIZE 128+64 // size of
#define DATA_M_SIZE 2000 // size of DATA message (bit)
#define COMP_FACTOR 10.0

#define MAX_DIST(range) (range)
//#define BS_DIST(x,y) (sqrt(pow(((100) - x),2) + pow(((-100) - y),2)))
#define BS_DIST(x,y) (sqrt(pow((x),2) + pow((y),2)))

//#define CH_SLOT_MAXDIST_IN_CLUSTER // <-- enable almost adaptive TDMA. if not, TDMA slots are the same for all the network
//#define USE_BS_DIST // <-- use the real distance from BS instead of MAX_DIST
//#define ACCOUNT_CH_SETUP
#define ONE_TX_PER_ROUND

#define BS_ID 999999


enum msgKinds {
    // LEACH protocol messages
    ADV_M,
    JOIN_M,
    SCHED_M,
    DATA_M,
    // internal events
    START_ROUND,
    START_TX,
    RCVD_ADV,
    RCVD_JOIN,
    RCVD_SCHED,
    RCVD_DATA,
    // new events
    CENTER_M
};

enum compState {
    RX,
    TX,
    COMPRESS
};

enum nodeRole {
    SENSOR,
    CH,
    DEAD
};



#endif /* COMMON_H_ */
