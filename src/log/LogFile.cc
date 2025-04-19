#include "LogFile.h"

#include <iostream>

#include "base/ProcInfo.h"
#include "timer/TimeStamp.h"

std::string LogFile::getLogFileName(TimeStamp& now) {
  std::string filename;
  filename.reserve(307);  // 15 + 1 + 255 + 32 + 4

  // time info
  filename += now.formattedString("%4d%02d%02d-%02d%02d%02d");  // 15 bytes

  filename += '.';  // 1 byte

  filename += ProcInfo::hostname();  // maximum 255 bytes
  // thread info
  char pidbuf[32];
  snprintf(pidbuf, sizeof(pidbuf), ".%d", ProcInfo::pid());
  filename += pidbuf;  // 32 bytes

  filename += ".log";  // 4 bytes
  return filename;
}

LogFile::LogFile(const std::string& dir, size_t rollSize, int checkEveryN, time_t period)
    : dir_(dir),
      rollSize_(rollSize),
      checkEveryN_(checkEveryN),
      lastRoll_(0),
      // lastFlush_(0),
      count_(0),
      periodStartingPoint_(0),
      period_(period) {
  // create log file
  TimeStamp now = TimeStamp::now();
  rollFile(now);
  // now it has been properly initialized
  periodStartingPoint_ = now.getSecond() / period_ * period_;
}

// // original code
// void LogFile::append(const char *logline, int len) {
//     // write to the buffer
//     file_->append(logline, len);

//     if (file_->writtenBytes() > rollSize_) {
//         TimeStamp now = TimeStamp::now();
//         rollFile(now);
//     } else {
//         ++count_;
//         // after checkEveryN_ times of writing
//         if (count_ >= checkEveryN_) {
//             count_ = 0;
//             TimeStamp now = TimeStamp::now();
//             time_t nowInSecond = now.getSecond();
//             // check if it is different periods
//             time_t thisPeriod = nowInSecond / period_ * period_;
//             if (thisPeriod != periodStartingPoint_) {
//                 periodStartingPoint_ = thisPeriod;
//                 rollFile(now);
//             } else if (nowInSecond - lastFlush_ > flushInterval_) {
//                 lastFlush_ = nowInSecond;
//                 file_->flush();
//             }
//         }
//     }
// }

// new append, remove lastFlush_
void LogFile::append(const char* logline, int len) {
  // write to the buffer
  file_->append(logline, len);

  if (file_->writtenBytes() > rollSize_) {
    TimeStamp now = TimeStamp::now();
    rollFile(now);
  } else {
    // since we hand over the flush to AsyncLogging, so we don't need to flush in LogFile,
    // but AsyncLogging will flush after every append, so we need to check if it is different periods in every append
    ++count_;
    // after checkEveryN_ times of writing
    if (count_ >= checkEveryN_) {
      count_ = 0;
      TimeStamp now = TimeStamp::now();
      time_t nowInSecond = now.getSecond();
      // check if it is different periods
      time_t thisPeriod = nowInSecond / period_ * period_;
      if (thisPeriod != periodStartingPoint_) {
        perror("thisPeriod != periodStartingPoint_");
        periodStartingPoint_ = thisPeriod;
        rollFile(now);
      }
    }
  }
}

// // original code
// void LogFile::flush() { file_->flush(); }

// void LogFile::rollFile(TimeStamp& now) {
//     time_t nowInSecond = now.getSecond();
//     // if the current timestamp is greater than lastRoll_
//     if (nowInSecond > lastRoll_) {
//         lastRoll_ = nowInSecond;
//         lastFlush_ = nowInSecond;
//         std::string logFileName = getLogFileName(now);
//         file_.reset(new FileUtil::AppendFile(dir_ + '/' + logFileName));
//     }
// }

// // remove lastFlush_
void LogFile::rollFile(TimeStamp& now) {
  time_t nowInSecond = now.getSecond();
  // if the current timestamp is greater than lastRoll_
  if (nowInSecond > lastRoll_) {
    lastRoll_ = nowInSecond;
    std::string logFileName = getLogFileName(now);
    file_.reset(new FileUtil::AppendFile(dir_ + '/' + logFileName));
  }
}
