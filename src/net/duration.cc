#include "inner_pre.h"
#include "duration.h"

namespace net {
const int64_t Duration::kNanosecond = 1LL;
const int64_t Duration::kMicrosecond = 1000 * kNanosecond;
const int64_t Duration::kMillisecond = 1000 * kMicrosecond;
const int64_t Duration::kSecond = 1000 * kMillisecond;
const int64_t Duration::kMinute = 60 * kSecond;
const int64_t Duration::kHour = 60 * kMinute;
}