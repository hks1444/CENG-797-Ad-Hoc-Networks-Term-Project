package src.rl;

simple RlAgent
{
    parameters:
        double updatePeriod @unit(s) = default(1s);
        int    numActions = default(4);
        @display("i=block/table");
}
