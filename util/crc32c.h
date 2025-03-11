#ifndef STORAGE_LEVELDB_UTIL_CRC32C_H_
#define STORAGE_LEVELDB_UTIL_CRC32C_H_

#include <cstdint>
#include <cstddef>

namespace leveldb {
namespace crc32c {

// TODO: 返回 concat(A, data[0,n-1] 的 crc32c, 这个init_cc就是A的crc32c
// Extend()被经常用来维护说stream of data的crc32c
uint32_t Extend(uint32_t init_crc, const char* data, size_t n);

// 返回说data[0,n-1]的crc32c
inline uint32_t Value(const char* data, size_t n) { return Extend(0, data, n); }

}
}

#endif