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

//
// ADV message
//
message mAdvertisement {
    unsigned int id; // CH id
}
// DATA message
message mData {
    int id;	// sender id
    int round; // round number
}
// JOIN message
message mJoin {
    int id;
    //string anotherField;
    //double arrayField1[];
    //double arrayField2[10];
}
// SCHED message
message mSchedule {
    int turn; // TDMA turn
    double duration; // TDMA slot duration
    int round; // round number
    int CHId; // CH id
}

// ALTERNATIVE CH SELCTION
message mCenterCH {
    int clusterN; // num. of sensors in cluster
    double IDLETime; // TDMA slot duration
    double SCHEDDelay;
}

