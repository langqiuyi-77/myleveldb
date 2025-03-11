#ifndef STORAGE_LEVELDB_INCLUDE_ENV_H_
#define STORAGE_LEVELDB_INCLUDE_ENV_H_

#include "leveldb/slice.h"
#include "leveldb/status.h"

namespace leveldb {

// 这个是一个基类，是一个文件的sequential writing的抽象
// 要求它的实现一定要有buffery因为可能添加的时候添加的small fragments
// 使用基类是因为在不用的win、posix下他调用的方法不同所以有关于env
class WritableFile {
  public:
    WritableFile() = default;
    
    WritableFile(const WritableFile&) = delete;
    WritableFile& operator=(const WritableFile&) = delete;

    virtual ~WritableFile();

    virtual Status Append(const Slice& data) = 0;
    virtual Status Close() = 0;
    virtual Status Flush() = 0;
    virtual Status Sync() = 0;
};

}

#endif