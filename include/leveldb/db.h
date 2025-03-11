#ifndef STORAGE_LEVELDB_INCLUDE_DB_H_
#define STORAGE_LEVELDE_INCLUDE_DB_H_

#include <string>

#include "leveldb/slice.h"
#include "leveldb/write_batch.h"
#include "include/leveldb/options.h"

// 简化实现　TODO
// ----------------------
//
// (1) Slice: 一个数据视图，不会拷贝数据，简化传递数据都是string

namespace leveldb {

// 一个DB是一个持久化的ordered map from keys to values
// 一个DB是线程安全的，不需要external synchronization
class DB {
  public:
    // 使用＂name＂命名打开这个数据库，*dbptr中存储在heap分配的数据库指针
    static bool Open(const std::string& name, DB** dbptr);

    DB() = default;

    // 禁用数据库的复制构造和运算符
    DB(const DB&) = delete;
    DB& operator=(const DB&) = delete; 

    // 数据库可扩展性，有不同种DB实现
    virtual ~DB();

    virtual bool Put(const WriteOptions&, const Slice& key, const Slice& value) = 0;
    virtual bool Delet(const WriteOptions&, const Slice& key) = 0;
    virtual bool Write(const WriteOptions&, WriteBatch* updates) = 0;
    virtual bool Get(const WriteOptions&, const Slice& key, std::string* value) = 0;

};

}

#endif