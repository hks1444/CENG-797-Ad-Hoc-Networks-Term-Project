#include "inet/physicallayer/wireless/common/base/packetlevel/FlatRadioBase.h"
#include "inet/common/INETDefs.h"
#include "inet/common/ModuleAccess.h"
#include "inet/power/contract/IEpEnergyStorage.h"
#include "inet/queueing/contract/IPacketQueue.h"

using namespace inet;

class EnergyController : public cSimpleModule, public cListener
{
  protected:
    power::IEpEnergyStorage *bat = nullptr;
    physicallayer::IRadio *radio = nullptr;
    bool off = false;
    double offThreshold = 0.0; // in [0,1]

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override { delete msg; }

    // we only care about the double-valued signal
    virtual void receiveSignal(cComponent *src, simsignal_t id, double value, cObject *details) override;
};

Define_Module(EnergyController);

void EnergyController::initialize()
{
    offThreshold = par("offThreshold").doubleValue();

    cModule *node = getContainingNode(this);

    // energy storage on the node
    cModule *e = node->getSubmodule("energyStorage");
    if (e) {
        bat = check_and_cast<power::IEpEnergyStorage*>(e);  // interface
        e->subscribe("residualEnergyCapacityChanged", this); // <- subscribe on module
    }

    // wlan[0].radio
    if (auto *wlan0 = node->getSubmodule("wlan", 0)) {
        if (auto *r = wlan0->getSubmodule("radio"))
            radio = check_and_cast<physicallayer::IRadio*>(r);
    }
}

void EnergyController::receiveSignal(cComponent *src, simsignal_t id, double value, cObject *details)
{
    if (!bat || !radio || off)
        return;

    double cap = bat->getNominalEnergyCapacity().get();      // J
    double rem = bat->getResidualEnergyCapacity().get();     // J
    if (cap <= 0)
        return;

    double frac = rem / cap;
    if (frac <= offThreshold) {
        off = true;

        EV_WARN << "EnergyController: node " << getContainingNode(this)->getFullPath()
                << " reached energy threshold, turning radio OFF\n";

        radio->setRadioMode(physicallayer::IRadio::RADIO_MODE_OFF);

        // optional: flush MAC queue
        if (auto *wlan0 = getContainingNode(this)->getSubmodule("wlan", 0)) {
            if (auto *queue = wlan0->getSubmodule("queue")) {
                auto pq = dynamic_cast<queueing::IPacketQueue *>(queue);
                if (pq) {
                    while (pq->getNumPackets() > 0) {
                        auto pkt = pq->dequeuePacket();
                        delete pkt; // drop queued packets
                    }
                }
            }
        }
    }
}
