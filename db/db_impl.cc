#include <cassert>

#include "db/db_impl.h"
#include "db/write_batch_internal.h"

namespace leveldb {

// Writer保存所有waiting的信息以及同步阻塞
// TODO: 这里对Writer的设计如果cv和mu有绑定就好，Warp cv
struct DBImpl::Writer {
  explicit Writer(std::mutex* mu)
      : batch(nullptr), sync(false), done(false), mu_(mu) {}

  WriteBatch* batch;
  bool sync;            // WriteOption，是否要同步刷新
  bool done;            // 被合并的写判断是否完成
  bool status;          // 完成的结果，成功/失败
  std::condition_variable cv_;
  std::mutex* mu_;      // cv_和mu_只一个线程可以写，通知和阻塞
};


DBImpl::DBImpl(const std::string& dbname) : dbname_(dbname) {}

DBImpl::~DBImpl() {}

bool DBImpl::Put(const WriteOptions& options, const Slice& key, const Slice& value) {}
bool DBImpl::Delet(const WriteOptions& options, const Slice& key) {}
bool DBImpl::Get(const WriteOptions& options, const Slice& key, std::string* value) {}


// Write(WriteBatch* updates)是实际上写入的方法，put本身也是调用这个方法
// 这个方法是线程安全的，实现合并写，WriteBatch就是一系列要写的操作
// 可以是队头合并写也可以是一个用户给的批处理，WriteBatch是外部API
bool DBImpl::Write(const WriteOptions& options, WriteBatch* updates) {
  Writer w(&mutex_);
  w.batch = updates;
  w.sync = options.sync;
  w.done = false;

  std::unique_lock<std::mutex> lock(mutex_);
  writers_.push_back(&w);
  while (!w.done && &w != writers_.front()) {
    w.cv_.wait(lock);
  }
  if (w.done) {         // 写操作被队头合并写完成
    return w.status;
  }
 
  // 以下只有写操作的队头在获得锁后合并写的时候执行
  bool status = MakeRoomForWrite(updates == nullptr);
  uint64_t last_sequence = versions_->LastSequence();
  Writer* last_writer = &w;
  if (status && updates != nullptr) { // TODO: nullptr batch is for compaction? 记得说compaction的逻辑是关键，但是我还没有学习逻辑
    WriteBatch* write_batch = BuildBatchGroup(&last_writer);
    WriteBatchInternal::SetSequence(write_batch, last_sequence + 1);
    last_sequence += WriteBatchInternal::Count(write_batch);

    // WAL添加到log中并且应用到内存数据库中。我们可以释放锁
    // 因为这个时候w正在做logging和防止出现concurrent loggers和concurrent writes
    // 所以mutex_是为了保护last_sequence和writers? 这个时候可以让writers继续添加
    {
      mutex_.unlock();
      status = log_->AddRecord(WriteBatchInternal::Contents(write_batch));
    }
  }
}

// 合并写将writers_中合适数量的写操作合并到队头的WriteBatcho中
// 并更新last_writer为最后一个被合并的writer，下一个队头是后一个
// 要求Writers list非空并且队头writer不能是null batch
WriteBatch* DBImpl::BuildBatchGroup(Writer** last_writer) {
  // mutex_.AssertHeld();
  assert(!writers_.empty());
  Writer* first = writers_.front();
  WriteBatch* result = first->batch;
  assert(result != nullptr);

  size_t size = WriteBatchInternal::ByteSize(first->batch);

  // 合并写合并写操作到maximum size, 但是如果原本的写操作很small.
  // 我们不希望说grow up太多的操作使得整个操作本身花费太多的时间
  // 因此当我们检测到了它的size比较小的时候，我们需要限制它的growth
  size_t max_size = 1 << 20;  // 1,048,576
  if (size <= (128 << 10)) {  // 1 << 17 = 2^17 = 131,072
    max_size = size + (128 << 10);
  } // 限制growth的时候最大 2*(128<<10) = 2^18 = 262,144

  *last_writer = first;
  std::deque<Writer*>::iterator iter = writers_.begin();
  ++iter; // 首先advance第一个first
  for (; iter != writers_.end(); ++iter) {
    Writer* w = *iter;
    if (w->sync && !first->sync) {
      // 不要合并sync的写操作到一个non-sync的操作中
      // 不要继续了？要保持这个顺序，必须一个sync的更改key->1
      // 后面有一个non-sync的key->2如果继续这次写了key->2
      // 后做key->1,最后的答案不是期望的，顺序性
      break;
    }

    if (w->batch != nullptr) {  // 当前write操作有batch, 好像如果是nullptr是compaction
      size += WriteBatchInternal::ByteSize(w->batch);
      if (size > max_size) {
        break;
      }

      // Append to *result
      if (result == first->batch) {
        // 切换到一个temporary batch而不是直接在队头的batch中添加
        result = tmp_batch_;
        assert(WriteBatchInternal::Count(result) == 0);
        WriteBatchInternal::Append(result, first->batch);
      }
      WriteBatchInternal::Append(result, first->batch);
    }
    *last_writer = w;
  }
  return result;
}

}

