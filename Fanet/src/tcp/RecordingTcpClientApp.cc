// src/tcp/RecordingTcpClientApp.cc
#include "src/tcp/RecordingTcpClientApp.h"

#include "inet/common/ModuleAccess.h"

namespace inet {

Define_Module(RecordingTcpClientApp);

void RecordingTcpClientApp::initialize(int stage)
{
    TcpBasicClientApp::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        delaySamplePeriod = par("delaySamplePeriod");
        delayStaleWindow  = par("delayStaleWindow");

        v_delayPerReply.setName("e2eDelayPerReplySec");
        v_delayPeriodic.setName("e2eDelayPeriodicSec");

        if (delaySamplePeriod > SIMTIME_ZERO) {
            delayTick = new cMessage("delayTick");
            scheduleAt(simTime() + delaySamplePeriod, delayTick);
        }
    }
}

void RecordingTcpClientApp::finish()
{
    if (delayTick) {
        cancelAndDelete(delayTick);
        delayTick = nullptr;
    }
    TcpBasicClientApp::finish();
}

void RecordingTcpClientApp::sendRequest()
{
    lastReqSentTime = simTime();
    TcpBasicClientApp::sendRequest();
}

void RecordingTcpClientApp::socketDataArrived(TcpSocket *socket, Packet *msg, bool urgent)
{
    // measure at first reply arrival after the most recent request
    if (lastReqSentTime >= SIMTIME_ZERO) {
        // treat the first data after the request as the "reply"
        if (lastReplyTime < lastReqSentTime) {
            lastReplyTime = simTime();
            lastDelaySec  = (lastReplyTime - lastReqSentTime).dbl();
            v_delayPerReply.record(lastDelaySec);
        }
    }

    TcpBasicClientApp::socketDataArrived(socket, msg, urgent);
}

void RecordingTcpClientApp::handleTimer(cMessage *msg)
{
    if (msg == delayTick) {
        double sample = std::numeric_limits<double>::quiet_NaN();

        if (!std::isnan(lastDelaySec)) {
            if (delayStaleWindow <= SIMTIME_ZERO)
                sample = lastDelaySec;
            else if (lastReplyTime >= SIMTIME_ZERO && (simTime() - lastReplyTime) <= delayStaleWindow)
                sample = lastDelaySec;
        }

        v_delayPeriodic.record(sample);

        scheduleAt(simTime() + delaySamplePeriod, delayTick);
        return;
    }

    TcpBasicClientApp::handleTimer(msg);
}

} // namespace inet
