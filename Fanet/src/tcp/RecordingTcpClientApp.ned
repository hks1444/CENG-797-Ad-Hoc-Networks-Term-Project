// src/tcp/RecordingTcpClientApp.ned
package src.tcp;

import inet.applications.tcpapp.TcpBasicClientApp;

simple RecordingTcpClientApp extends TcpBasicClientApp
{
    parameters:
        @class(inet::RecordingTcpClientApp);
        double delaySamplePeriod @unit(s) = default(1s);  // 0s disables periodic sampling
        double delayStaleWindow  @unit(s) = default(3s);  // 0s = always accept lastDelay
        double targetChangePeriod @unit(s) = default(5s);
        @display("i=block/app");
}
