#include <cstring>
#include <assert.h>

#include "db/log_writer.h"
#include "util/crc32c.h"
#include "util/coding.h"

namespace leveldb {
namespace log {

static void InitTypeCrc(uint32_t* type_crc) {
  for (int i = 0; i <= kMaxRecordType; i ++) {
    char t = static_cast<char>(i);
    type_crc[i] = crc32c::Value(&t, 1);
  }
}

Writer::Writer(WritableFile* dest) : dest_(dest), block_offset_(0) {
  InitTypeCrc(type_crc_);
}

Writer::Writer(WritableFile* dest, uint64_t dest_length)
  : dest_(dest), block_offset_(dest_length % kBlockSize) {
  InitTypeCrc(type_crc_);
}

Writer::~Writer() = default;

Status Writer::AddRecord(const Slice& slice) {
  const char* ptr = slice.data();
  size_t left = slice.size();

  // 将记录Fragement并且emit it. 注意到如果说slice是空的，TODO:为什么？
  // 我们依旧需要去iteratr once to emit a single zero-length record
  Status s;
  bool begin = true;
  do {
    const int leftover = kBlockSize - block_offset_;
    assert(leftover >= 0);
    if (leftover < kHeaderSize) {
      // 切换到一个新的block
      if (leftover > 0) {
        // 将尾部trailer填满，literal below relies on kHeaderSize being 7
        static_assert(kHeaderSize == 7, "");
        dest_->Append(Slice("\x00\x00\x00\x00\x00\x00", leftover));
        // TODO: 不太明白这个日志不是32KB吗？但是writablefile的buffer是64KB的这个,日志分block是？
      }
      block_offset_ = 0;
    }

    // Invariant: we never leave < kHeaderSize bytes in a block
    assert(kBlockSize - block_offset_ - kHeaderSize >= 0);

    const size_t avail = kBlockSize - block_offset_ - kHeaderSize;
    const size_t fragment_length = (left < avail) ? left : avail;

    RecordType type;
    const bool end = (left == fragment_length);
    if (begin && end) {
      type = kFullType;
    } else if (begin) {
      type = kFirstType;
    } else if (end) {
      type = kLastType;
    } else {
      type = kMiddleType;
    }

    s = EmitPhysicalRecord(type, ptr, fragment_length);
    ptr += fragment_length;
    left -= fragment_length;
    begin = false;
  } while (s.ok() && left > 0);
  return s;
}

// 实际上就是WritableFile中把数据写入，而且每emit一个就会flush
// 这个flush就是系统调用write，将数据刷入操作系统的buffer
Status Writer::EmitPhysicalRecord(RecordType t, const char* ptr, size_t length) {
  assert(length <= 0xffff); // Must fit in two bytes 日志中长度最多是2 bytes
  assert(block_offset_ + kHeaderSize + length <= kBlockSize);
  
  // Format the header, length(2 bytes), type(1 byte)
  char buf[kHeaderSize];
  buf[4] = static_cast<char>(length & 0xff); 
  buf[5] = static_cast<char>(length >> 8);
  buf[6] = static_cast<char>(t);

  // Compute crc of the record type and the payload. TODO: 我对crc完全不懂，学过概念但是完全没有应用过，忘光
  uint32_t crc = crc32c::Extend(type_crc_[t], ptr, length);
  crc = crc32c::Mask(crc);   // Adjust for storage
  EncodeFixed32(buf, crc);   // 对数字式需要明确的编码的，因为不同的平台有不同的顺序，如果只是memcpy那会依赖对应的平台产生跨平台兼容性问题，而char*不会有，因为它的顺序是唯一的

  // Write the header and the payload
  Status s = dest_->Append(Slice(buf, kHeaderSize));
  if (s.ok()) {
    s = dest_->Append(Slice(ptr, length));
    if (s.ok()) {
      s = dest_->Flush();
      if (s.ok()) {
        s = dest_->Flush();
      }
    }
    block_offset_ += kHeaderSize + length;
    return s;
  }
}

}
}