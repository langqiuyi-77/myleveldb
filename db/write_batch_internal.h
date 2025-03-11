#ifndef STORAGE_LEVELDB_DB_WRITE_BATCH_INTERNAL_H_
#define STORAGE_LEVELDB_DB_WRITE_BATCH_INTERNAL_H_

#include "db/dbformat.h"
#include "leveldb/write_batch.h"

namespace leveldb {
 
// WriteBatchInternal提供了static methods去修改WriteBatch的seq+count
// 我们不希望用户能设置seq+count,因为不能将方法实现在暴露的writeBatch　API中
// 有需要提供一个方法，因此使用WriteBatchInternal,这个类是内部使用的，不会暴露
class WriteBatchInternal {
  public:
    static int Count(const WriteBatch* batch);

    static void SetCount(WriteBatch* batch, int n);

    static SequenceNumber Sequence(const WriteBatch* batch);

    static void SetSequence(WriteBatch* batch, SequenceNumber seq);

    static void Append(WriteBatch* dst, const WriteBatch* src);
    
    static size_t ByteSize(const WriteBatch* batch) { return batch->rep_.size(); }

    static Slice Contents(const WriteBatch* batch) { return Slice(batch->rep_); }
};
}
#endif