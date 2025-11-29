#ifndef SRC_CLUSTER_CLUSTERAPP_H
#define SRC_CLUSTER_CLUSTERAPP_H

#include "inet/applications/base/ApplicationBase.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/common/geometry/common/Coord.h"
#include "inet/mobility/contract/IMobility.h"
#include "src/cluster/ClusterHeader_m.h"
#include "src/metrics/NodeMetrics.h"
#include "src/rl/RlAgent.h"

#define CH_GRACE_PERIOD 10
#define TIMEOUT_VALUE 3
enum msgTypes{
    HELLO,
    CH_REQUEST,
    CH_RESPONSE,
    DECLARATION,
};

using namespace inet;

class ClusterApp : public ApplicationBase
{
  protected:
    // UDP plumbing
    UdpSocket socket;
    int localPort = -1;
    int destPort  = -1;
    L3Address destAddr;

    // hooks into your metrics and RL
    NodeMetrics *metrics = nullptr;
    RlAgent     *rl      = nullptr;

    // timers
    cMessage *helloTimer = nullptr;
    cMessage *cfTimer    = nullptr;
    cMessage *requestTimeoutMsg = nullptr;

    // roles / cluster state
    enum Role { ROLE_FREE = 0, ROLE_CH = 1, ROLE_MEMBER = 2 };
    Role role = ROLE_FREE;
    int myId  = -1;
    int currentClusterHeadId = -1;
    simtime_t becoming_ch_time = 0;

    struct NeighborInfo {
        int    id = -1;
        double utility = 0.0;
        double hldTime = 0.0;
        bool   isClusterHead = false;
        simtime_t lastHeard;
    };

    std::map<int, NeighborInfo> neighbors;
    std::set<int> clusterMembers;

    // CF state
    int maxTrials;
    int trials = 0;
    int pendingCandidateId = -1;
    double hldTimeThreshold;
    int maxClusterSize;
    simtime_t helloInterval;
    simtime_t cfInterval;
    simtime_t requestTimeout;

    enum SelfKinds { HELLO_TIMER = 1, CF_TIMER = 2, REQ_TIMEOUT = 3 };

  protected:
    // ApplicationBase overrides
    virtual void initialize(int stage) override;
    virtual void handleMessageWhenUp(cMessage *msg) override;
    virtual void finish() override;
    virtual void handleStartOperation(LifecycleOperation *operation) override;
    virtual void handleStopOperation(LifecycleOperation *operation) override {}
    virtual void handleCrashOperation(LifecycleOperation *operation) override {}

    // CF/CHRS helpers
    void handleRoleChange(Role new_role);
    void sendHello();
    void runClusterFormation();
    void handleClusterPacket(Packet *pk);
    void handleJoinRequest(const ClusterHeader& hdr);
    void handleResponse(const ClusterHeader& hdr);
    void handleDeclaration(const ClusterHeader& hdr);
    void becomeClusterHead();
    void handleClusterJoinReject();
    void handleClusterJoinAccept(int chId);
    void logCluster(const char *tag, int p1, int p2 = -1, double d1 = 0, double d2 = 0);
    void pseudo_broadcast(int kind, int dstId, int chId, double util, double hld);
    bool isCfInProgress() const;
    NeighborInfo* findBestClusterHead();
    void removeCandidate(int id);
    bool hasCapacity() const { return (int)clusterMembers.size() < maxClusterSize; }

    // utility access
    double subjectUtility() const;
    double subjectHldTime() const;
    int nodeId() const;
    double readCommRange() const;

    // message send helpers
    void sendClusterMsg(int kind, int dstId, int chId, double util, double hld, L3Address dest);

  public:
    ClusterApp() {}
};

#endif
