package src.cluster;

import inet.transportlayer.contract.IUdp;
import inet.applications.contract.IApp;

simple ClusterApp like IApp
{
    parameters:
        int localPort = default(4000);
        int destPort  = default(4000);
        string destAddress = default("10.0.0.255"); // or multicast
        double helloInterval @unit(s) = default(1s);
        double cfInterval @unit(s) = default(2s);
        double requestTimeout @unit(s) = default(1s);
        double hldTimeThreshold @unit(s) = default(1s);
        int    maxTrials        = default(3);
        int    maxClusterSize   = default(10);
        @display("i=block/app");
    gates:
        input  socketIn;
        output socketOut;
}