#pragma once

#include <iostream>
#include <string>
#include <vector>

class Buffer {
 public:
  static const size_t CheapPrepend = 8;
  static const size_t InitSize = 1024;

  explicit Buffer(size_t initSize = InitSize)
      : buffer_(initSize + CheapPrepend), readerIndex_(CheapPrepend), writerIndex_(CheapPrepend) {}

 public:
  void swap(Buffer& rhs) {
    buffer_.swap(rhs.buffer_);
    std::swap(readerIndex_, rhs.readerIndex_);
    std::swap(writerIndex_, rhs.writerIndex_);
  }

  // writerIndex_ - readerIndex_
  size_t readableBytes() const { return writerIndex_ - readerIndex_; }
  // buffer_.size() - writerIndex_
  size_t writableBytes() const { return buffer_.size() - writerIndex_; }
  // readerIndex_
  size_t prependableBytes() const { return readerIndex_; }

  // peek at the readerIndex_ without moving it
  const char* peek() const { return begin() + readerIndex_; }
  // peek at the writerIndex_ without moving it
  char* beginWrite() { return begin() + writerIndex_; }
  const char* beginWrite() const { return begin() + writerIndex_; }

  // retrieve operation will move the readerIndex_ & writerIndex_ only
  void retrieveAll() {
    readerIndex_ = CheapPrepend;
    writerIndex_ = CheapPrepend;
  }
  void retrieve(size_t len) {
    if (len < readableBytes()) {
      readerIndex_ += len;
    } else {
      retrieveAll();
    }
  }

  // return string
  std::string retrieveAsString(size_t len) {
    std::string result(peek(), len);
    retrieve(len);  // move len bytes forward
    return result;
  }
  std::string retrieveAllAsString() {
    std::string result(peek(), readableBytes());
    retrieveAll();
    return result;
  }
  std::string toString() const { return std::string(peek(), readableBytes()); }

  // make sure there are LEN bytes left in the buffer
  void ensureWritableBytes(size_t len) {
    if (writableBytes() < len) {
      makeSpace(len);
    }
  }

  // append
  void append(const char* /*restrict*/ data, size_t len) {
    ensureWritableBytes(len);
    std::copy(data, data + len, begin() + writerIndex_);
    writerIndex_ += len;
  }

  // move the writerIndex_ forward
  void hasWritten(size_t len) {
    if (len > writableBytes()) {
      std::cerr << "Buffer::hasWritten, len > writableBytes()" << std::endl;
    } else {
      writerIndex_ += len;
    }
  }

  // prepend
  void prepend(const void* /*restrict*/ data, size_t len) {
    readerIndex_ -= len;
    const char* d = static_cast<const char*>(data);
    std::copy(d, d + len, begin() + readerIndex_);
  }

  // shrink
  void shrink() { buffer_.shrink_to_fit(); }

 protected:
  /// @brief return char* to the beginning of the buffer
  char* begin() { return &*buffer_.begin(); }
  const char* begin() const { return &*buffer_.begin(); }

  /// @brief expand LEN bytes of space in the buffer
  void makeSpace(size_t len) {
    if (writableBytes() + prependableBytes() < len + CheapPrepend) {
      buffer_.resize(writerIndex_ + len);
    } else {
      size_t readable = readableBytes();
      std::copy(begin() + readerIndex_, begin() + writerIndex_, begin() + CheapPrepend);
      readerIndex_ = CheapPrepend;
      writerIndex_ = readerIndex_ + readable;
    }
  }

 protected:
  std::vector<char> buffer_;
  size_t readerIndex_;
  size_t writerIndex_;
};

// template <typename T>
// class readspan {
//   std::span<T> sp;
//   size_t start_ = 0;  // logic start point
//   size_t len_ = 0;
//   size_t lenOfBuffer_ = 0;  // length of buffer

//  public:
//   readspan(std::span<T> sequence, size_t start, size_t len, size_t lenOfBuffer)
//       : sp(sequence), start_(start), len_(len), lenOfBuffer_(lenOfBuffer) {}

//   T &operator[](size_t index) { return sp[(index + start_) % lenOfBuffer_]; }

//   size_t size() const { return len_; }
// };

// class CycleBuffer {
//  public:
//   explicit CycleBuffer(size_t initSize = 1024) : buffer_(initSize), readerIndex_(0), writerIndex_(0) {}

//   readspan<char> peek() { return readspan<char>(span<char>(buffer_), readerIndex_, readableBytes(), buffer_.size());
//   }

//   size_t readableBytes() {
//     return writerIndex_ > readerIndex_ ? (writerIndex_ - readerIndex_) : (buffer_.size() + writerIndex_ -
//     readerIndex_);
//   }
//   size_t writableBytes() {
//     return readerIndex_ > writerIndex_ ? (readerIndex_ - writerIndex_) : (buffer_.size() + readerIndex_ -
//     writerIndex_);
//   }

//   void retrieve(size_t len) {
//     if (len < readableBytes()) {
//       readerIndex_ = (readerIndex_ + len) % buffer_.size();
//     } else {
//       retrieveAll();
//     }
//   }
//   void retrieveAll() {
//     readerIndex_ = 0;
//     writerIndex_ = 0;
//   }

//   void append(const char * /*restrict*/ data, size_t len) {
//     ensureWritableBytes(len);
//     if (writerIndex_ + len > buffer_.size()) {
//       size_t firstHalf = buffer_.size() - writerIndex_;
//       std::copy(data, data + firstHalf, begin() + writerIndex_);
//       std::copy(data, data + len - firstHalf, begin());
//     } else {
//       std::copy(data, data + len, begin() + writerIndex_);
//     }
//     writerIndex_ = (writerIndex_ + len) % buffer_.size();
//   }

//   void ensureWritableBytes(size_t len) {
//     if (writableBytes() < len) {
//       makeSpace(len);
//     }
//   }

//   void makeSpace(size_t len) {
//     size_t originSize = buffer_.size();
//     std::cout << "originSize: " << originSize << std::endl;
//     size_t readableToRightBorder = buffer_.size() - readerIndex_;
//     buffer_.resize(buffer_.size() + len);
//     if (writerIndex_ > readerIndex_) {
//       return;
//     } else {
//       std::cout << "copy: " << buffer_.size() << std::endl;
//       std::copy_backward(begin() + readerIndex_, begin() + originSize, end());
//     }
//   }

//  protected:
//   char *begin() { return &*buffer_.begin(); }
//   const char *begin() const { return &*buffer_.begin(); }
//   char *end() { return &*buffer_.end(); }
//   const char *end() const { return &*buffer_.end(); }

//  protected:
//   std::vector<char> buffer_;
//   size_t readerIndex_;
//   size_t writerIndex_;
// };
