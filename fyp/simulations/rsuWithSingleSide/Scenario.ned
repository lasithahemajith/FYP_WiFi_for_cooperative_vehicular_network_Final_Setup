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

package fyp.simulations.rsuWithSingleSide;

import inet.physicallayer.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import inet.visualizer.integrated.IntegratedCanvasVisualizer;
import org.car2x.veins.subprojects.veins_inet.VeinsInetManager;

network Scenario
{
    submodules:
        radioMedium: Ieee80211ScalarRadioMedium;
        manager: VeinsInetManager;
        node[0]: Car;
        rsu[3]: rsu {

            @display("p=122,188");
        }

}
