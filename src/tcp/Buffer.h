#pragma once

#include <string>
#include <vector>

class Buffer {
 public:
  static const size_t CheapPrepend = 8;
  static const size_t InitSize = 1024;

  explicit Buffer(size_t initSize = InitSize)
      : buffer_(initSize + CheapPrepend), readerIndex_(CheapPrepend), writerIndex_(CheapPrepend) {}

 public:
  void swap(Buffer &rhs) {
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
  const char *peek() const { return begin() + readerIndex_; }
  // peek at the writerIndex_ without moving it
  char *beginWrite() { return begin() + writerIndex_; }
  const char *beginWrite() const { return begin() + writerIndex_; }

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
  void append(const char * /*restrict*/ data, size_t len) {
    ensureWritableBytes(len);
    std::copy(data, data + len, begin() + writerIndex_);
    writerIndex_ += len;
  }

  // prepend
  void prepend(const void * /*restrict*/ data, size_t len) {
    readerIndex_ -= len;
    const char *d = static_cast<const char *>(data);
    std::copy(d, d + len, begin() + readerIndex_);
  }

  // shrink
  void shrink() { buffer_.shrink_to_fit(); }

 private:
  /// @brief return char* to the beginning of the buffer
  char *begin() { return &*buffer_.begin(); }
  const char *begin() const { return &*buffer_.begin(); }

  /// @brief expand LEN bytes of space in the buffer
  void makeSpace(size_t len) {
    if (len > writableBytes()) {
      buffer_.resize(writerIndex_ + len);
    }
  }

 private:
  std::vector<char> buffer_;
  size_t readerIndex_;
  size_t writerIndex_;
};
