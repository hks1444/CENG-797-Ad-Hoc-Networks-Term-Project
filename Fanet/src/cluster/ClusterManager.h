#ifndef SRC_CLUSTER_CLUSTERMANAGER_H_
#define SRC_CLUSTER_CLUSTERMANAGER_H_

#include "ClusterMsg.h"
#include "inet/common/INETDefs.h"
#include "src/metrics/NodeMetrics.h"
#include "src/rl/RlAgent.h"

using namespace inet;
class ClusterManager : public cSimpleModule
{
  protected:
    struct NeighborInfo {
        int    id = -1;
        double utility = 0.0;
        double hldTime = 0.0;
        bool   isClusterHead = false;
        simtime_t lastHeard;
    };

    enum Role { ROLE_FREE = 0, ROLE_CH = 1, ROLE_MEMBER = 2 };

    Role role = ROLE_FREE;
    int  myId = -1;
    int  currentClusterHeadId = -1;

    std::map<int, NeighborInfo> neighbors;
    std::set<int> clusterMembers;

    cMessage *helloTick = nullptr;
    cMessage *cfTick = nullptr;
    cMessage *requestTimeoutMsg = nullptr;

    double hldTimeThreshold;
    int maxTrials;
    int maxClusterSize;
    simtime_t helloInterval;
    simtime_t requestTimeout;

    // CF state
    int trials = 0;
    int pendingCandidateId = -1;

    // pointers to other submodules
    NodeMetrics *metrics = nullptr;
    RlAgent     *rl      = nullptr;

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;

    void sendHello();
    void handleClusterMsg(ClusterMsg *pkt);

    void removePendingCandidate();
    void becomeClusterHead();
    void runClusterFormation();
    void processClusterHeadResponse(ClusterMsg *pkt);
    void processDeclaration(ClusterMsg *pkt);

    // CHRS logic
    void handleJoinRequest(ClusterMsg *pkt);
    bool hasCapacity() const { return (int)clusterMembers.size() < maxClusterSize; }

    // utility helpers
    double subjectUtility() const;
    double subjectHldTime() const;

    // neighbor table helpers
    NeighborInfo* findBestClusterHead();
    void removeCandidate(int id);

    int nodeId() const;
};

#endif
