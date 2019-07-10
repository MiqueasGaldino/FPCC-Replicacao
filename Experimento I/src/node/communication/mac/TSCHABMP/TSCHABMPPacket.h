#ifndef _TSCHABMPPACKET_
#define _TSCHABMPPACKET_

#include <omnetpp.h>

#include "MacPacket_m.h"

#define DATA_PACKET 111
#define BEACON_PACKET 112
#define QTDNODES 5

enum TSCHABMPFrameTypeDef {
    DATA_FRAME = 1,
    BEACON_FRAME = 2,
	ACK_FRAME = 3
};

class TSCHABMPPacket : public ::MacPacket
{
  protected:
	bool ackReq;
	int kind_tschabmp;
	int ack_bitmap;
	int *channel;
	int beacon_id;
	int blacklist;
	int first_ch;
	int b0_error;
	int change_bl;
	
  private:
    void copy(const TSCHABMPPacket& other);
	
  protected:
	 // protected and unimplemented operator==(), to prevent accidental usage
  bool operator==(const TSCHABMPPacket&);

  public:
    TSCHABMPPacket(const char *name=NULL, int kind=0);
    TSCHABMPPacket(const TSCHABMPPacket& other);
    virtual ~TSCHABMPPacket();
    TSCHABMPPacket& operator=(const TSCHABMPPacket& other);
    virtual TSCHABMPPacket *dup() const {return new TSCHABMPPacket(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
	virtual bool getAckReq() const;
	virtual void setAckReq(bool);
	virtual int getAckBitmap() const;
	virtual void setAckBitmap(int);	
	
	virtual int* getChannel() const;
	virtual void setChannel(int*);
	
	virtual int getBeaconId() const;
	virtual void setBeaconId(int);
	virtual int getBlacklist() const;
	virtual void setBlacklist(int);
	virtual int getFirstch() const;
	virtual void setFirstch(int);
	virtual int getB0error() const;
	virtual void setB0error(int);
	virtual int getChangebl() const;
	virtual void setChangebl(int);

	int getKindTSCHABMP();
	void setKindTSCHABMP(int);
};

inline void doPacking(cCommBuffer *b, TSCHABMPPacket& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, TSCHABMPPacket& obj) {obj.parsimUnpack(b);}

#endif // ifndef _TSCHABMPPACKET_

