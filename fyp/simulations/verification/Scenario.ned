//
// Copyright (C) 2006-2017 Christoph Sommer <sommer@ccs-labs.org>
//
// Documentation for these modules is at http://veins.car2x.org/
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

package fyp.simulations.verification;

import inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator;
import inet.node.inet.WirelessHost;
import inet.node.inet.AdhocHost;
import inet.node.wireless.AccessPoint;
import inet.physicallayer.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import inet.visualizer.contract.IIntegratedVisualizer;
import inet.physicallayer.ieee80211.packetlevel.Ieee80211RadioMedium;
//import org.car2x.veins.subprojects.veins_inet.VeinsInetManager;
import inet.environment.common.PhysicalEnvironment;


network Scenario
{
    parameters:
        int num_ap;
        @display("bgb=2000,2000");
    submodules:
        radioMedium: Ieee80211ScalarRadioMedium;
        node[num_ap]: AccessPoint {
            @display("p=$num_ap,$num_ap");
        }

        visualizer: <default("IntegratedCanvasVisualizer")> like IIntegratedVisualizer if hasVisualizer() {
            parameters:
                @display("p=24,78");
        }
        iPv4NetworkConfigurator: IPv4NetworkConfigurator {
            @display("p=24,135");
        }
        physicalEnvironment: PhysicalEnvironment {
            @display("p=304.608,653.30396");
        }

}
