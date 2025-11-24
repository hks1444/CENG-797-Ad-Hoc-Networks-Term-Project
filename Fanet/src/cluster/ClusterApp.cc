#include "ClusterApp.h"

Define_Module(ClusterApp);

// ---------- logging helper ----------

void ClusterApp::logCluster(const char *tag, int p1, int p2, double d1, double d2)
{
    EV_INFO << "Cluster: [" << tag << "]"
            << " node=" << myId
            << " p1=" << p1
            << " p2=" << p2
            << " d1=" << d1
            << " d2=" << d2
            << "\n";
}

// ---------- small helpers ----------

int ClusterApp::nodeId() const
{
    auto *node = getContainingNode(const_cast<ClusterApp*>(this));
    return node ? node->getIndex() : -1;
}

double ClusterApp::subjectUtility() const
{
    return rl ? rl->getClusterUtility() : 0.0;
}

double ClusterApp::subjectHldTime() const
{
    return metrics ? metrics->getLastLinkHoldingTime() : 2;
}

// ---------- role transitions ----------

void ClusterApp::becomeClusterHead()
{
    logCluster("BECOME_CH", myId, -1, subjectUtility(), subjectHldTime());
    trials = 0;
    role = ROLE_CH;
    currentClusterHeadId = myId;
    pseudo_broadcast(DECLARATION, -1, myId, subjectUtility(), subjectHldTime());
}

void ClusterApp::handleClusterJoinReject()
{
    logCluster("JOIN_REJECT_LOCAL", pendingCandidateId, trials);

    removeCandidate(pendingCandidateId);
    pendingCandidateId = -1;
    trials++;
}

void ClusterApp::handleClusterJoinAccept(int chId)
{
    logCluster("JOIN_ACCEPT_LOCAL", chId);

    role = ROLE_MEMBER;
    currentClusterHeadId = chId;
    pendingCandidateId = -1;
}

// ---------- initialization / lifecycle ----------

void ClusterApp::initialize(int stage)
{
    ApplicationBase::initialize(stage);
    if (stage != INITSTAGE_LOCAL)
        return;

    myId  = nodeId();

    localPort = par("localPort");
    destPort  = par("destPort");

    helloInterval     = par("helloInterval");
    cfInterval        = par("cfInterval");
    hldTimeThreshold  = par("hldTimeThreshold");   // double with unit(s)
    maxTrials         = par("maxTrials");
    maxClusterSize    = par("maxClusterSize");
    requestTimeout    = par("requestTimeout");

    auto *node = getContainingNode(this);
    metrics = node->getSubmodule("metrics") ? check_and_cast<NodeMetrics*>(node->getSubmodule("metrics")) : nullptr;
    rl      = node->getSubmodule("rl")      ? check_and_cast<RlAgent*>(node->getSubmodule("rl"))         : nullptr;

    helloTimer = new cMessage("helloTimer", HELLO_TIMER);
    cfTimer    = new cMessage("cfTimer", CF_TIMER);
    requestTimeoutMsg = new cMessage("requestTimeout", REQ_TIMEOUT);
}

void ClusterApp::handleStartOperation(LifecycleOperation *operation)
{
    destAddr  = L3AddressResolver().resolve(par("destAddress").stringValue());
    socket.setOutputGate(gate("socketOut"));
    socket.bind(localPort);
    socket.setBroadcast(true);

    if (destAddr.isMulticast()) {
        socket.joinMulticastGroup(destAddr);
    }

    scheduleAt(simTime(), helloTimer);
    scheduleAt(simTime() + uniform(0, cfInterval), cfTimer);
}

void ClusterApp::finish()
{
    std::cout << this->nodeId() << " " << role << " "<< currentClusterHeadId << std::endl;
    ApplicationBase::finish();
    cancelAndDelete(helloTimer);
    cancelAndDelete(cfTimer);
    cancelAndDelete(requestTimeoutMsg);
}

// ---------- main message handler ----------

void ClusterApp::handleMessageWhenUp(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        switch (msg->getKind()) {
        case HELLO_TIMER:
            logCluster("TIMER_HELLO", 0);
            sendHello();
            scheduleAt(simTime() + helloInterval, msg);
            break;

        case CF_TIMER:
            logCluster("TIMER_CF", 0);
            if (role == ROLE_FREE && currentClusterHeadId < 0)
                runClusterFormation();
            scheduleAt(simTime() + cfInterval, msg);
            break;

        case REQ_TIMEOUT:
            logCluster("TIMER_REQ_TIMEOUT", pendingCandidateId, trials);
            // previous candidate didn’t answer
            if (pendingCandidateId >= 0 && trials < maxTrials) {
                handleClusterJoinReject();
                runClusterFormation();
            } else if (role == ROLE_FREE && currentClusterHeadId < 0) {
                becomeClusterHead();
            }
            break;
        }
        return;
    }
    // NOT a self-message: comes from UDP / socket / ICMP
    if (auto *pk = dynamic_cast<Packet *>(msg)) {
        auto hdr = pk->peekAtFront<ClusterHeader>();
        logCluster("APP_IN",
                   myId,
                   hdr->getSrcId(),
                   hdr->getUtility(),
                   hdr->getHldTime());
        // normal data packet
        handleClusterPacket(pk);
        return;
    }

    // anything else is an indication (e.g., ICMP error from UDP)
    EV_WARN << "[UDP_CTRL] node=" << myId
            << " got non-packet msg from UDP: name=" << msg->getName()
            << " kind=" << msg->getKind() << "\n";

    // For now we just drop it; you can add custom logic later if you care.
    delete msg;
}

// ---------- sending ----------

void ClusterApp::sendClusterMsg(int kind, int dstId, int chId, double util, double hld, L3Address dest)
{
   // logCluster("SEND", kind, dstId, util, hld);

    auto pk = new Packet("CLUSTER");
    auto hdr = makeShared<ClusterHeader>();
    hdr->setKind(kind);
    hdr->setSrcId(myId);
    hdr->setDstId(dstId);
    hdr->setClusterHeadId(chId);
    hdr->setUtility(util);
    hdr->setHldTime(hld);

    // give header a concrete size to avoid -1b length
    hdr->setChunkLength(inet::units::values::B(24));

    pk->insertAtFront(hdr);
    socket.sendTo(pk, dest, destPort); // broadcast at L3, dstId is app-level filter
}

void ClusterApp::sendHello()
{
    int chId = (role == ROLE_CH) ? myId : currentClusterHeadId;
    pseudo_broadcast(HELLO, -1, chId, subjectUtility(), subjectHldTime());
}

// ---------- CF helpers ----------

ClusterApp::NeighborInfo* ClusterApp::findBestClusterHead()
{
    NeighborInfo *best = nullptr;
    for (auto &kv : neighbors) {
        auto &n = kv.second;
        if (!n.isClusterHead)
            continue;
        if (!best || n.utility > best->utility)
            best = &n;
    }
    if(!best){
        logCluster("CF_best", -1, neighbors.size());
    }else{
        logCluster("CF_best", best->id);
    }
    return best;
}

void ClusterApp::removeCandidate(int id)
{
    auto it = neighbors.find(id);
    if (it != neighbors.end())
        it->second.isClusterHead = false;
}

void ClusterApp::runClusterFormation()
{
    trials = 0;
    pendingCandidateId = -1;

    logCluster("CF_START", 0);

    while (true) {
        NeighborInfo *cand = findBestClusterHead();
        if (!cand)
            break;

        double subjU = subjectUtility();
        logCluster("CF_TRY", myId, cand->id, cand->utility, cand->hldTime);

        if (cand->utility > subjU /*&& cand->hldTime > hldTimeThreshold*/) {
            // send CH_REQUEST to that node (unicast at app-level via dstId)
            pendingCandidateId = cand->id;
            pseudo_broadcast(CH_REQUEST, cand->id, cand->id, subjU, subjectHldTime());
            if (requestTimeoutMsg->isScheduled())
                cancelEvent(requestTimeoutMsg);
            scheduleAt(simTime() + requestTimeout, requestTimeoutMsg);
            return;
        } else {
            removeCandidate(cand->id);
        }
    }

    // no suitable CH found -> become CH
    if (currentClusterHeadId < 0 && role == ROLE_FREE) {
        logCluster("CF_BECOME_CH", myId);
        becomeClusterHead();
    }
}

// ---------- inbound packet handling ----------

void ClusterApp::handleClusterPacket(Packet *pk)
{
    auto hdr = pk->peekAtFront<ClusterHeader>();

    logCluster("RECV",
               myId,
               hdr->getSrcId(),
               hdr->getUtility(),
               hdr->getHldTime());

    int kind = hdr->getKind();
    int src  = hdr->getSrcId();
    int dst  = hdr->getDstId();

    // app-level unicast filter
    if (dst != -1 && dst != myId) {
        delete pk;
        return;
    }

    NeighborInfo &n = neighbors[src];
    n.id           = src;
    n.utility      = hdr->getUtility();
    n.hldTime      = hdr->getHldTime();
    n.lastHeard    = simTime();
    n.isClusterHead = (hdr->getClusterHeadId() == src);

    switch (kind) {
    case HELLO:
        // nothing else; CH detection already done
        break;

    case CH_REQUEST:
        handleJoinRequest(*hdr);
        break;

    case CH_RESPONSE:
        handleResponse(*hdr);
        break;

    case DECLARATION:
        handleDeclaration(*hdr);
        break;
    }

    delete pk;
}

double ClusterApp::readCommRange() const
{
    cModule *node = getContainingNode(const_cast<ClusterApp*>(this));

    cModule *wlan0 = node->getSubmodule("wlan", 0);
    if (!wlan0) return 0.0;
    cModule *radio = wlan0->getSubmodule("radio");
    if (!radio) return 0.0;
    cModule *tx = radio->getSubmodule("transmitter");
    if (!tx) return 0.0;
    cPar& p = tx->par("communicationRange");
    return p.doubleValue();   // meters
}

void ClusterApp::pseudo_broadcast(int kind, int dstId, int chId, double util, double hld)
{
    cModule *selfNode = getContainingNode(this);

    // our position
    IMobility *myMob = check_and_cast<IMobility*>(selfNode->getSubmodule("mobility"));
    Coord myPos = myMob->getCurrentPosition();

    // radio range
    double R = readCommRange();

    cModule *network = getSystemModule();
    for (cModule::SubmoduleIterator it(network); !it.end(); ++it) {
        cModule *m = *it;

        // skip non-hosts and self
        if (m == selfNode)
            continue;

        cModule *mobM = m->getSubmodule("mobility");
        if (!mobM)
            continue;

        IMobility *other = check_and_cast<IMobility*>(mobM);
        Coord oPos = other->getCurrentPosition();

        if (myPos.distance(oPos) > R)
            continue;   // out of range

        // IP address of neighbor
        L3Address dstAddr = L3AddressResolver().addressOf(m, L3AddressResolver::ADDR_IPv4);

        // unicast CLUSTER to this neighbor
        sendClusterMsg(kind, dstId, chId, util, hld, dstAddr);
    }
}

// ---------- CHRS: behavior at CH ----------

void ClusterApp::handleJoinRequest(const ClusterHeader& hdr)
{
    if (role != ROLE_CH)
        return;

    int requesterId = hdr.getSrcId();
    double reqHld   = hdr.getHldTime();

    logCluster("JOIN_REQ", myId, requesterId, reqHld, hldTimeThreshold);

    if (!hasCapacity() /*|| reqHld <= hldTimeThreshold*/) {
        // reject: send CH_RESPONSE with clusterHeadId = -1
        logCluster("CH_REJECT", myId, requesterId, reqHld, hldTimeThreshold);
        pseudo_broadcast(CH_RESPONSE, requesterId, -1, 0.0, subjectHldTime());
        return;
    }

    // accept
    clusterMembers.insert(requesterId);
    logCluster("CH_ACCEPT", myId, requesterId, subjectUtility(), reqHld);
    pseudo_broadcast(CH_RESPONSE, requesterId, myId, subjectUtility(), subjectHldTime());

    // RL hook could go here
    // if (rl) rl->onClusterConfirmation();
}

// ---------- subject node processing response ----------

void ClusterApp::handleResponse(const ClusterHeader& hdr)
{
    int chId = hdr.getClusterHeadId();
    int src  = hdr.getSrcId();

    logCluster("RESP", chId, src, hdr.getUtility(), hdr.getHldTime());

    if (src != pendingCandidateId)
        return;

    cancelEvent(requestTimeoutMsg);

    if (chId >= 0) {
        logCluster("JOIN_ACCEPT", chId);
        handleClusterJoinAccept(chId);
    } else {
        if (trials < maxTrials) {
            logCluster("JOIN_REJECT_RETRY", src, trials);
            handleClusterJoinReject();
            runClusterFormation();
        } else if (role == ROLE_FREE && currentClusterHeadId < 0) {
            logCluster("JOIN_REJECT_BECOME_CH", myId);
            becomeClusterHead();
        }
    }
}

// ---------- declaration handling ----------

void ClusterApp::handleDeclaration(const ClusterHeader& hdr)
{
    int chId = hdr.getClusterHeadId();

    logCluster("DECL_RECV",
               chId,
               hdr.getSrcId(),
               hdr.getUtility(),
               hdr.getHldTime());

    NeighborInfo &n = neighbors[chId];
    n.id           = chId;
    n.isClusterHead = true;
    n.utility      = hdr.getUtility();
    n.hldTime      = hdr.getHldTime();
    n.lastHeard    = simTime();
}
