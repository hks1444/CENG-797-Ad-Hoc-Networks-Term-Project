package src;

import inet.node.inet.AdhocHost;
import inet.physicallayer.wireless.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import inet.visualizer.canvas.integrated.IntegratedCanvasVisualizer;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.node.contract.INetworkNode;
import inet.physicallayer.wireless.common.contract.packetlevel.IRadioMedium;
import inet.visualizer.contract.IIntegratedVisualizer;

network AodvRealAdhoc5
{
    parameters:
        @display("bgb=1250,1250");

        @figure[rcvdPkText](type=indicatorText; pos=380,20; anchor=w; font=,18; textFormat="packets received: %g"; initialValue=0);
        @statistic[packetReceived](source=host[24].app[0].packetReceived; record=figure(count); targetFigure=rcvdPkText);
    submodules:
        visualizer: <default(firstAvailableOrEmpty("IntegratedCanvasVisualizer"))> like IIntegratedVisualizer if typename != "" {
            @display("p=900,900");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=900,800");
        }
        radioMedium: <default("UnitDiskRadioMedium")> like IRadioMedium {
            @display("p=900,700");
        }
        host[25]: <default("WirelessHostWithMetrics")> like INetworkNode {
             parameters:
                @display("i=device/cellphone");
                mobility.initFromDisplayString = false;
                //mobility.initialY = (index == 0) ? 500m : 100m;
//                mobility.initialY = 100m + 100m * (index/5);
//                mobility.initialX = 100m + 100m * (index%5);

        }
}

