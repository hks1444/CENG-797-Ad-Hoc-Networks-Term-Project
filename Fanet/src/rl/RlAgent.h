#ifndef SRC_RL_RLAGENT_H_
#define SRC_RL_RLAGENT_H_

#include "inet/common/INETDefs.h"
#include "src/metrics/NodeMetrics.h"
#include <vector>
using namespace inet;

class RlAgent : public cSimpleModule
{
  protected:
    cMessage *tick = nullptr;
    simtime_t updatePeriod;

    NodeMetrics *metrics = nullptr;

    // current action: index into predefined weight sets
    int currentAction = 0;

    // per-node weight vectors for each action option
    std::vector<std::vector<double >> actions;

    // reward components history
    double lastEnergy = 0.0;
    int    roleChangesInWindow = 0;
    int    confirmationsInWindow = 0;

    int currentRole = 0; // 0 = normal, 1 = CH (example)

  public:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;

    // RL plumbing
    void initActionSpace();
    void onUpdate();

    double computeRc() const;
    double computeEc() const;
    double computeCf() const;

    double computeReward(double Rc, double Ec, double Cf) const;
    void   updatePolicy(double reward);
    double getClusterUtility() const;
    double residualEnergy() const;
};

#endif
