#ifndef STORAGE_LEVELDB_DB_LOG_FORMAT_H_
#define STORAGE_LEVELDB_DB_LOG_FORMAT_H_

namespace leveldb {
namespace log {

enum RecordType {
  kZeroType = 0,

  kFullType = 1,

  // For fragments
  kFirstType = 2,
  kMiddleType = 3,
  kLastType = 4
};

static const int kMaxRecordType = kLastType; // static 用于将符号的链接性限制为当前翻译单元,避免全局污染

static const int kBlockSize = 32768;  // 日志以block的方法划分，32KB一个block

// 日志的header是 checksum(4 bytes), length(2 bytes), type(1 byte)共7个bytes
static const int kHeaderSize = 4 + 2 + 1;
}
}

#endif