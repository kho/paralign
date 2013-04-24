#ifndef _PARALIGN_TYPES_H_
#define _PARALIGN_TYPES_H_

#include <stdint.h>                     // some older C++ compiler does not ship cstdint

namespace paralign {
typedef int32_t WordId;
typedef uint16_t SentSz;
typedef uint32_t SentSzPair;

inline SentSzPair MkSzPair(SentSz a, SentSz b) {
  return (a << 16) | b;
}

inline SentSz FirstSz(SentSzPair p) {
  return p >> 16;
}

inline SentSz SecondSz(SentSzPair p) {
  return p & 0x0000ffff;
}

inline int64_t DoubleAsInt64(double v) {
  union { int64_t i; double d; } w;
  w.d = v;
  return w.i;
}

inline double DoubleFromInt64(int64_t v) {
  union { int64_t i; double d; } w;
  w.i = v;
  return w.d;
}

}      // namespace paralign

#endif  // _PARALIGN_TYPES_H_
