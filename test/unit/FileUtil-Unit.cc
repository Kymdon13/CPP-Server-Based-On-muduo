#include <sys/stat.h>
#include <unistd.h>

#include <iostream>

// #include <benchmark/benchmark.h>

#include "base/FileUtil.h"

// stat
int main() {
  using namespace std;
  string path = "/media/fastData/wzy/code/ctest/cpp/src/inode/main.cc";

  struct stat st;

  while (1) {
    stat(path.c_str(), &st);
    cout << "st.st_mode: " << st.st_mode << endl;
    cout << "st.st_ino: " << st.st_ino << endl;
    cout << "st.st_dev: " << st.st_dev << endl;
    cout << "st.st_nlink: " << st.st_nlink << endl;
    cout << "st.st_uid: " << st.st_uid << endl;
    cout << "st.st_gid: " << st.st_gid << endl;
    cout << "st.st_atime: " << st.st_atime << endl;
    cout << "st.st_mtime: " << st.st_mtime << endl;
    cout << "st.st_ctime: " << st.st_ctime << endl;
    sleep(3);
  }
}
