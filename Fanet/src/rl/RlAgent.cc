#include "src/rl/RlAgent.h"
#include "inet/power/contract/IEpEnergyStorage.h"
#include <algorithm>
#include <cmath>

Define_Module(RlAgent);

void RlAgent::initialize()
{
    updatePeriod = par("updatePeriod");
    alpha        = par("alpha");
    gamma        = par("gamma");
    epsilon      = par("epsilon");
    numActions   = par("numActions");
    numStates    = par("numStates");

    tick = new cMessage("rlTick");
    scheduleAt(simTime() + updatePeriod, tick);

    // locate NodeMetrics on same node
    cModule *node = getContainingNode(this);
    cModule *m = node->getSubmodule("metrics");

    metrics = m ? check_and_cast<NodeMetrics*>(m) : nullptr;

    initActionSpace();
    initQTable();

    // initial state, action, and energy snapshot
    currentState  = computeStateIndex();
    currentAction = 0;               // equal-weights action
    lastEnergy    = residualEnergy();

    rewardVector.setName("rlReward");
    actionVector.setName("rlAction");
    stateVector.setName("rlState");
    RcVector.setName("Rc");
    EcVector.setName("Ec");
    CfVector.setName("Cf");
}

void RlAgent::initActionSpace()
{
    // NOTE: action = <w1..w6>, each wi in [0,1], sum ≈ 1
    // First action: equal weights as in paper Q[s, <1/6,...>] = 1
    actions.clear();
    std::vector<double> equalW(NUM_METRICS, 1.0 / NUM_METRICS);
    actions.push_back(equalW);  // a=0

    // Some additional fixed policies (examples, can tune):
    actions.push_back({0.4, 0.2, 0.1, 0.1, 0.1, 0.1}); // energy-heavy
    actions.push_back({0.2, 0.4, 0.1, 0.1, 0.1, 0.1}); // centrality-heavy
    actions.push_back({0.1, 0.2, 0.2, 0.1, 0.2, 0.2}); // mobility/link-heavy

    // ensure we have at least numActions actions
    while ((int)actions.size() < numActions)
        actions.push_back(actions.back());

    // clip to numActions
    if ((int)actions.size() > numActions)
        actions.resize(numActions);
}


void RlAgent::initQTable()
{
    if (numStates <= 0)  numStates  = 1;
    if (numActions <= 0) numActions = (int)actions.size();

    Q.assign(numStates,    std::vector<double>(numActions, 0.0));
    Ravg.assign(numStates, std::vector<double>(numActions, 0.0));
    Nsa.assign(numStates,  std::vector<int>(numActions,    0));

    // Initialization: Q[s, equalWeightsAction] = 1 for all states
    for (int a = 0; a < numActions; a++){
        for (int s = 0; s < numStates; ++s)
            Q[s][a] = 1.0;
    }
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

// --- state encoding -------------------------------------------------

int RlAgent::computeStateIndex() const
{
    if (!metrics || numStates <= 1)
        return 0;

    const auto& s = metrics->getLastUtilities();
    if (s.empty())
        return 0;

    double H = entropy(s);                   // normalized to [0,1]
    double norm = std::clamp(H, 0.0, 0.999999);

    double binWidth = 1.0 / numStates;

    int idx = (int)(norm / binWidth);
    if (idx < 0) idx = 0;
    if (idx >= numStates) idx = numStates - 1;

    return idx;
}

// --- reward components ----------------------------------------------

double RlAgent::residualEnergy() const
{
    if (!metrics || actions.empty())
       return 0.0;

    const auto& s = metrics->getLastUtilities();

    return s[0]; // [0,1]
}

double RlAgent::computeRc() const
{
    // Rc = 1 if no role change during last window, otherwise -1
    return (roleChangesInWindow < 2) ? 1.0 : -1.0;
}

double RlAgent::computeEc()
{
    double nowE = const_cast<RlAgent*>(this)->residualEnergy();
    double delta = lastEnergy - nowE;
    if(delta > max_energy_drop){
        max_energy_drop = delta;
        delta = 1;
    }else{
        delta = delta/max_energy_drop;
    }
    double drop = std::clamp(delta, 0.0, 1.0);  // [0,1]
    return 1.0 - drop;                           // 1: no drop, 0: large drop
}

void RlAgent::reportCHLastHeard(){
    CHLastHeard = simTime();
}

double RlAgent::computeCf() const
{
    if(currentRole == ROLE_CH){
        return (confirmationsInWindow > 0) ? 1 : -1;
    }else if(currentRole == ROLE_MEMBER){
        return (simTime() - CHLastHeard - 1 > TIMEOUT_VALUE) ? 1 : -1;
    }else{
        return -1;
    }
}

double RlAgent::computeReward(double Rc, double Ec, double Cf) const
{
    // r = w1 * Rc + w2 * Ec + w3 * Cf
    const auto& w = actions[currentAction];

    double r = 0.0;
    if (!w.empty()) {
        double w1 = w.size() > 0 ? w[0] : 0.0;
        double w2 = w.size() > 1 ? w[1] : 0.0;
        double w3 = w.size() > 2 ? w[2] : 0.0;
        double w4 = w.size() > 3 ? w[3] : 0.0;
        double w5 = w.size() > 4 ? w[4] : 0.0;
        double w6 = w.size() > 5 ? w[5] : 0.0;
        r = (w1+w4) * Rc + (w2+w5) * Ec + (w3+w6) * Cf;
    }

    // RPN = (1 − e^{−3r}) / (1 + e^{3r})
    double num = 1.0 - std::exp(-3.0 * r);
    double den = 1.0 + std::exp( 3.0 * r);
    if (den == 0.0) return 0.0;

    double RPN = r; //num / den;

    // Local version: use RPN as per-node reward
    return RPN;
}

// --- Q-learning core ------------------------------------------------

int RlAgent::selectActionEpsGreedy(int state)
{
    if (numActions <= 0)
        return 0;

    double u = uniform(0, 1);
    if (u < epsilon) {
        // exploration
        return intrand(numActions);
    }

    // exploitation: argmax_a Q[state][a]
    int bestA = 0;
    double bestQ = Q[state][0];
    for (int a = 1; a < numActions; ++a) {
        if (Q[state][a] > bestQ) {
            bestQ = Q[state][a];
            bestA = a;
        }
    }
    return bestA;
}

void RlAgent::bellmanUpdate(int s, int a, int sNext, double reward)
{
    if (s < 0 || s >= numStates)  return;
    if (a < 0 || a >= numActions) return;
    if (sNext < 0 || sNext >= numStates) sNext = s;

    // Maintain running average R[s,a]
    Nsa[s][a] += 1;
    double n = (double)Nsa[s][a];
    double oldR = Ravg[s][a];
    double newR = oldR + (reward - oldR) / n;
    Ravg[s][a] = newR;

    // Bellman: Q[s,a] <- (1-α)Q[s,a] + α*( newR + γ max_a' Q[sNext,a'] )
    double maxNext = *std::max_element(Q[sNext].begin(), Q[sNext].end());
    double target  = newR + gamma * maxNext;
    Q[s][a]        = (1.0 - alpha) * Q[s][a] + alpha * target;
}

void RlAgent::onUpdate()
{
    // encode next state from current utilities
    int nextState = computeStateIndex();

    // reward over last period T
    double Rc = computeRc();
    double Ec = computeEc();
    double Cf = computeCf();

    RcVector.record(1-Rc);
    EcVector.record(Ec);
    CfVector.record(Cf);

    double reward = computeReward(Rc, Ec, Cf);

    // scalar + vector log
    recordScalar("RlReward", reward);
    rewardVector.record(reward);
    stateVector.record(currentState);   // log state *before* transition
    actionVector.record(currentAction); // log action used in this window


    // local Q-learning update for (currentState, currentAction) -> nextState
    bellmanUpdate(currentState, currentAction, nextState, reward);

    int oldAction = currentAction;
    // select next action for next period
    currentState  = nextState;
    currentAction = selectActionEpsGreedy(currentState);

    // optional: log action-change events in text
//    if (oldAction != currentAction) {
//        std::cout << "RL node=" << getContainingNode(this)->getFullPath()
//                << " t=" << simTime()
//                << " state=" << currentState
//                << " actionChange " << oldAction << " -> " << currentAction
//                << " reward=" << reward << std::endl;
//    }

    // reset sliding window counters
    lastEnergy = residualEnergy();
    if(CHWindow == 0){
        roleChangesInWindow   = 0;
    }
    confirmationsInWindow = 0;
    CHWindow = (CHWindow + 1) % 10;
}

// --- interaction with ClusterApp -----------------------------------

double RlAgent::getClusterUtility() const
{
    if (!metrics || actions.empty())
        return 0.0;

    const auto& s = metrics->getLastUtilities();
    const auto& w = actions[currentAction];

    int L = std::min((int)s.size(), (int)w.size());
    double score = 0.0;
    for (int i = 0; i < L; ++i)
        score += w[i] * s[i];

    return score;
}

void RlAgent::notifyRoleChange(int newRole)
{
    if (newRole != currentRole) {
        currentRole = newRole;
        roleChangesInWindow++;
    }
}

void RlAgent::notifyClusterConfirmation()
{
    /*
     *  Cf, which represents the number of confir-
mation messages received after delivering the packet through
the corresponding node.
        Currently app acks are not considered.
     * */
    confirmationsInWindow++;
}
