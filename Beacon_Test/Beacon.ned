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
import inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator;
import inet.node.inet.WirelessHost;
import inet.node.inet.AdhocHost;
import inet.node.wireless.AccessPoint;
import inet.physicallayer.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import inet.visualizer.contract.IIntegratedVisualizer;
import inet.physicallayer.ieee80211.packetlevel.Ieee80211RadioMedium;
//
// TODO auto-generated type
//
network Beacon
{
    parameters:
        int num_ap = 20;
    submodules:
        accessPoint[num_ap]: AccessPoint {
            @display("p=$num_ap,$num_ap");
        }
        A: AccessPoint {
            @display("p=85,22");
        }
        B: AccessPoint {
            @display("p=85,74");
        }

        radioMedium: Ieee80211RadioMedium {
            @display("p=23,23");
        }
        iPv4NetworkConfigurator: IPv4NetworkConfigurator {
            @display("p=24,135");
        }
        visualizer: <default("IntegratedCanvasVisualizer")> like IIntegratedVisualizer if hasVisualizer() {
            parameters:
                @display("p=24,78");
        }
}
