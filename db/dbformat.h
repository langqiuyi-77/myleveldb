#ifndef STORAGE_LEVELDB_DB_DBFORMAT_H_
#define STORAGE_LEVELDB_DB_DBFORMAT_H_

#include <cstddef>

namespace leveldb {
  
typedef uint64_t SequenceNumber; 

enum ValueType { kTypeDeletion = 0x0, kTypeValue = 0x1 };

}

#endif