#ifndef STORAGE_LEVELDB_INCLUDE_SLICE_H_
#define STORAGE_LEVELDB_INCLUDE_SLICE_H_

#include <cstddef>
#include <cstring>
#include <string>

namespace leveldb {

// 为什么使用Slice而不是引用如果是为了减少这种数据的复制
// Slice是一个数据视图，主要因为Slice的灵活性，如果是引用
// 首先引用在创建的时候就要绑定，而且引用不能引用部分，也
// 不好表示空，但是Slice就很灵活，可以表达部分和空
class Slice {
  public:
    // 创建一个空的slice
    Slice() : data_(""), size_(0) {}

    // 创建一个slice指向d[0,n-1]
    Slice(const char* d, size_t n) : data_(d), size_(n) {}

    Slice(const std::string& s) : data_(s.data()), size_(s.size()) {}

    // 创建一个指向s[0,strlen(s)-1]的slice
    Slice(const char* s) : data_(s), size_(strlen(s)) {}

    void clear() {
      data_ = "";
      size_ = 0;
    }

    const char* data() const { return data_; }
    size_t size() const {return size_; }

    Slice(const Slice&) = default;
    Slice& operator=(const Slice&) = default; 

  private:
    const char* data_;
    size_t size_;
};

}

#endif