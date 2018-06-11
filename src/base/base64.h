#ifndef _BASE64_H_
#define _BASE64_H_

#include <string>

namespace base {

class Base64
{
public:
    Base64() {}
    virtual ~Base64() {}

    static bool Encode(const std::string& src, std::string* dst);

    static bool Decode(const std::string& src, std::string* dst);

private:
    // 根据在Base64编码表中的序号求得某个字符
    static inline char Base2Chr(unsigned char n);

    // 求得某个字符在Base64编码表中的序号
    static inline unsigned char Chr2Base(char c);

    inline static int Base64EncodeLen(int n);
    inline static int Base64DecodeLen(int n);
};

} 

#endif 

