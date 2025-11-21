#include "ClusterMsg.h"

ClusterMsg::ClusterMsg(const char *name, Kind kind)
    : cPacket(name)
{
    kindField = kind;
}

ClusterMsg::ClusterMsg(const ClusterMsg& other)
    : cPacket(other)
{
    copy(other);
}

ClusterMsg::~ClusterMsg() = default;

ClusterMsg& ClusterMsg::operator=(const ClusterMsg& other)
{
    if (this == &other)
        return *this;
    cPacket::operator=(other);
    copy(other);
    return *this;
}

void ClusterMsg::copy(const ClusterMsg& other)
{
    kindField     = other.kindField;
    srcId         = other.srcId;
    clusterHeadId = other.clusterHeadId;
    utility       = other.utility;
    hldTime       = other.hldTime;
}

ClusterMsg *ClusterMsg::dup() const
{
    return new ClusterMsg(*this);
}

void ClusterMsg::parsimPack(cCommBuffer *b) const
{
    cPacket::parsimPack(b);
    doParsimPacking(b, (int)kindField);
    doParsimPacking(b, srcId);
    doParsimPacking(b, clusterHeadId);
    doParsimPacking(b, utility);
    doParsimPacking(b, hldTime);
}

void ClusterMsg::parsimUnpack(cCommBuffer *b)
{
    cPacket::parsimUnpack(b);
    int k;
    doParsimUnpacking(b, k);
    kindField = static_cast<Kind>(k);
    doParsimUnpacking(b, srcId);
    doParsimUnpacking(b, clusterHeadId);
    doParsimUnpacking(b, utility);
    doParsimUnpacking(b, hldTime);
}
