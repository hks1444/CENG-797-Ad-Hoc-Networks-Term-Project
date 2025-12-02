#ifndef SRC_RL_RLAGENT_H_
#define SRC_RL_RLAGENT_H_

#include "inet/common/INETDefs.h"
#include "src/metrics/NodeMetrics.h"
#include "src/cluster/roles.h"
#include <vector>

using namespace inet;

class RlAgent : public cSimpleModule
{
  protected:
    // timing
    cMessage *tick = nullptr;
    simtime_t updatePeriod;
    simtime_t CHLastHeard;
    // links to metrics
    NodeMetrics *metrics = nullptr;
    double max_energy_drop;
    // discrete state/action spaces
    int numStates = 1;
    int numActions = 0;
    int CHWindow = 0;

    // Q-learning hyperparameters
    double alpha   = 0.1;   // learning rate
    double gamma   = 0.9;   // discount factor
    double epsilon = 0.1;   // exploration prob.

    // current RL configuration
    int currentState  = 0;
    int currentAction = 0;

    // per-node weight vectors for each action option
    // each action is a 6-D weight vector w = <w1..w6>, sum ~ 1
    std::vector<std::vector<double>> actions;

    // Q[s,a], R[s,a], N[s,a]
    std::vector<std::vector<double>> Q;
    std::vector<std::vector<double>> Ravg;
    std::vector<std::vector<int>>    Nsa;

    // reward components history
    double lastEnergy = 0.0;
    int    roleChangesInWindow     = 0;
    int    confirmationsInWindow   = 0;
    int    currentRole             = 0; // 0 = non-CH, 1 = CH (for Rc)

    // NEW: per-node time series logging
    cOutVector rewardVector;
    cOutVector actionVector;
    cOutVector stateVector;
    cOutVector RcVector;
    cOutVector EcVector;
    cOutVector CfVector;

  public:
    // lifecycle
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;

    // RL plumbing
    void initActionSpace();
    void initQTable();
    void onUpdate();

    // reward components and reward
    double computeRc() const;
    double computeEc();
    double computeCf() const;
    double computeReward(double Rc, double Ec, double Cf) const;

    // Q-learning helpers
    int    computeStateIndex() const;
    int    selectActionEpsGreedy(int state);
    void   bellmanUpdate(int s, int a, int sNext, double reward);

    // cluster utility for ClusterApp
    double getClusterUtility() const;

    // energy access
    double residualEnergy() const;

    // hooks from ClusterApp (local node only)
    void notifyRoleChange(int newRole);         // increments Rc counter
    void notifyClusterConfirmation();           // increments Cf counter
    void reportCHLastHeard();
    // optional: expose Q-table for logging/inspection
    const std::vector<std::vector<double>>& getQTable() const { return Q; }
};

#endif
