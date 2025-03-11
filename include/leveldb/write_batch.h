#ifndef STOTAGE_LEVELDB_INCLUDE_WRITE_BATCH_H
#define STOTAGE_LEVELDB_INCLUDE_WRITE_BATCH_H

#include <string>

namespace leveldb {

class WriteBatch {
  public:
    WriteBatch();

    WriteBatch(const WriteBatch&) = default;
    WriteBatch& operator=(const WriteBatch&) = default;

    ~WriteBatch();

    void Put(const std::string& key, const std::string& value); // 添加一个put操作到batch
    void Delete(const std::string& key);      // 添加一个delete操作到batch
    void Append(const WriteBatch& source);    // 合并写
    void Clear();

  private:
    friend class WriteBatchInternal;

    std::string rep_; // 把它当作一个　char数组＝字节数组　来存储数据
};
}

#endif