#include <string.h>
#include <arpa/inet.h>

#include "logging.h"

#include "tlv.h"
#include "tlv_box.h"

using namespace std;
using namespace base;

namespace base {

static float swapFloat( float f )  //assumes sender & receiver use same float format, such as IEEE-754
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    union { float f; int i; } u32; 
    u32.f = f;
    u32.i = htonl(u32.i);
    return u32.f;
#else
    return f;
#endif
}

static double swapDouble( double d )  //assumes sender & receiver use same double format, such as IEEE-754
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    union { double d; int i[2]; } u64;  //used i[2] instead of long long since some compilers don't support long long
    u64.d = d;
    int temp = htonl(u64.i[1]);
    u64.i[1] = htonl(u64.i[0]);
    u64.i[0] = temp;
    return u64.d;
#else
    return d;
#endif
}

TlvBox::TlvBox() : mSerializedBuffer(NULL), mSerializedBytes(0), mTypeLen(2), mLenLen(2)
{
    LOG_TRACE("create TlvBox mTypeLen:%d, mLenLen:%d", mTypeLen, mLenLen);
}

TlvBox::~TlvBox()
{
    LOG_TRACE("~TlvBox");
    if (mSerializedBuffer != NULL) {
        delete[] mSerializedBuffer;
        mSerializedBuffer = NULL;
    }

    std::map<int,Tlv*>::iterator itor;
    for(itor=mTlvMap.begin(); itor != mTlvMap.end(); itor++) {
        delete itor->second;
    }

    mTlvMap.clear();
}

bool TlvBox::Serialize()
{
    if (mSerializedBuffer != NULL) {
        return false;
    }

    int offset = 0;
    mSerializedBuffer = new unsigned char[mSerializedBytes];

    std::map<int,Tlv*>::iterator itor;
    uint16_t type16 = 0;
    uint32_t type32 = 0;
    uint16_t nwlength16 = 0;
    uint32_t nwlength32 = 0;
    for(itor=mTlvMap.begin(); itor!=mTlvMap.end(); itor++) { 
        if (mTypeLen == 4) {
            type32 = htonl(itor->second->GetType());
            memcpy(mSerializedBuffer+offset, &type32, mTypeLen);
        }
        else {
            type16 = htons((uint16_t)(itor->second->GetType()));
            memcpy(mSerializedBuffer+offset, &type16, mTypeLen);
        }
        offset += mTypeLen;  

        unsigned short length = itor->second->GetLength();
        if (mTypeLen == 4) {
            nwlength32 = htonl(length);
            memcpy(mSerializedBuffer+offset, &nwlength32, mLenLen);
        }
        else {
            nwlength16 = htons(length);
            memcpy(mSerializedBuffer+offset, &nwlength16, mLenLen);
        }
        offset += mLenLen;
        memcpy(mSerializedBuffer+offset, itor->second->GetValue(), length);        
        offset += length;
    }

    return true;
}

bool TlvBox::Parse(const unsigned char *buffer, int buffersize)
{
    if (mSerializedBuffer != NULL || buffer == NULL) {
        return false;
    }

    unsigned char *cached = new unsigned char[buffersize];
    memcpy(cached, buffer, buffersize);

    int offset = 0;
    int type = 0;
    int length = 0;
    while (offset < buffersize) {
        if (mTypeLen == 4)
            type = ntohl((*(uint32_t *)(cached + offset)));
        else
            type = ntohs((*(uint16_t *)(cached + offset)));
        offset += mTypeLen;
        if (mLenLen == 4)
            length = ntohl((*(uint32_t *)(cached + offset)));
        else
            length = ntohs((*(uint16_t *)(cached + offset)));
        offset += mLenLen;
        PutValue(new Tlv(type, cached + offset, length));
        offset += length;
    }

    mSerializedBuffer = cached;
    mSerializedBytes = buffersize;

    return true;
}

unsigned char * TlvBox::GetSerializedBuffer() const
{
    return mSerializedBuffer;
}

int TlvBox::GetSerializedBytes() const
{
    LOG_TRACE("mSerializedBytes:%d",mSerializedBytes);
    return mSerializedBytes;
}

bool TlvBox::PutValue(Tlv *value)
{
    std::map<int,Tlv*>::iterator itor = mTlvMap.find(value->GetType());
    if (itor != mTlvMap.end()) {
        Tlv *prevTlv = itor->second;
        mSerializedBytes = mSerializedBytes - (mTypeLen + mLenLen + prevTlv->GetLength());
        delete itor->second;
        itor->second = value;    
    } else {
        mTlvMap.insert(std::pair<int, Tlv*>(value->GetType(), value));
    }  
 
    mSerializedBytes += (mTypeLen + mLenLen + value->GetLength());
    return true;
}

bool TlvBox::PutNoValue(int type)
{
    if (mSerializedBuffer != NULL) {
        return false;
    }
    return PutValue(new Tlv(type));
}

bool TlvBox::PutBoolValue(int type, bool value)
{
    if (mSerializedBuffer != NULL) {
        return false;
    }
    return PutValue(new Tlv(type, value));
}

bool TlvBox::PutCharValue(int type, char value)
{
    if (mSerializedBuffer != NULL) {
        return false;
    }
    return PutValue(new Tlv(type, value));
}

bool TlvBox::PutShortValue(int type, uint16_t value)
{
    if (mSerializedBuffer != NULL) {
        return false;
    }
    short nwvalue = htons(value);
    return PutValue(new Tlv(type, nwvalue));
}

bool TlvBox::PutIntValue(int type, int value)
{
    if (mSerializedBuffer != NULL) {
        return false;
    }
    int nwvalue = htonl(value);
    return PutValue(new Tlv(type, nwvalue));
}

bool TlvBox::PutLongValue(int type, long value)
{
    if (mSerializedBuffer != NULL) {
        return false;
    }
    long nwvalue = htonl(value);
    return PutValue(new Tlv(type, nwvalue));
}

/*
bool TlvBox::PutLongLongValue(int type, long long value)
{
    if (mSerializedBuffer != NULL) {
        return false;
    }
    long long nwvalue = htonll(value);
    return PutValue(new Tlv(type, nwvalue));
}
*/
bool TlvBox::PutFloatValue(int type, float value)
{
    if (mSerializedBuffer != NULL) {
        return false;
    }
    float nwvalue = swapFloat(value);
    return PutValue(new Tlv(type, nwvalue));
}

bool TlvBox::PutDoubleValue(int type, double value)
{
    if (mSerializedBuffer != NULL) {
        return false;
    }
    double nwvalue = swapDouble(value);
    return PutValue(new Tlv(type, nwvalue));
}

bool TlvBox::PutStringValue(int type, char *value)
{
    if (mSerializedBuffer != NULL) {
        return false;
    }
    return PutValue(new Tlv(type, value));
}

bool TlvBox::PutStringValue(int type, const std::string &value)
{
    if (mSerializedBuffer != NULL) {
        return false;
    }
    return PutValue(new Tlv(type, value));
}

bool TlvBox::PutBytesValue(int type, unsigned char *value, int length)
{
    if (mSerializedBuffer != NULL) {
        return false;
    }
    return PutValue(new Tlv(type, value, length));
}

bool TlvBox::PutObjectValue(int type, const TlvBox& value)
{
    if (mSerializedBuffer != NULL) {
        return false;
    }
    unsigned char *buffer = value.GetSerializedBuffer();
    if (buffer == NULL) {
        return false;
    }
    return PutValue(new Tlv(type, buffer, value.GetSerializedBytes()));
}

bool TlvBox::GetNoValue(int type) const
{
    std::map<int,Tlv*>::const_iterator itor = mTlvMap.find(type);
    if (itor != mTlvMap.end()) {
        return true;
    }
    return false;
}

bool TlvBox::GetBoolValue(int type, bool &value) const
{
    std::map<int,Tlv*>::const_iterator itor = mTlvMap.find(type);
    if (itor != mTlvMap.end()) {
        value = (*(bool *)(itor->second->GetValue()));
        return true;
    }
    return false;
}

bool TlvBox::GetCharValue(int type, char &value) const
{
    std::map<int,Tlv*>::const_iterator itor = mTlvMap.find(type);
    if (itor != mTlvMap.end()) {
        value = (*(char *)(itor->second->GetValue()));
        return true;
    }
    return false;
}

bool TlvBox::GetShortValue(int type, uint16_t &value) const
{
    std::map<int,Tlv*>::const_iterator itor = mTlvMap.find(type);
    if (itor != mTlvMap.end()) {
        value = ntohs((*(uint16_t *)(itor->second->GetValue())));
        return true;
    }
    return false;
}

bool TlvBox::GetIntValue(int type, int &value) const
{
    std::map<int,Tlv*>::const_iterator itor = mTlvMap.find(type);
    if (itor != mTlvMap.end()) {
        value = ntohl((*(int *)(itor->second->GetValue())));
        return true;
    }
    return false;
}

bool TlvBox::GetLongValue(int type, long &value) const
{
    std::map<int,Tlv*>::const_iterator itor = mTlvMap.find(type);
    if (itor != mTlvMap.end()) {
        value = ntohl((*(long *)(itor->second->GetValue())));
        return true;
    }
    return false;
}

/*
bool TlvBox::GetLongLongValue(int type, long long &value) const
{
    std::map<int,Tlv*>::const_iterator itor = mTlvMap.find(type);
    if (itor != mTlvMap.end()) {
        value = ntohll((*(long long *)(itor->second->GetValue())));
        return true;
    }
    return false;
}
*/

bool TlvBox::GetFloatValue(int type, float& value) const
{
    std::map<int,Tlv*>::const_iterator itor = mTlvMap.find(type);
    if (itor != mTlvMap.end()) {
        value = swapFloat((*(float *)(itor->second->GetValue())));
        return true;
    }
    return false;
}

bool TlvBox::GetDoubleValue(int type, double& value) const
{
    std::map<int,Tlv*>::const_iterator itor = mTlvMap.find(type);
    if (itor != mTlvMap.end()) {
        value = swapDouble((*(double *)(itor->second->GetValue())));
        return true;
    }
    return false;
}

bool TlvBox::GetStringValue(int type, char *value, int &length) const
{
    return GetBytesValue(type,(unsigned char *)value,length);
}

bool TlvBox::GetStringValue(int type, std::string &value) const
{
    std::map<int,Tlv*>::const_iterator itor = mTlvMap.find(type);
    if (itor != mTlvMap.end()) {        
        value = (char *)(itor->second->GetValue());        
        return true;
    }
    return false;
}

bool TlvBox::GetBytesValue(int type, unsigned char *value, int &length) const
{
    std::map<int,Tlv*>::const_iterator itor = mTlvMap.find(type);
    if (itor == mTlvMap.end()) {
        return false;
    }

    if (length < itor->second->GetLength()) {
        return false;
    }

    memset(value, 0, length);
    length = itor->second->GetLength();
    memcpy(value,itor->second->GetValue(), length);

    return true;
}

bool TlvBox::GetBytesValuePtr(int type, unsigned char **value, int &length) const
{
    std::map<int,Tlv*>::const_iterator itor = mTlvMap.find(type);
    if (itor == mTlvMap.end()) {
        return false;        
    }
    *value = itor->second->GetValue();
    length = itor->second->GetLength();    
    return true;
}

bool TlvBox::GetObjectValue(int type, TlvBox& value) const
{
    std::map<int,Tlv*>::const_iterator itor = mTlvMap.find(type);
    if (itor == mTlvMap.end()) {
        return false;    
    }
    return value.Parse(itor->second->GetValue(), itor->second->GetLength());
}

int TlvBox::GetTLVList(std::vector<int> &list) const
{
    std::map<int, Tlv*>::const_iterator iter;
    for (iter=mTlvMap.begin(); iter != mTlvMap.end(); iter++) {
        list.push_back(iter->first);
    }
    return list.size();
}

} //namespace 
