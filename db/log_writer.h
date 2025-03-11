#ifndef STORAGE_LEVELDB_DB_LOG_WRITER_H_
#define STORAGE_LEVELDB_DB_LOG_WRITER_H_

#include "leveldb/slice.h"
#include "leveldb/status.h"
#include "leveldb/env.h"
#include "db/log_format.h"

namespace leveldb {

namespace log {

class Writer {
  public:
    // 创建一个writer将数据append到*dest
    // *dest一开始要是empty
    // WritableFile内部是有一个buffer的
    // 当这个writer在使用的时候*dest一定要存在
    explicit Writer(WritableFile* dest);

    // 创建一个writer将数据append到*dest
    // *dest一开始就要有dest_length的initial length
    Writer(WritableFile* dest, uint64_t dest_length);

    Writer(const Writer&) = delete;
    Writer& operator=(const Writer&) = delete;

    ~Writer();

    Status AddRecord(const Slice& slice);

  private:
    Status EmitPhysicalRecord(RecordType t, const char* ptr, size_t length);

    WritableFile* dest_;
    int block_offset_; // 当前的offset in block

    // crc32c. 他们是pre-computed来减少计算crc的overhead
    uint32_t type_crc_[kMaxRecordType + 1];
};

}

}

#endif