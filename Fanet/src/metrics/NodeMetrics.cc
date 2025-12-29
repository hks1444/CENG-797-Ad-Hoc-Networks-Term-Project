// NodeMetrics.cc
#include "../metrics/NodeMetrics.h"

#include "inet/common/geometry/common/Coord.h"
#include "inet/physicallayer/wireless/common/medium/RadioMedium.h"

Define_Module(NodeMetrics);

void NodeMetrics::initialize()
{
    period = par("samplePeriod");
    stateSignal = registerSignal("state");
    lastS.assign(NUM_METRICS, 0.0);
    mob = getContainingNode(this)->getSubmodule("mobility") ?
          check_and_cast<IMobility*>(getContainingNode(this)->getSubmodule("mobility")) : nullptr;

    cModule *energy = getContainingNode(this)->getSubmodule("energyStorage");
    bat = energy ? check_and_cast<power::IEpEnergyStorage*>(energy) : nullptr;

    tick = new cMessage("metricsTick");
    scheduleAt(simTime() + period, tick);
}

void NodeMetrics::handleMessage(cMessage *msg)
{
    if (msg == tick) {
        sample();
        scheduleAt(simTime() + period, tick);
    }
    else
        delete msg;
}

void NodeMetrics::finish()
{
    cancelAndDelete(tick);
    tick = nullptr;
}

void NodeMetrics::sample()
{
    std::vector<double> s;
    s.reserve(NUM_METRICS);
    s.push_back(residualEnergy());
    s.push_back(degreeCentrality());
    s.push_back(velocitySimilarity());
    s.push_back(queueFill());
    s.push_back(routingCloseness());
    double lht = linkHoldingTime();
    s.push_back(lht);

    lastS = s;
    lastLht = lht;

    if (par("recordPerUtility").boolValue()) {
        for (size_t i = 0; i < s.size(); ++i)
            recordScalar((std::string("s") + std::to_string(i+1)).c_str(), s[i]);
    }

    double H = entropy(s);
    emit(stateSignal, H);
    recordScalar("stateEntropy", H);
}

// --- s1: residual energy ---
double NodeMetrics::residualEnergy()
{
    if (!bat) return 0.0; // no energy model attached
    double cap = bat->getNominalEnergyCapacity().get();
    double rem = bat->getResidualEnergyCapacity().get();
    if (cap <= 0) return 0.0;
    double r = rem / cap;
    return std::clamp(r, 0.0, 1.0);
}

static double readCommRange(cModule *node)
{
//    cModule *wlan0 = node->getSubmodule("wlan", 0);
//    if (!wlan0) return 0.0;
//    cModule *radio = wlan0->getSubmodule("radio");
//    if (!radio) return 0.0;
//    cModule *tx = radio->getSubmodule("transmitter");
//    if (!tx) return 0.0;
//    cPar& p = tx->par("communicationRange");
//    return p.doubleValue();
    return 350;
}

// --- s2: degree centrality (normalized neighbor count) ---
double NodeMetrics::degreeCentrality()
{
    if (!mob) return 0.0;

    Coord myPos = mob->getCurrentPosition();
    double R = readCommRange(getContainingNode(this));
    if (R <= 0) return 0.0;

    int N = 0;        // total nodes (minus self)
    int deg = 0;      // neighbors within range

    cModule *network = getSystemModule();
    for (cModule::SubmoduleIterator it(network); !it.end(); ++it) {
        cModule *m = *it;
        cModule *mobM = m->getSubmodule("mobility");
        if (!mobM) continue;
        if (m == getContainingNode(this)) continue;

        ++N;

        IMobility *other = check_and_cast<IMobility*>(mobM);
        Coord op = other->getCurrentPosition();
        if (myPos.distance(op) <= R) ++deg;
    }
    if (N <= 0) return 0.0;
    double s = (double)deg / (double)N;
    return std::clamp(s, 0.0, 1.0);
}

// --- s3: velocity similarity (cosine to neighbors, averaged) ---
double NodeMetrics::velocitySimilarity()
{
    if (!mob) return 0.0;

    Coord v = mob->getCurrentVelocity();
    double vnorm = v.length();
    double R = readCommRange(getContainingNode(this));
    if (R <= 0) return 0.0;

    int cnt = 0;
    double acc = 0.0;
    Coord myPos = mob->getCurrentPosition();

    cModule *network = getSystemModule();
    for (cModule::SubmoduleIterator it(network); !it.end(); ++it) {
        cModule *m = *it;
        cModule *mobM = m->getSubmodule("mobility");
        if (!mobM || m == getContainingNode(this)) continue;

        IMobility *other = check_and_cast<IMobility*>(mobM);
        if (myPos.distance(other->getCurrentPosition()) > R) continue;

        Coord u = other->getCurrentVelocity();
        double unorm = u.length();
        if (vnorm == 0 || unorm == 0) continue;

        double cosSim = (v.x*u.x + v.y*u.y + v.z*u.z) / (vnorm * unorm);
        // map [-1,1] -> [0,1]
        acc += 0.5 * (cosSim + 1.0);
        ++cnt;
    }

    if (cnt == 0) return 0.0;
    return std::clamp(acc / cnt, 0.0, 1.0);
}

// --- s4: queue fill ratio on wlan[0] ---
double NodeMetrics::queueFill()
{
    cModule *wlan0 = getContainingNode(this)->getSubmodule("wlan", 0);
    if (!wlan0) return 0.0;
    cModule *q = wlan0->getSubmodule("queue");
    if (!q) return 0.0;

    auto pq = dynamic_cast<queueing::IPacketQueue*>(q);
    if (!pq) return 0.0;

    int cap = q->par("packetCapacity");
    if (cap <= 0) return 0.0;

    int len = pq->getNumPackets();
    double ratio = (double)len / (double)cap;

    return std::clamp(ratio, 0.0, 1.0);
}

// --- s5: routing closeness (smaller avg hop distance => higher score) ---
double NodeMetrics::routingCloseness()
{
    cModule *nl = getContainingNode(this)->getSubmodule("networkLayer");
    if (!nl)
        return 0.0;

    cModule *ipv4 = nl->getSubmodule("ipv4");
    if (!ipv4)
        return 0.0;

    auto rt = ipv4->getSubmodule("routingTable")
                ? check_and_cast<Ipv4RoutingTable*>(ipv4->getSubmodule("routingTable"))
                : nullptr;
    if (!rt)
        return 0.0;

    int n = rt->getNumRoutes();
    if (n == 0)
        return 0.0;

    double acc = 0.0;
    for (int i = 0; i < n; ++i) {
        auto *route = rt->getRoute(i);
        if (!route)
            continue;

        // heuristic: INET stores hop count in metric for many protocols
        int hops = route->getMetric() > 0 ? route->getMetric() : 1;
        acc += (double)hops;
    }

    if (acc == 0.0)
        return 0.0;

    double avgHops = acc / n;                 // [1 .. ~diameter]
    double score   = 1.0 - std::min(avgHops, 10.0) / 10.0;  // normalize to [0,1]
    return std::clamp(score, 0.0, 1.0);
}


double NodeMetrics::linkHoldingTime()
{
    if (!mob) return 0.0;

        Coord v = mob->getCurrentVelocity();
        double vnorm = v.length();
        double R = readCommRange(getContainingNode(this));
        if (R <= 0) return 0.0;

        int cnt = 0;
        double acc = 0.0;
        Coord myPos = mob->getCurrentPosition();

        cModule *network = getSystemModule();
        for (cModule::SubmoduleIterator it(network); !it.end(); ++it) {
            cModule *m = *it;
            cModule *mobM = m->getSubmodule("mobility");
            if (!mobM || m == getContainingNode(this)) continue;

            IMobility *other = check_and_cast<IMobility*>(mobM);
            if (myPos.distance(other->getCurrentPosition()) > R) continue;

            Coord u = other->getCurrentVelocity();
            Coord uPos = other->getCurrentPosition();
            double unorm = u.length();
            if (vnorm == 0 || unorm == 0) continue;

            double a = v.x - u.x;
            double c = v.y - u.y;
            double b = myPos.x - uPos.x;
            double d = myPos.y - uPos.y;
            double denom = a * a + c * c;
            if (denom <= 0)     // parallel velocities, no relative motion in plane
                continue;

            // link holding time prediction:
            // LHTP = ( sqrt( (a^2 + c^2) * R^2 - (a*d - b*c)^2 ) - (a*b + c*d) ) / (a^2 + c^2)
            double ad_bc   = a * d - b * c;
            double inside  = denom * R * R - ad_bc * ad_bc;
            if (inside <= 0)
                continue;       // no future intersection with the circle

            double root = std::sqrt(inside);
            double num  = root - (a * b + c * d);
            double LHTP = num / denom;

            if (LHTP <= 0)
                continue;       // link already expiring or invalid

            acc += LHTP;
            ++cnt;
        }

        if (cnt == 0)
            return 0.0;

        return acc / cnt;
}

NodeMetrics::NodeMetrics() {
    // TODO Auto-generated constructor stub

}

NodeMetrics::~NodeMetrics() {
    // TODO Auto-generated destructor stub
}

