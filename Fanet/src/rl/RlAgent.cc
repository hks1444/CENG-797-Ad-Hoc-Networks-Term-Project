#include "src/rl/RlAgent.h"
#include "inet/power/contract/IEpEnergyStorage.h"

Define_Module(RlAgent);

void RlAgent::initialize()
{
    updatePeriod = par("updatePeriod");
    tick = new cMessage("rlTick");
    scheduleAt(simTime() + updatePeriod, tick);

    // locate NodeMetrics on same node
    cModule *node = getContainingNode(this);
    cModule *m = node->getSubmodule("metrics");
    metrics = m ? check_and_cast<NodeMetrics*>(m) : nullptr;

    initActionSpace();

    // initial energy snapshot
    lastEnergy = residualEnergy();
}

void RlAgent::initActionSpace()
{
    int n = par("numActions");
    actions.clear();
    actions.reserve(n);

    // Example: 4 fixed weight sets, all normalized (sum = 1)
    // You can change these to match the paper’s preferred initial policies.
    actions.push_back({0.4, 0.2, 0.1, 0.1, 0.1, 0.1}); // energy-heavy
    actions.push_back({0.2, 0.4, 0.1, 0.1, 0.1, 0.1}); // centrality-heavy
    actions.push_back({0.2, 0.2, 0.2, 0.2, 0.1, 0.1}); // balanced
    actions.push_back({0.1, 0.2, 0.2, 0.1, 0.2, 0.2}); // mobility/link heavy

    // ensure we have at least numActions definitions
    while ((int)actions.size() < n)
        actions.push_back(actions.back());
}

void RlAgent::handleMessage(cMessage *msg)
{
    if (msg == tick) {
        onUpdate();
        scheduleAt(simTime() + updatePeriod, tick);
    } else {
        delete msg;
    }
}

void RlAgent::finish()
{
    if (tick) {
        cancelAndDelete(tick);
        tick = nullptr;
    }
}

double RlAgent::residualEnergy() const
{
    cModule *node = getContainingNode(this);
    if (!node) return 0.0;

    cModule *e = node->getSubmodule("energyStorage");
    if (!e) return 0.0;

    auto bat = dynamic_cast<power::IEpEnergyStorage*>(e);
    if (!bat) return 0.0;

    double cap = bat->getNominalEnergyCapacity().get();
    double rem = bat->getResidualEnergyCapacity().get();
    if (cap <= 0) return 0.0;

    return rem / cap; // [0,1]
}

void RlAgent::onUpdate()
{
    // 1) compute reward components over last window
    double Rc = computeRc();
    double Ec = computeEc();
    double Cf = computeCf();

    // 2) compute reward using paper formula
    double reward = computeReward(Rc, Ec, Cf);
    recordScalar("RlReward", reward);

    // 3) update policy / choose next action (placeholder)
    updatePolicy(reward);

    // 4) reset window counters
    roleChangesInWindow = 0;
    confirmationsInWindow = 0;
}

double RlAgent::computeRc() const
{
    // paper:
    // Rc = 1  if number of node changing its role with frequency < ∞   (practically: stable)
    // Rc = -1 otherwise
    // Here: if roleChangesInWindow == 0 -> stable
    return (roleChangesInWindow == 0) ? 1.0 : -1.0;
}

double RlAgent::computeEc() const
{
    // Ec: decline in energy over last T
    double nowE = const_cast<RlAgent*>(this)->residualEnergy();
    double delta = nowE - lastEnergy; // negative means energy dropped
    // Map energy drop to [0,1], 1 = no drop, 0 = large drop
    double drop = std::clamp(-delta, 0.0, 1.0);
    return 1.0 - drop;
}

double RlAgent::computeCf() const
{
    // Cf = 1 if confirmations > 0 else 0
    return (confirmationsInWindow > 0) ? 1.0 : 0.0;
}

double RlAgent::computeReward(double Rc, double Ec, double Cf) const
{
    const auto& w = actions[currentAction];

    // r = w1 * Rc + w2 * Ec + w3 * Cf
    double r = w[0] * Rc + w[1] * Ec + w[2] * Cf;

    // RPN = (1 − e^{−3r}) / (1 + e^{3r})
    double num = 1.0 - std::exp(-3.0 * r);
    double den = 1.0 + std::exp( 3.0 * r);
    if (den == 0.0) return 0.0;

    double RPN = num / den;

    // Reward per paper: Reward = RPN / #(nodes.)
    // Per-node module cannot easily divide by global node count here,
    // so you can either:
    //  - just use RPN as “local reward”
    //  - or provide nodeCount as a global parameter.
    return RPN;
}

double RlAgent::getClusterUtility() const
{
    if (!metrics)
        return 0.0;

    const auto& s = metrics->getLastUtilities();
    const auto& w = actions[currentAction];

    double score = 0.0;
    for (int i = 0; i < 6 && i < (int)s.size(); ++i)
        score += w[i] * s[i];

    return score;
}

void RlAgent::updatePolicy(double reward)
{
    // Placeholder: greedy over fixed action set based on current reward.
    // For now, just keep the same action; implement Q-learning later.
    // Example of very simple explore-exploit could be added here.

    // Example stub:
    // recordScalar("currentAction", currentAction);
    (void)reward;
}
