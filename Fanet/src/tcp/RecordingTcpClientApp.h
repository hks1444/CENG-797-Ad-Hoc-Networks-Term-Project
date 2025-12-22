// src/tcp/RecordingTcpClientApp.h
#ifndef __SRC_TCP_RECORDINGTCPCLIENTAPP_H
#define __SRC_TCP_RECORDINGTCPCLIENTAPP_H

#include <cmath>
#include <limits>

#include "inet/applications/tcpapp/TcpBasicClientApp.h"
#include "inet/common/INETDefs.h"

namespace inet {

class RecordingTcpClientApp : public TcpBasicClientApp
{
  protected:
    // periodic sampling
    cMessage *delayTick = nullptr;
    simtime_t delaySamplePeriod = SIMTIME_ZERO;   // if 0 -> disabled
    simtime_t delayStaleWindow  = SIMTIME_ZERO;   // if 0 -> always accept lastDelay

    // last observed transaction
    simtime_t lastReqSentTime = SIMTIME_ZERO - 1;
    simtime_t lastReplyTime   = SIMTIME_ZERO - 1;
    double    lastDelaySec    = std::numeric_limits<double>::quiet_NaN();

    // outputs
    cOutVector v_delayPerReply;
    cOutVector v_delayPeriodic;

    std::vector<L3Address> targetAddrs;
    simtime_t targetChangePeriod;
    cMessage *targetChangeTimer = nullptr;

    virtual void initialize(int stage) override;
    virtual void finish() override;

    // hook points inside TcpBasicClientApp
    virtual void sendRequest() override;
    virtual void socketDataArrived(TcpSocket *socket, Packet *msg, bool urgent) override;
    virtual void handleTimer(cMessage *msg) override;

};

} // namespace inet

#endif
