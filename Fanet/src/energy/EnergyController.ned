package src.energy;

simple EnergyController
{
    parameters:
        double offThreshold = default(0.05);  // fraction of capacity at which to shut down
        @display("i=block/stop");
}