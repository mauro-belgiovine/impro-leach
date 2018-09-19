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
import impro_leach.Sensor;
//
// TODO auto-generated module
//
simple BS
{
    parameters:

    	double bitrate = default(25000); // max bitrate of deployed nodes (b/s).
    	int round = default(-1);	// keep tracks of current round #
    	
    	@display("i=old/pctower2;p=0,0");
    
    gates:
        input in @directIn;
}
