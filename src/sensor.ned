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

package impro_leach;

//
// TODO auto-generated module
//
simple Sensor
{
    parameters:
        int posX @unit(m) = default(0);
        int posY @unit(m) = default(0);
        @display("i=old/ball;is=s;p=$posX,$posY");
        int round = default(-1);	// keep tracks of current round #
        
        double bitrate = default(25000); // max bitrate of deployed nodes (b/s).
        //double range = default(300); // max range of communication of nodes (m).
        
        double energy = default(0.5); // initial energy (J)
        double gamma = default(2); // path loss exponent
        double Eelec = default(0.000000050); // energy dissipation for radio operations (J/bit)
        double Eamp =  default(0.000000000100); // energy dissipation for radio amplifier (J/bit/m^2)
        double Ecomp = default(0.000000005); // energy dissipation for message aggregation (J/bit/msg)
        
        @signal[energy](type="double");
        @statistic[batteryLevel](title="battery level";source="energy";record=vector; interpolationmode=none);
        
        bool DistAwareCH = default(false);
        bool EnergyAwareCH = default(false);
        
        
        
    gates:
        input in @directIn;
}

