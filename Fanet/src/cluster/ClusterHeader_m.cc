//
// Generated file, do not edit! Created by opp_msgtool 6.2 from src/cluster/ClusterHeader.msg.
//

// Disable warnings about unused variables, empty switch stmts, etc:
#ifdef _MSC_VER
#  pragma warning(disable:4101)
#  pragma warning(disable:4065)
#endif

#if defined(__clang__)
#  pragma clang diagnostic ignored "-Wshadow"
#  pragma clang diagnostic ignored "-Wconversion"
#  pragma clang diagnostic ignored "-Wunused-parameter"
#  pragma clang diagnostic ignored "-Wc++98-compat"
#  pragma clang diagnostic ignored "-Wunreachable-code-break"
#  pragma clang diagnostic ignored "-Wold-style-cast"
#elif defined(__GNUC__)
#  pragma GCC diagnostic ignored "-Wshadow"
#  pragma GCC diagnostic ignored "-Wconversion"
#  pragma GCC diagnostic ignored "-Wunused-parameter"
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#  pragma GCC diagnostic ignored "-Wsuggest-attribute=noreturn"
#  pragma GCC diagnostic ignored "-Wfloat-conversion"
#endif

#include <iostream>
#include <sstream>
#include <memory>
#include <type_traits>
#include "ClusterHeader_m.h"

namespace omnetpp {

// Template pack/unpack rules. They are declared *after* a1l type-specific pack functions for multiple reasons.
// They are in the omnetpp namespace, to allow them to be found by argument-dependent lookup via the cCommBuffer argument

// Packing/unpacking an std::vector
template<typename T, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::vector<T,A>& v)
{
    int n = v.size();
    doParsimPacking(buffer, n);
    for (int i = 0; i < n; i++)
        doParsimPacking(buffer, v[i]);
}

template<typename T, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::vector<T,A>& v)
{
    int n;
    doParsimUnpacking(buffer, n);
    v.resize(n);
    for (int i = 0; i < n; i++)
        doParsimUnpacking(buffer, v[i]);
}

// Packing/unpacking an std::list
template<typename T, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::list<T,A>& l)
{
    doParsimPacking(buffer, (int)l.size());
    for (typename std::list<T,A>::const_iterator it = l.begin(); it != l.end(); ++it)
        doParsimPacking(buffer, (T&)*it);
}

template<typename T, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::list<T,A>& l)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        l.push_back(T());
        doParsimUnpacking(buffer, l.back());
    }
}

// Packing/unpacking an std::set
template<typename T, typename Tr, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::set<T,Tr,A>& s)
{
    doParsimPacking(buffer, (int)s.size());
    for (typename std::set<T,Tr,A>::const_iterator it = s.begin(); it != s.end(); ++it)
        doParsimPacking(buffer, *it);
}

template<typename T, typename Tr, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::set<T,Tr,A>& s)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        T x;
        doParsimUnpacking(buffer, x);
        s.insert(x);
    }
}

// Packing/unpacking an std::map
template<typename K, typename V, typename Tr, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::map<K,V,Tr,A>& m)
{
    doParsimPacking(buffer, (int)m.size());
    for (typename std::map<K,V,Tr,A>::const_iterator it = m.begin(); it != m.end(); ++it) {
        doParsimPacking(buffer, it->first);
        doParsimPacking(buffer, it->second);
    }
}

template<typename K, typename V, typename Tr, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::map<K,V,Tr,A>& m)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        K k; V v;
        doParsimUnpacking(buffer, k);
        doParsimUnpacking(buffer, v);
        m[k] = v;
    }
}

// Default pack/unpack function for arrays
template<typename T>
void doParsimArrayPacking(omnetpp::cCommBuffer *b, const T *t, int n)
{
    for (int i = 0; i < n; i++)
        doParsimPacking(b, t[i]);
}

template<typename T>
void doParsimArrayUnpacking(omnetpp::cCommBuffer *b, T *t, int n)
{
    for (int i = 0; i < n; i++)
        doParsimUnpacking(b, t[i]);
}

// Default rule to prevent compiler from choosing base class' doParsimPacking() function
template<typename T>
void doParsimPacking(omnetpp::cCommBuffer *, const T& t)
{
    throw omnetpp::cRuntimeError("Parsim error: No doParsimPacking() function for type %s", omnetpp::opp_typename(typeid(t)));
}

template<typename T>
void doParsimUnpacking(omnetpp::cCommBuffer *, T& t)
{
    throw omnetpp::cRuntimeError("Parsim error: No doParsimUnpacking() function for type %s", omnetpp::opp_typename(typeid(t)));
}

}  // namespace omnetpp

Register_Class(ClusterHeader)

ClusterHeader::ClusterHeader() : ::inet::FieldsChunk()
{
}

ClusterHeader::ClusterHeader(const ClusterHeader& other) : ::inet::FieldsChunk(other)
{
    copy(other);
}

ClusterHeader::~ClusterHeader()
{
}

ClusterHeader& ClusterHeader::operator=(const ClusterHeader& other)
{
    if (this == &other) return *this;
    ::inet::FieldsChunk::operator=(other);
    copy(other);
    return *this;
}

void ClusterHeader::copy(const ClusterHeader& other)
{
    this->kind = other.kind;
    this->srcId = other.srcId;
    this->dstId = other.dstId;
    this->clusterHeadId = other.clusterHeadId;
    this->utility = other.utility;
    this->hldTime = other.hldTime;
}

void ClusterHeader::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::inet::FieldsChunk::parsimPack(b);
    doParsimPacking(b,this->kind);
    doParsimPacking(b,this->srcId);
    doParsimPacking(b,this->dstId);
    doParsimPacking(b,this->clusterHeadId);
    doParsimPacking(b,this->utility);
    doParsimPacking(b,this->hldTime);
}

void ClusterHeader::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::inet::FieldsChunk::parsimUnpack(b);
    doParsimUnpacking(b,this->kind);
    doParsimUnpacking(b,this->srcId);
    doParsimUnpacking(b,this->dstId);
    doParsimUnpacking(b,this->clusterHeadId);
    doParsimUnpacking(b,this->utility);
    doParsimUnpacking(b,this->hldTime);
}

int ClusterHeader::getKind() const
{
    return this->kind;
}

void ClusterHeader::setKind(int kind)
{
    handleChange();
    this->kind = kind;
}

int ClusterHeader::getSrcId() const
{
    return this->srcId;
}

void ClusterHeader::setSrcId(int srcId)
{
    handleChange();
    this->srcId = srcId;
}

int ClusterHeader::getDstId() const
{
    return this->dstId;
}

void ClusterHeader::setDstId(int dstId)
{
    handleChange();
    this->dstId = dstId;
}

int ClusterHeader::getClusterHeadId() const
{
    return this->clusterHeadId;
}

void ClusterHeader::setClusterHeadId(int clusterHeadId)
{
    handleChange();
    this->clusterHeadId = clusterHeadId;
}

double ClusterHeader::getUtility() const
{
    return this->utility;
}

void ClusterHeader::setUtility(double utility)
{
    handleChange();
    this->utility = utility;
}

double ClusterHeader::getHldTime() const
{
    return this->hldTime;
}

void ClusterHeader::setHldTime(double hldTime)
{
    handleChange();
    this->hldTime = hldTime;
}

class ClusterHeaderDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_kind,
        FIELD_srcId,
        FIELD_dstId,
        FIELD_clusterHeadId,
        FIELD_utility,
        FIELD_hldTime,
    };
  public:
    ClusterHeaderDescriptor();
    virtual ~ClusterHeaderDescriptor();

    virtual bool doesSupport(omnetpp::cObject *obj) const override;
    virtual const char **getPropertyNames() const override;
    virtual const char *getProperty(const char *propertyName) const override;
    virtual int getFieldCount() const override;
    virtual const char *getFieldName(int field) const override;
    virtual int findField(const char *fieldName) const override;
    virtual unsigned int getFieldTypeFlags(int field) const override;
    virtual const char *getFieldTypeString(int field) const override;
    virtual const char **getFieldPropertyNames(int field) const override;
    virtual const char *getFieldProperty(int field, const char *propertyName) const override;
    virtual int getFieldArraySize(omnetpp::any_ptr object, int field) const override;
    virtual void setFieldArraySize(omnetpp::any_ptr object, int field, int size) const override;

    virtual const char *getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const override;
    virtual std::string getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const override;
    virtual omnetpp::cValue getFieldValue(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const override;

    virtual const char *getFieldStructName(int field) const override;
    virtual omnetpp::any_ptr getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const override;
};

Register_ClassDescriptor(ClusterHeaderDescriptor)

ClusterHeaderDescriptor::ClusterHeaderDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(ClusterHeader)), "inet::FieldsChunk")
{
    propertyNames = nullptr;
}

ClusterHeaderDescriptor::~ClusterHeaderDescriptor()
{
    delete[] propertyNames;
}

bool ClusterHeaderDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<ClusterHeader *>(obj)!=nullptr;
}

const char **ClusterHeaderDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *ClusterHeaderDescriptor::getProperty(const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

int ClusterHeaderDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 6+base->getFieldCount() : 6;
}

unsigned int ClusterHeaderDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeFlags(field);
        field -= base->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,    // FIELD_kind
        FD_ISEDITABLE,    // FIELD_srcId
        FD_ISEDITABLE,    // FIELD_dstId
        FD_ISEDITABLE,    // FIELD_clusterHeadId
        FD_ISEDITABLE,    // FIELD_utility
        FD_ISEDITABLE,    // FIELD_hldTime
    };
    return (field >= 0 && field < 6) ? fieldTypeFlags[field] : 0;
}

const char *ClusterHeaderDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldName(field);
        field -= base->getFieldCount();
    }
    static const char *fieldNames[] = {
        "kind",
        "srcId",
        "dstId",
        "clusterHeadId",
        "utility",
        "hldTime",
    };
    return (field >= 0 && field < 6) ? fieldNames[field] : nullptr;
}

int ClusterHeaderDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "kind") == 0) return baseIndex + 0;
    if (strcmp(fieldName, "srcId") == 0) return baseIndex + 1;
    if (strcmp(fieldName, "dstId") == 0) return baseIndex + 2;
    if (strcmp(fieldName, "clusterHeadId") == 0) return baseIndex + 3;
    if (strcmp(fieldName, "utility") == 0) return baseIndex + 4;
    if (strcmp(fieldName, "hldTime") == 0) return baseIndex + 5;
    return base ? base->findField(fieldName) : -1;
}

const char *ClusterHeaderDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeString(field);
        field -= base->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "int",    // FIELD_kind
        "int",    // FIELD_srcId
        "int",    // FIELD_dstId
        "int",    // FIELD_clusterHeadId
        "double",    // FIELD_utility
        "double",    // FIELD_hldTime
    };
    return (field >= 0 && field < 6) ? fieldTypeStrings[field] : nullptr;
}

const char **ClusterHeaderDescriptor::getFieldPropertyNames(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldPropertyNames(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

const char *ClusterHeaderDescriptor::getFieldProperty(int field, const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldProperty(field, propertyName);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

int ClusterHeaderDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    ClusterHeader *pp = omnetpp::fromAnyPtr<ClusterHeader>(object); (void)pp;
    switch (field) {
        default: return 0;
    }
}

void ClusterHeaderDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    ClusterHeader *pp = omnetpp::fromAnyPtr<ClusterHeader>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'ClusterHeader'", field);
    }
}

const char *ClusterHeaderDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    ClusterHeader *pp = omnetpp::fromAnyPtr<ClusterHeader>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string ClusterHeaderDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    ClusterHeader *pp = omnetpp::fromAnyPtr<ClusterHeader>(object); (void)pp;
    switch (field) {
        case FIELD_kind: return long2string(pp->getKind());
        case FIELD_srcId: return long2string(pp->getSrcId());
        case FIELD_dstId: return long2string(pp->getDstId());
        case FIELD_clusterHeadId: return long2string(pp->getClusterHeadId());
        case FIELD_utility: return double2string(pp->getUtility());
        case FIELD_hldTime: return double2string(pp->getHldTime());
        default: return "";
    }
}

void ClusterHeaderDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    ClusterHeader *pp = omnetpp::fromAnyPtr<ClusterHeader>(object); (void)pp;
    switch (field) {
        case FIELD_kind: pp->setKind(string2long(value)); break;
        case FIELD_srcId: pp->setSrcId(string2long(value)); break;
        case FIELD_dstId: pp->setDstId(string2long(value)); break;
        case FIELD_clusterHeadId: pp->setClusterHeadId(string2long(value)); break;
        case FIELD_utility: pp->setUtility(string2double(value)); break;
        case FIELD_hldTime: pp->setHldTime(string2double(value)); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'ClusterHeader'", field);
    }
}

omnetpp::cValue ClusterHeaderDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    ClusterHeader *pp = omnetpp::fromAnyPtr<ClusterHeader>(object); (void)pp;
    switch (field) {
        case FIELD_kind: return pp->getKind();
        case FIELD_srcId: return pp->getSrcId();
        case FIELD_dstId: return pp->getDstId();
        case FIELD_clusterHeadId: return pp->getClusterHeadId();
        case FIELD_utility: return pp->getUtility();
        case FIELD_hldTime: return pp->getHldTime();
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'ClusterHeader' as cValue -- field index out of range?", field);
    }
}

void ClusterHeaderDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    ClusterHeader *pp = omnetpp::fromAnyPtr<ClusterHeader>(object); (void)pp;
    switch (field) {
        case FIELD_kind: pp->setKind(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_srcId: pp->setSrcId(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_dstId: pp->setDstId(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_clusterHeadId: pp->setClusterHeadId(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_utility: pp->setUtility(value.doubleValue()); break;
        case FIELD_hldTime: pp->setHldTime(value.doubleValue()); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'ClusterHeader'", field);
    }
}

const char *ClusterHeaderDescriptor::getFieldStructName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructName(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    };
}

omnetpp::any_ptr ClusterHeaderDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    ClusterHeader *pp = omnetpp::fromAnyPtr<ClusterHeader>(object); (void)pp;
    switch (field) {
        default: return omnetpp::any_ptr(nullptr);
    }
}

void ClusterHeaderDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    ClusterHeader *pp = omnetpp::fromAnyPtr<ClusterHeader>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'ClusterHeader'", field);
    }
}

namespace omnetpp {

}  // namespace omnetpp

