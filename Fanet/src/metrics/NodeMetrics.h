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

using namespace inet;

class NodeMetrics : public cSimpleModule
{
  public:
    NodeMetrics();
    virtual ~NodeMetrics();
  protected:
    cMessage *tick = nullptr;
    simtime_t period;

    IMobility *mob = nullptr;
    power::IEpEnergyStorage *bat = nullptr;

    simsignal_t stateSignal;

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

    // entropy over utilities (non-zero guarded)
    double entropy(const std::vector<double>& s);
};


#endif /* SRC_METRICS_NODEMETRICS_H_ */
