#include "AsyncLogging.h"

#include <assert.h>

#include "LogFile.h"
#include "LogStream.h"

void AsyncLogging::threadFunc() {
  // LogFile handles the file writing
  LogFile output(dir_, rollSize_, flushInterval_);
  // temporary buffer
  BufferPtr swap_currentBuffer(new Buffer);
  BufferPtr swap_nextBuffer(new Buffer);
  // swap vector for buffers_
  BufferVector buffersToWrite;
  buffersToWrite.reserve(16);

  latch_.countDown();
  while (running_) {
    // swap the buffers
    {
      std::unique_lock<std::mutex> lock(mutex_);
      // wait for buffers_ vector to have element or timeout
      if (buffers_.empty()) {
        cond_.wait_for(lock, std::chrono::seconds(flushInterval_));
        buffers_.push_back(std::move(currentBuffer_));
        // swap the buffers
        buffersToWrite.swap(buffers_);
        currentBuffer_ = std::move(swap_currentBuffer);
        if (!nextBuffer_) {
          nextBuffer_ = std::move(swap_nextBuffer);
        }
      } else {
        buffersToWrite.swap(buffers_);
        // only recover nextBuffer_, let currentBuffer_ keep logging
        nextBuffer_ = std::move(swap_nextBuffer);
      }
    }
    // write the buffers to the file
    for (const auto& buffer : buffersToWrite) {
      output.append(buffer->data(), buffer->length());
    }
    // recover the buffers
    if (buffersToWrite.size() > 2) {
      buffersToWrite.resize(2);
    }
    if (!swap_currentBuffer) {
      assert(!buffersToWrite.empty());
      swap_currentBuffer = std::move(buffersToWrite.back());
      buffersToWrite.pop_back();
      swap_currentBuffer->reset();  // set cur_ to data_
    }
    if (!swap_nextBuffer) {
      assert(!buffersToWrite.empty());
      swap_nextBuffer = std::move(buffersToWrite.back());
      buffersToWrite.pop_back();
      swap_nextBuffer->reset();  // set cur_ to data_
    }
    buffersToWrite.clear();
    output.flush();
  }
  output.flush();
}

AsyncLogging::AsyncLogging(const std::string& dir, size_t rollSize, time_t flushInterval)
    : running_(true),
      dir_(dir),
      rollSize_(rollSize),
      flushInterval_(flushInterval),
      latch_(1),
      currentBuffer_(new Buffer),
      nextBuffer_(new Buffer) {}

AsyncLogging::~AsyncLogging() {
  if (running_) {
    stop();
  }
}

void AsyncLogging::append(const char* logline, int len) {
  std::unique_lock<std::mutex> lock(mutex_);
  // if the current buffer is not full, we will append the logline to the current buffer
  if (currentBuffer_->avail() > len) {
    currentBuffer_->append(logline, len);
  } else {
    // if the current buffer is full, we will push it to the buffers_ vector and swap it with the nextBuffer_
    buffers_.push_back(std::move(currentBuffer_));
    if (nextBuffer_) {
      currentBuffer_ = std::move(nextBuffer_);
    } else {
      // create a new buffer
      currentBuffer_.reset(new Buffer);
    }
    currentBuffer_->append(logline, len);
    cond_.notify_one();
  }
}
