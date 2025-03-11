#include <unistd.h>
#include <cerrno>

#include "leveldb/env.h"
#include "leveldb/status.h"

namespace leveldb {

constexpr const size_t KWritableFileBufferSize = 65536; // 要放在namespace内部

Status PosixError(const std::string& context, int error_number) {
  if (error_number == ENOENT) { // 表示文件或目录不存在
    return Status::NotFound(context, std::strerror(error_number));
  } else {
    return Status::IOError(context, std::strerror(error_number));
  }
}

class PosixWritableFile final : public WritableFile {
  public: 
    PosixWritableFile(std::string filename, int fd)
      : pos_(0),
        fd_(fd),
        filename_(std::move(filename)) {}

    Status Append(const Slice& data) override {
      size_t write_size = data.size();
      const char* write_data = data.data();

      // 将尽量多的数据放入buffer
      size_t copy_size = std::min(write_size, KWritableFileBufferSize - pos_);
      std::memcpy(buf_ + pos_, write_data, copy_size);  // memcpy的使用(要写入的位置，要写入的数据，写入数据的大小)
      write_data += copy_size;  // 调整要写入的数据到还未写入的部分
      write_size -= copy_size;
      pos_ += copy_size;
      if (write_size == 0) {
        return Status::OK();
      }

      // 不能全部放入buffer，至少要做一次write
      Status status = FlushBuffer();
      if (!status.ok()) {
        return status;
      }

      // 小的write到buffer，大的写written directly
      if (write_size < KWritableFileBufferSize) {
        std::memcpy(buf_, write_data, write_size);
        pos_ = write_size;
        return Status::OK();
      }
      return WriteUnbuffered(write_data, write_size);
    }

    Status Close() override {
      Status status = FlushBuffer();
      const int close_result = ::close(fd_);
      if (close_result < 0 && status.ok()) {
        status = PosixError(filename_, errno);
      }
      fd_ = -1;
      return status;
    }

    Status Flush() override { return FlushBuffer(); }

    // TODO: 好像是涉及Manifest的，不懂
    Status Sync() override {

    }

  private:
    Status FlushBuffer() {
      Status status = WriteUnbuffered(buf_, pos_);
      pos_ = 0;
      return status;
    }

    Status WriteUnbuffered(const char* data, size_t size) {
      while (size > 0) {
        ssize_t write_result = ::write(fd_, data, size);  // 全局作用域，确保是系统调用
        if (write_result < 0) { 
          if (errno = EINTR) {  // EINTR标识写入操作被信号中断，重试
            continue;
          }
          return PosixError(filename_, errno);
        }
        data += write_result;
        size -= write_result;
      }
    }

    // buf_[0-pos-1]保存了要写入fd_的数据
    char buf_[KWritableFileBufferSize];
    size_t pos_;
    int fd_;

    const std::string filename_;
};
}