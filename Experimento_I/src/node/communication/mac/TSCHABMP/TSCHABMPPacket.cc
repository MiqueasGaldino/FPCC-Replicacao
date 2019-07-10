#include <iostream>
#include <sstream>
#include "TSCHABMPPacket.h"

USING_NAMESPACE

// Another default rule (prevents compiler from choosing base class' doPacking())
template<typename T>
void doPacking(cCommBuffer *, T& t) {
    throw cRuntimeError("Parsim error: no doPacking() function for type %s or its base class (check .msg and _m.cc/h files!)",opp_typename(typeid(t)));
}

template<typename T>
void doUnpacking(cCommBuffer *, T& t) {
    throw cRuntimeError("Parsim error: no doUnpacking() function for type %s or its base class (check .msg and _m.cc/h files!)",opp_typename(typeid(t)));
}


// Template rule for outputting std::vector<T> types
template<typename T, typename A>
inline std::ostream& operator<<(std::ostream& out, const std::vector<T,A>& vec)
{
    out.put('{');
    for(typename std::vector<T,A>::const_iterator it = vec.begin(); it != vec.end(); ++it)
    {
        if (it != vec.begin()) {
            out.put(','); out.put(' ');
        }
        out << *it;
    }
    out.put('}');
    
    char buf[32];
    sprintf(buf, " (size=%u)", (unsigned int)vec.size());
    out.write(buf, strlen(buf));
    return out;
}

// Template rule which fires if a struct or class doesn't have operator<<
template<typename T>
inline std::ostream& operator<<(std::ostream& out,const T&) {return out;}

EXECUTE_ON_STARTUP(
    cEnum *e = cEnum::find("TSCHABMPFrameTypeDef");
    if (!e) enums.getInstance()->add(e = new cEnum("TSCHABMPFrameTypeDef"));
    e->insert(DATA_FRAME, "DATA_FRAME");
    e->insert(BEACON_FRAME, "BEACON_FRAME");
    e->insert(ACK_FRAME, "ACK_FRAME");
);

Register_Class(TSCHABMPPacket);

TSCHABMPPacket::TSCHABMPPacket(const char *name, int kind) : ::MacPacket(name,kind)
{
    this->ackReq = 0;
	this->ack_bitmap = 0;
	this->channel = new int[QTDNODES];
 	for(int i = 0; i < QTDNODES; i++) {
		this->channel[i] = 0;
	}
 	this->beacon_id = 0;
	this->blacklist = 0x0F;
	this->first_ch = 0;
	this->b0_error = 0;
	this->change_bl = 0;
}

TSCHABMPPacket::TSCHABMPPacket(const TSCHABMPPacket& other) : ::MacPacket(other)
{
    this->channel = new int[QTDNODES];
	copy(other);
}

TSCHABMPPacket::~TSCHABMPPacket()
{
}

TSCHABMPPacket& TSCHABMPPacket::operator=(const TSCHABMPPacket& other)
{
    if (this==&other) return *this;
    ::MacPacket::operator=(other);
    copy(other);
    return *this;
}

void TSCHABMPPacket::copy(const TSCHABMPPacket& other)
{
    this->ackReq = other.ackReq;
	this->ack_bitmap = other.ack_bitmap;
	this->kind_tschabmp = other.kind_tschabmp;
	for(int i = 0; i < QTDNODES; i++)
		this->channel[i] = other.channel[i];
	this->beacon_id = other.beacon_id;
	this->blacklist = other.blacklist;
	this->first_ch = other.first_ch;
	this->b0_error = other.b0_error;
	this->change_bl = other.change_bl;
}

void TSCHABMPPacket::parsimPack(cCommBuffer *b)
{
    ::MacPacket::parsimPack(b);
    doPacking(b,this->ackReq);
}

void TSCHABMPPacket::parsimUnpack(cCommBuffer *b)
{
    ::MacPacket::parsimUnpack(b);
    doUnpacking(b,this->ackReq);
}

bool TSCHABMPPacket::getAckReq() const
{
    return ackReq;
}

void TSCHABMPPacket::setAckReq(bool ackReq)
{
    this->ackReq = ackReq;
}

int TSCHABMPPacket::getAckBitmap() const
{
    return ack_bitmap;
}

void TSCHABMPPacket::setAckBitmap(int ack_bitmap)
{
    this->ack_bitmap = ack_bitmap;
}

int *TSCHABMPPacket::getChannel() const
{
    return channel;
}

void TSCHABMPPacket::setChannel(int *channel)
{
	for(int i = 0; i < QTDNODES; i++)
    	this->channel[i] = channel[i];
}

void TSCHABMPPacket::setBeaconId(int beacon_id) {
	this->beacon_id = beacon_id;
}

int TSCHABMPPacket::getBeaconId() const{
	return beacon_id;
}

void TSCHABMPPacket::setBlacklist(int blacklist) {
	this->blacklist = blacklist;
}

int TSCHABMPPacket::getBlacklist() const{
	return blacklist;
}

void TSCHABMPPacket::setFirstch(int first_ch) {
	this->first_ch = first_ch;
}

int TSCHABMPPacket::getFirstch() const{
	return first_ch;
}

void TSCHABMPPacket::setB0error(int b0_error) {
	this->b0_error = b0_error;
}

int TSCHABMPPacket::getB0error() const{
	return b0_error;
}

void TSCHABMPPacket::setChangebl(int change_bl) {
	this->change_bl = change_bl;
}

int TSCHABMPPacket::getChangebl() const{
	return change_bl;
}

void TSCHABMPPacket::setKindTSCHABMP(int kind_tschabmp) {
	this->kind_tschabmp = kind_tschabmp;
}

int TSCHABMPPacket::getKindTSCHABMP() {
	return this->kind_tschabmp;
}

class TSCHABMPPacketDescriptor : public cClassDescriptor
{
  public:
    TSCHABMPPacketDescriptor();
    virtual ~TSCHABMPPacketDescriptor();

    virtual bool doesSupport(cObject *obj) const;
    virtual const char *getProperty(const char *propertyname) const;
    virtual int getFieldCount(void *object) const;
    virtual const char *getFieldName(void *object, int field) const;
    virtual int findField(void *object, const char *fieldName) const;
    virtual unsigned int getFieldTypeFlags(void *object, int field) const;
    virtual const char *getFieldTypeString(void *object, int field) const;
    virtual const char *getFieldProperty(void *object, int field, const char *propertyname) const;
    virtual int getArraySize(void *object, int field) const;

    virtual std::string getFieldAsString(void *object, int field, int i) const;
    virtual bool setFieldAsString(void *object, int field, int i, const char *value) const;

    virtual const char *getFieldStructName(void *object, int field) const;
    virtual void *getFieldStructPointer(void *object, int field, int i) const;
};

Register_ClassDescriptor(TSCHABMPPacketDescriptor);

TSCHABMPPacketDescriptor::TSCHABMPPacketDescriptor() : cClassDescriptor("TSCHABMPPacket", "MacPacket")
{
}

TSCHABMPPacketDescriptor::~TSCHABMPPacketDescriptor()
{
}

bool TSCHABMPPacketDescriptor::doesSupport(cObject *obj) const
{
    return dynamic_cast<TSCHABMPPacket *>(obj)!=NULL;
}

const char *TSCHABMPPacketDescriptor::getProperty(const char *propertyname) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? basedesc->getProperty(propertyname) : NULL;
}

int TSCHABMPPacketDescriptor::getFieldCount(void *object) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? 1+basedesc->getFieldCount(object) : 1;
}

unsigned int TSCHABMPPacketDescriptor::getFieldTypeFlags(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldTypeFlags(object, field);
        field -= basedesc->getFieldCount(object);
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,
    };
    return (field>=0 && field<1) ? fieldTypeFlags[field] : 0;
}

const char *TSCHABMPPacketDescriptor::getFieldName(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldName(object, field);
        field -= basedesc->getFieldCount(object);
    }
    static const char *fieldNames[] = {
        "ackReq","kind_tschabmp","ack_bitmap", "channel", "beacon_id", "blacklist", "first_ch", "b0_error", "change_bl"
    };
    return (field>=0 && field<1) ? fieldNames[field] : NULL;
}

int TSCHABMPPacketDescriptor::findField(void *object, const char *fieldName) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    int base = basedesc ? basedesc->getFieldCount(object) : 0;
    if (fieldName[0]=='f' && (strcmp(fieldName, "ackReq")==0 || strcmp(fieldName, "kind_tschabmp")==0 || strcmp(fieldName, "ack_bitmap")==0 || strcmp(fieldName, "channel")==0 || strcmp(fieldName, "beacon_id")==0 || strcmp(fieldName, "blacklist")==0 || strcmp(fieldName, "first_ch")==0 || strcmp(fieldName,"b0_error")==0 || strcmp(fieldName,"change_bl"))) return base+0;
    return basedesc ? basedesc->findField(object, fieldName) : -1;
}

const char *TSCHABMPPacketDescriptor::getFieldTypeString(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldTypeString(object, field);
        field -= basedesc->getFieldCount(object);
    }
    static const char *fieldTypeStrings[] = {
        "bool",
    };
    return (field>=0 && field<1) ? fieldTypeStrings[field] : NULL;
}

const char *TSCHABMPPacketDescriptor::getFieldProperty(void *object, int field, const char *propertyname) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldProperty(object, field, propertyname);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        case 0:
            if (!strcmp(propertyname,"enum")) return "TSCHABMPFrameTypeDef";
            return NULL;
        default: return NULL;
    }
}

int TSCHABMPPacketDescriptor::getArraySize(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getArraySize(object, field);
        field -= basedesc->getFieldCount(object);
    }
    TSCHABMPPacket *pp = (TSCHABMPPacket *)object; (void)pp;
    switch (field) {
        default: return 0;
    }
}

std::string TSCHABMPPacketDescriptor::getFieldAsString(void *object, int field, int i) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldAsString(object,field,i);
        field -= basedesc->getFieldCount(object);
    }
    TSCHABMPPacket *pp = (TSCHABMPPacket *)object; (void)pp;
    switch (field) {
        case 0: return long2string(pp->getAckReq());
        case 1: return long2string(pp->getKindTSCHABMP());
		case 2: return long2string(pp->getAckBitmap());
        default: return "";
    }
}

bool TSCHABMPPacketDescriptor::setFieldAsString(void *object, int field, int i, const char *value) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->setFieldAsString(object,field,i,value);
        field -= basedesc->getFieldCount(object);
    }
    TSCHABMPPacket *pp = (TSCHABMPPacket *)object; (void)pp;
    switch (field) {
        case 0: pp->setAckReq(string2long(value)); return true;
		case 1: pp->setKindTSCHABMP(string2long(value)); return true;
		case 2: pp->setAckBitmap(string2long(value)); return true;
        default: return false;
    }
}

const char *TSCHABMPPacketDescriptor::getFieldStructName(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldStructName(object, field);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        default: return NULL;
    };
}

void *TSCHABMPPacketDescriptor::getFieldStructPointer(void *object, int field, int i) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldStructPointer(object, field, i);
        field -= basedesc->getFieldCount(object);
    }
    TSCHABMPPacket *pp = (TSCHABMPPacket *)object; (void)pp;
    switch (field) {
        default: return NULL;
    }
}
