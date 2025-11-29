#ifndef SRC_METRICS_NODEMETRICS_H_
#define SRC_METRICS_NODEMETRICS_H_
#pragma once
#include "inet/common/INETDefs.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/geometry/common/Coord.h"
#include "inet/mobility/contract/IMobility.h"
#include "inet/power/contract/IEnergyStorage.h"
#include "inet/power/contract/IEpEnergyStorage.h"
#include "inet/queueing/contract/IPacketQueue.h"
#include "inet/networklayer/ipv4/Ipv4RoutingTable.h"
#define NUM_METRICS 6

using namespace inet;

struct WeightVector {
    double w[NUM_METRICS];
};

// entropy over utilities (non-zero guarded)
inline double entropy(const std::vector<double>& s)
{
    if (s.empty())
        return 0.0;

    double sum = 0.0;
    for (double x : s)
        sum += std::max(0.0, x);

    if (sum <= 0.0)
        return 0.0;

    double H = 0.0;
    for (double x : s) {
        double p = std::max(0.0, x) / sum;
        if (p > 0.0)
            H -= p * std::log2(p);
    }

    if (s.size() < 2)
        return 0.0;

    double Hmax = std::log2(static_cast<double>(s.size()));
    return (Hmax > 0.0) ? (H / Hmax) : 0.0;
}


class NodeMetrics : public cSimpleModule
{
  public:
    NodeMetrics();
    virtual ~NodeMetrics();
    const std::vector<double>& getLastUtilities() const { return lastS; }
    double getLastLinkHoldingTime() const { return lastLht; }
  protected:
    cMessage *tick = nullptr;
    simtime_t period;

    IMobility *mob = nullptr;
    power::IEpEnergyStorage *bat = nullptr;

    simsignal_t stateSignal;
    std::vector<double> lastS;
    double lastLht = 0.0;

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;

    void sample();

    // --- utility components (examples) ---
    // s1: residual energy (0..1)
    double residualEnergy();

    // s2: degree centrality (neighbors count normalized)
    double degreeCentrality();

    // s3: velocity similarity with neighbors (0..1)
    double velocitySimilarity();

    // s4: queue fill ratio on wlan[0] (0..1)
    double queueFill();

    // s5: average hop distance to all known routes (normalized)
    double routingCloseness();

    // s6: placeholder (e.g., link success over window) -> return 0..1, or 0 if unused
    double linkHoldingTime();
};


#endif /* SRC_METRICS_NODEMETRICS_H_ */
