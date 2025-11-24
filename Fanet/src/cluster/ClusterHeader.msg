import inet.common.packet.chunk.Chunk;

class ClusterHeader extends inet::FieldsChunk
{
    int kind;          // 0=HELLO, 1=CH_REQUEST, 2=CH_RESPONSE, 3=DECLARATION
    int srcId;         // sender node index
    int dstId;         // -1=broadcast, otherwise target node index
    int clusterHeadId; // CH id (or -1)
    double utility;    // sender’s cluster utility
    double hldTime;    // sender’s link-holding-time metric
}