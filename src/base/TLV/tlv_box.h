#ifndef _TLV_BOX_H_
#define _TLV_BOX_H_

#include <map>
#include <vector>
#include <string>
#include "tlv.h"

namespace base {

class Tlv;

class TlvBox
{
public:
    TlvBox();
    virtual ~TlvBox();

private:
    TlvBox(const TlvBox& c);
    TlvBox &operator=(const TlvBox &c);

public:
    //put one TLV object
    bool PutNoValue(int type);
    bool PutBoolValue(int type, bool value);
    bool PutCharValue(int type, char value);
    bool PutShortValue(int type, uint16_t value);
    bool PutIntValue(int type, int value);
    bool PutLongValue(int type, long value);
    //bool PutLongLongValue(int type, long long value);
    bool PutFloatValue(int type, float value);
    bool PutDoubleValue(int type, double value);
    bool PutStringValue(int type, char *value);
    bool PutStringValue(int type, const std::string &value);
    bool PutBytesValue(int type, unsigned char *value, int length);
    bool PutObjectValue(int type, const TlvBox& value);        

    //do encode
    bool Serialize(); 

    //access encoded buffer and length
    unsigned char * GetSerializedBuffer() const;
    int GetSerializedBytes() const;
    
    //returns number of TLVs in TlvBox, along with a vector of the types
    int GetTLVList(std::vector<int> &list) const;

public:    
    //do decode
    bool Parse(const unsigned char *buffer, int buffersize); 

    //get one TLV object
    bool GetNoValue(int type) const;
    bool GetBoolValue(int type, bool &value) const;
    bool GetCharValue(int type, char &value) const;
    bool GetShortValue(int type, uint16_t &value) const;
    bool GetIntValue(int type, int &value) const;
    bool GetLongValue(int type, long &value) const;
    //bool GetLongLongValue(int type, long long &value) const;
    bool GetFloatValue(int type, float &value) const;
    bool GetDoubleValue(int type, double &value) const;
    bool GetStringValue(int type, char *value, int &length) const;
    bool GetStringValue(int type,std::string &value) const;
    bool GetBytesValue(int type, unsigned char *value, int &length) const;
    bool GetBytesValuePtr(int type, unsigned char **value, int &length) const;
    bool GetObjectValue(int type, TlvBox& value) const;

private:
    bool PutValue(Tlv *value);

private:
    std::map<int,Tlv*> mTlvMap;
    unsigned char *mSerializedBuffer;
    int mSerializedBytes;
	int mTypeLen;
	int mLenLen;
};

} //namespace 

#endif //_TLVOBJECT_H_
