#ifndef STORAGE_LEVELDB_INCLUDE_STATUS_H_
#define STORAGE_LEVELDB_INCLUDE_STATUS_H_

#include "leveldb/status.h"
#include "leveldb/slice.h"

namespace leveldb {

class Status {
  public:
    Status() noexcept : state_(nullptr) {}
    ~Status() { delete[] state_; }

    static Status OK() { return Status(); }

    static Status NotFound(const Slice& msg, const Slice& msg2 = Slice()) {
      return Status(kNotFound, msg, msg2);
    }

    static Status IOError(const Slice& msg, const Slice& msg2 = Slice()) {
      return Status(kIOError, msg, msg2);
    }

    bool ok() const { return (state_ == nullptr); }

  private:
    enum Code {
      kOk = 0,
      kNotFound = 1,
      kCorruption = 2,
      kNotSupported = 3,
      kInvalidArgument = 4,
      kIOError = 5
    };

    Status(Code code, const Slice& msg, const Slice& msg2);

    // OK status 有一个null的state_. 不然state_就是一个new[] array
    // state_[0...3] == 信息的长度 4个byte保持长度
    // state_[4] == code
    // state_[5..] == messgae
    const char* state_;
};

}

#endif