#ifndef STORAGE_LEVELDB_UTIL_CODING_H_
#define STORAGE_LEVELDB_UTIL_CODING_H_

#include <cstdint>
#include <string>

namespace leveldb {
  
// REQUESTS: dst需要有足够的空间
// uint32_t value = 0x12345678;
// 0x78 0x56 0x34 0x12 因为是小端序，所以低位置存储低位
// 所以是说static_cast<uint8_t>(value);后取的是低地址的８位所以是0x78
// 然后虽然我们在代码中写的是value>>8但是在内存中实际上就是把低位置的去掉，
// 所以下一个Buffer[1]是0x56, 因此在buffer中存的顺序和内存中的一致
inline void EncodeFixed32(char* dst, uint32_t value) {
  uint8_t* const buffer = reinterpret_cast<uint8_t*>(dst);

  buffer[0] = static_cast<uint8_t>(value);        // 存储最低字节 0x78
  buffer[1] = static_cast<uint8_t>(value >> 8);   // 存储次低字节 0x56
  buffer[2] = static_cast<uint8_t>(value >> 16);  // 存储次高字节 0x34
  buffer[3] = static_cast<uint8_t>(value >> 24);  // 存储最高字节 0x12
}

inline uint32_t DecodeFixed32(const char* ptr) {
  const uint8_t* const buffer = reinterpret_cast<const uint8_t*>(ptr);

  return (static_cast<uint32_t>(buffer[0])) |
         (static_cast<uint32_t>(buffer[1]) << 8) |
         (static_cast<uint32_t>(buffer[2]) << 16) |
         (static_cast<uint32_t>(buffer[3]) << 24); 
}

inline uint64_t DecodeFixed64(const char* ptr) {
  const uint8_t* const buffer = reinterpret_cast<const uint8_t*>(ptr);

  return (static_cast<uint64_t>(buffer[0])) |
         (static_cast<uint64_t>(buffer[1]) << 8) |
         (static_cast<uint64_t>(buffer[2]) << 16) |
         (static_cast<uint64_t>(buffer[3]) << 24) |
         (static_cast<uint64_t>(buffer[4]) << 32) |
         (static_cast<uint64_t>(buffer[5]) << 40) |
         (static_cast<uint64_t>(buffer[6]) << 48) |
         (static_cast<uint64_t>(buffer[7]) << 56);
}

inline void EncodeFixed64(char* dst, uint64_t value) {
  uint8_t* const buffer = reinterpret_cast<uint8_t*>(dst);

  buffer[0] = static_cast<uint8_t>(value);        
  buffer[1] = static_cast<uint8_t>(value >> 8);   
  buffer[2] = static_cast<uint8_t>(value >> 16);  
  buffer[3] = static_cast<uint8_t>(value >> 24);  
  buffer[4] = static_cast<uint8_t>(value >> 32);  
  buffer[5] = static_cast<uint8_t>(value >> 40);  
  buffer[6] = static_cast<uint8_t>(value >> 48);  
  buffer[7] = static_cast<uint8_t>(value >> 56);  
}

void PutLengthPrefixedString(std::string* dst, const std::string& value) {
  PutVarint32(dst, value.size());      // varsting的前４byte是length
  dst->append(value.data(), value.size());
}

void PutVarint32(std::string* dst, uint32_t v) {
  char buf[5];
  char* ptr = EncodeVarint32(buf, v);
  dst->append(buf, ptr - buf);
}

// varint32是指将32位数字编码成为变长编码
// 它的核心思想是：
// 每个字节的最高位（第 8 位）用作标志位：
//    如果标志位为 1，表示后续还有字节。
//    如果标志位为 0，表示当前字节是最后一个字节。
//    剩余的 7 位用于存储整数的有效数据。
// 所以只是char buf[4] 可能存不下
char* EncodeVarint32(char* dst, uint32_t v) {
  // Operate on characters as unsigneds
  uint8_t* ptr = reinterpret_cast<uint8_t*>(dst);
  static const int B = 128;   // B = 0x80 用来设置最高位
  if (v < (1 << 7)) {
    *(ptr++) = v;             // 如果v小于128直接存储
  } else if (v < (1 << 14)) {
    *(ptr++) = v | B;         // 存储第一个字节，并设置标志位
    *(ptr++) = v >> 7;        // 存储第二个字节
  } else if (v < (1 << 21)) {
    *(ptr++) = v | B;
    *(ptr++) = (v >> 7) | B;
    *(ptr++) = v >> 14;
  } else if (v < (1 << 28)) {
    *(ptr++) = v | B;
    *(ptr++) = (v >> 7) | B;
    *(ptr++) = (v >> 14) | B;
    *(ptr++) = v >> 21;
  } else {
    *(ptr++) = v | B;
    *(ptr++) = (v >> 7) | B;
    *(ptr++) = (v >> 14) | B;
    *(ptr++) = (v >> 21) | B;
    *(ptr++) = v >> 28;
  }
  return reinterpret_cast<char*>(ptr);
}
}

#endif