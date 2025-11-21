#ifndef SRC_CLUSTER_CLUSTERMSG_H
#define SRC_CLUSTER_CLUSTERMSG_H

#include "omnetpp.h"

using namespace omnetpp;

enum Kind {
    HELLO       = 1,
    CH_REQUEST  = 2,
    CH_RESPONSE = 3,
    DECLARATION = 4
};

class ClusterMsg : public cPacket
{
  public:


  protected:
    Kind   kindField = HELLO;
    int    srcId      = -1;
    int    clusterHeadId = -1;
    double utility    = 0.0;
    double hldTime    = 0.0;

  public:
    ClusterMsg(const char *name = nullptr, Kind kind = HELLO);
    ClusterMsg(const ClusterMsg& other);
    virtual ~ClusterMsg();

    ClusterMsg& operator=(const ClusterMsg& other);

    virtual ClusterMsg *dup() const override;

    // serialization (needed if you ever use parallel simulation)
    virtual void parsimPack(cCommBuffer *b) const override;
    virtual void parsimUnpack(cCommBuffer *b) override;

    // getters / setters
    void setKindField(Kind k)           { kindField = k; }
    Kind getKindField() const           { return kindField; }

    void setSrcId(int v)                { srcId = v; }
    int  getSrcId() const               { return srcId; }

    void setClusterHeadId(int v)        { clusterHeadId = v; }
    int  getClusterHeadId() const       { return clusterHeadId; }

    void setUtility(double v)           { utility = v; }
    double getUtility() const           { return utility; }

    void setHldTime(double v)           { hldTime = v; }
    double getHldTime() const           { return hldTime; }

  protected:
    void copy(const ClusterMsg& other);
};

#endif // SRC_CLUSTER_CLUSTERMSG_H
