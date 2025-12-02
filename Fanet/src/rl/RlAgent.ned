package src.rl;

simple RlAgent
{
    parameters:
        double updatePeriod @unit(s) = default(1s);
        int    numActions            = default(4);
        int    numStates             = default(5);   // number of entropy bins
        double alpha                 = default(0.001); // learning rate
        double gamma                 = default(0.9); // discount
        double epsilon               = default(0.1); // exploration prob.
        @display("i=block/table");
}
