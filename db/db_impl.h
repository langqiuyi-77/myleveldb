#ifndef STORAGE_LEVELDB_DB_DB_IMPL_H_
#define STORAGE_LEVELDB_DB_DB_IMPL_H_

#include <string>
#include <mutex>
#include <condition_variable>
#include <deque>

#include "leveldb/db.h"
#include "port/thread_annotations.h"

namespace leveldb {

class DBImpl : public DB {
public:
  DBImpl(const std::string& dbname);
  
  // 显式delete提高可读性于明确的错误信息
  DBImpl(const DBImpl&) = delete;
  DBImpl& operator=(const DBImpl&) = delete;

  ~DBImpl() override; // override 让编译器可以检查这个函数是否真的在覆盖基类的方法，避免错误。

  virtual bool Put(const WriteOptions&, const Slice& key, const Slice& value) override;
  virtual bool Delet(const WriteOptions&, const Slice& key) override;
  virtual bool Write(const WriteOptions&, WriteBatch* updates) override;
  virtual bool Get(const WriteOptions&, const Slice& key, std::string* value) override;

private:
  struct Writer;      // 声明Writer结构体是DBImplu内部

  WriteBatch* BuildBatchGroup(Writer** last_writer);

  std::string dbname_;

  std::mutex mutex_;  // 用来做合并写的互斥量
  std::deque<Writer*> writers_ GUARDED_BY(mutex_);
  WriteBatch* tmp_batch_ GUARDED_BY(mutex_);
};

}

#endif