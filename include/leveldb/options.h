#ifndef STORAGE_LEVELDB_INCLUDE_OPTIONS_H_
#define STORAGE_LEVELDB_INCLUDE_OPTIONS_H_

namespace leveldb {

struct WriteOptions {
  WriteOptions() = default;

  // sync=true表示在write完成之前会被
  // 从操作系统的buffer刷新到磁盘持久化
  bool sync = false;
};
}

#endif