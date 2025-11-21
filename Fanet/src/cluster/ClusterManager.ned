package src.cluster;

simple ClusterManager
{
    parameters:
        double hldTimeThreshold @unit(s) = default(1s);
        int    maxTrials = default(3);
        int    maxClusterSize = default(10);
        double helloInterval @unit(s) = default(0.5s);
        double requestTimeout @unit(s) = default(0.5s);
        @display("i=block/cloud");
    gates:
        input  in;
        output out;
}
