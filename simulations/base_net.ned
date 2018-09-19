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

package impro_leach.simulations;
import impro_leach.Sensor;
import impro_leach.BS;

network Base_net
{
    parameters:
        int Nnodes; // number of sensor nodes
        int Ndead = default(0);
        double P = default(0.05); // proportion of CH nodes in the network
        double roundTime = default(3); // duration of one round (s)
        int round = default(-1);	// keep tracks of current round #
        
        double edge = default(212);	// edge length (m), assuming the area is squared.
        							// it will determine the maximum range of transmission
        							// of devices (equal to the diagonal of the square area)
        int minX = default(0); // minimum X-distance from the base station ("the base station is far away")
        int minY = default(0); // same for Y-distance
    submodules:
        node[Nnodes]: Sensor;
        baseStation: BS;
        
        
    connections:
        //for i=0..Nnodes-1, for j=0..Nnodes-1 {
        //    node[i].out++ --> node[j].in++;
        //}
}
