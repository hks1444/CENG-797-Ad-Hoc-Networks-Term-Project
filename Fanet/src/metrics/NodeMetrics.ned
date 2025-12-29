package src.metrics;
import src.energy.EnergyController;
import src.rl.RlAgent;
import src.cluster.ClusterApp;
simple NodeMetrics
{
    parameters:
        double samplePeriod @unit(s) = default(0.5s);
        bool   recordPerUtility = default(true);
        @display("i=block/cog");
}

module RouterWithMetrics extends inet.node.aodv.AodvRouter
//module RouterWithMetrics extends inet.node.dsdv.DsdvRouter
//module RouterWithMetrics extends inet.node.gpsr.GpsrRouter
{
    submodules:
        metrics: NodeMetrics {
            parameters:
                samplePeriod = default(0.5s);
                @display("p=80,60");
        }
        energyController: EnergyController {
            parameters:
                offThreshold = 0.05; // shut down at 5% energy
                @display("p=40,25");
        }
        rl: RlAgent {
            parameters:
                @display("p=120,60");
        }
}
