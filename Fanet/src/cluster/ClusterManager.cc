#include "src/cluster/ClusterManager.h"
#include "inet/networklayer/common/L3AddressResolver.h"

Define_Module(ClusterManager);

void ClusterManager::initialize()
{
    hldTimeThreshold = par("hldTimeThreshold");
    maxTrials        = par("maxTrials");
    maxClusterSize   = par("maxClusterSize");
    helloInterval    = par("helloInterval");
    requestTimeout   = par("requestTimeout");

    cModule *node = getContainingNode(this);
    myId = nodeId();

    metrics = node->getSubmodule("metrics")
            ? check_and_cast<NodeMetrics*>(node->getSubmodule("metrics"))
            : nullptr;

    rl = node->getSubmodule("rl")
       ? check_and_cast<RlAgent*>(node->getSubmodule("rl"))
       : nullptr;

    helloTick = new cMessage("helloTick");
    cfTick    = new cMessage("cfTick");
    requestTimeoutMsg = new cMessage("requestTimeout");

    scheduleAt(simTime() + uniform(0, helloInterval), helloTick);
    scheduleAt(simTime() + uniform(0, 1), cfTick);
}

int ClusterManager::nodeId() const
{
    cModule *node = getContainingNode(const_cast<ClusterManager*>(this));
    return node ? node->getIndex() : -1;
}

void ClusterManager::finish()
{
    cancelAndDelete(helloTick);
    cancelAndDelete(cfTick);
    cancelAndDelete(requestTimeoutMsg);
}

double ClusterManager::subjectUtility() const
{
    if (!rl) return 0.0;
    return rl->getClusterUtility();
}

double ClusterManager::subjectHldTime() const
{
    if (!metrics) return 0.0;
    return metrics->getLastLinkHoldingTime();
}

void ClusterManager::removePendingCandidate(){
    removeCandidate(pendingCandidateId);
    pendingCandidateId = -1;
    trials++;
    runClusterFormation();
}

void ClusterManager::handleMessage(cMessage *msg)
{
    if (msg == helloTick) {
        sendHello();
        scheduleAt(simTime() + helloInterval, helloTick);
    }
    else if (msg == cfTick) {
        if (role == ROLE_FREE && currentClusterHeadId < 0)
            runClusterFormation();
        scheduleAt(simTime() + 1, cfTick); // CF trigger interval
    }
    else if (msg == requestTimeoutMsg) {
        // no response from candidate
        if (pendingCandidateId >= 0 && trials < maxTrials) {
            removePendingCandidate();

        } else if (role == ROLE_FREE && currentClusterHeadId < 0) {
            // condition 6: become CH yourself
            becomeClusterHead();
        }
    }
    else {
        ClusterMsg *pkt = check_and_cast<ClusterMsg*>(msg);
        handleClusterMsg(pkt);
    }
}

void ClusterManager::sendHello()
{
    auto *pkt = new ClusterMsg("hello");
    pkt->setKind(HELLO);
    pkt->setSrcId(myId);
    pkt->setClusterHeadId(currentClusterHeadId);
    pkt->setUtility(subjectUtility());
    pkt->setHldTime(subjectHldTime());
    send(pkt, "out");
}

void ClusterManager::becomeClusterHead(){
    role = ROLE_CH;
    currentClusterHeadId = myId;
    auto *decl = new ClusterMsg("decl");
    decl->setKind(DECLARATION);
    decl->setSrcId(myId);
    decl->setClusterHeadId(myId);
    decl->setUtility(subjectUtility());
    decl->setHldTime(subjectHldTime());
    send(decl, "out");
}

void ClusterManager::handleClusterMsg(ClusterMsg *pkt)
{
    int src = pkt->getSrcId();

    NeighborInfo &n = neighbors[src];
    n.id          = src;
    n.utility     = pkt->getUtility();
    n.hldTime     = pkt->getHldTime();
    n.lastHeard   = simTime();
    n.isClusterHead = (pkt->getClusterHeadId() == src);

    switch (pkt->getKind()) {
        case HELLO:
            // nothing more
            break;

        case CH_REQUEST:
            handleJoinRequest(pkt);
            break;

        case CH_RESPONSE:
            processClusterHeadResponse(pkt);
            break;

        case DECLARATION:
            processDeclaration(pkt);
            break;
    }

    delete pkt;
}

// Algorithm 6 - cluster formation at subject node
void ClusterManager::runClusterFormation()
{
    trials = 0;
    pendingCandidateId = -1;

    while (true) {
        NeighborInfo *cand = findBestClusterHead();
        if (!cand)
            break;  // no CH candidates left

        double subjU = subjectUtility();
        if (cand->utility > subjU && cand->hldTime > hldTimeThreshold) {
            auto *req = new ClusterMsg("chRequest");
            req->setKind(CH_REQUEST);
            req->setSrcId(myId);
            req->setClusterHeadId(cand->id);
            req->setUtility(subjU);
            req->setHldTime(subjectHldTime());
            send(req, "out");

            pendingCandidateId = cand->id;
            scheduleAt(simTime() + requestTimeout, requestTimeoutMsg);
            return;
        } else {
            removeCandidate(cand->id);
        }
    }

    // Step 6: if no cluster head chosen, declare yourself CH
    if (currentClusterHeadId < 0 && role == ROLE_FREE) {
        becomeClusterHead();
    }
}

ClusterManager::NeighborInfo* ClusterManager::findBestClusterHead()
{
    NeighborInfo *best = nullptr;
    for (auto &kv : neighbors) {
        auto &n = kv.second;
        if (!n.isClusterHead)
            continue;
        if (!best || n.utility > best->utility)
            best = &n;
    }
    return best;
}

void ClusterManager::removeCandidate(int id)
{
    auto it = neighbors.find(id);
    if (it != neighbors.end())
        it->second.isClusterHead = false;
}

// Step 4: handle accepted/denied CH response at subject
void ClusterManager::processClusterHeadResponse(ClusterMsg *pkt)
{
    if (pkt->getClusterHeadId() != pendingCandidateId)
        return;

    cancelEvent(requestTimeoutMsg);

    if (pkt->getClusterHeadId() >= 0) {
        role = ROLE_MEMBER;
        currentClusterHeadId = pkt->getClusterHeadId();
    } else {
        // explicit rejection: treat as timeout
        if (trials < maxTrials) {
            removePendingCandidate();
        } else {
            becomeClusterHead();
        }
    }
}

// Step 6: update upon declaration messages
void ClusterManager::processDeclaration(ClusterMsg *pkt)
{
    int chId = pkt->getClusterHeadId();

    NeighborInfo &n = neighbors[chId];
    n.id = chId;
    n.isClusterHead = true;
    n.utility   = pkt->getUtility();
    n.hldTime   = pkt->getHldTime();
    n.lastHeard = simTime();

    // optional: if we are free and have no CH, we may re-run CF
}

// CHRS – behavior at nodes already CH
void ClusterManager::handleJoinRequest(ClusterMsg *pkt)
{
    if (role != ROLE_CH)
        return;

    int requesterId = pkt->getSrcId();
    double reqHld   = pkt->getHldTime();

    if (!hasCapacity() || reqHld <= hldTimeThreshold) {
        auto *resp = new ClusterMsg("chRespReject");
        resp->setKind(CH_RESPONSE);
        resp->setSrcId(myId);
        resp->setClusterHeadId(-1);  // indicate rejection
        resp->setUtility(0);
        resp->setHldTime(subjectHldTime());
        send(resp, "out");
        return;
    }

    clusterMembers.insert(requesterId);

    auto *resp = new ClusterMsg("chRespAccept");
    resp->setKind(CH_RESPONSE);
    resp->setSrcId(myId);
    resp->setClusterHeadId(myId);
    resp->setUtility(subjectUtility());
    resp->setHldTime(subjectHldTime());
    send(resp, "out");

    // Cf hook: increase confirmations counter in RlAgent
    // e.g., rl->onClusterConfirmation(); implement that as increment in Cf window
}
