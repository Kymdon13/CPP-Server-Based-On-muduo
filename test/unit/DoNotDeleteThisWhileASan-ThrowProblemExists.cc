#include <iostream>

/**
 * if you run this code while enabling option -fsanitize=address in CMAKE_CXX_FLAGS_DEBUG or
 * CMAKE_EXE_LINKER_FLAGS in the CMakeLists.txt, you will get the following error:
 * ERROR: AddressSanitizer: alloc-dealloc-mismatch (operator new vs free),
 * note that the error is not in the code, but in the AddressSanitizer,
 * you must find compatible libasan.so to link with your code so that when you run
 * ldd <executable>, you will see the following:
 * libasan.so.6 or some other version
 */
void foo() { throw std::runtime_error("foo"); }

int main() {
  try {
    foo();
  } catch (const std::runtime_error& e) {
    std::cerr << "Caught exception: " << e.what() << std::endl;
  } catch (...) {
    std::cerr << "Caught unknown exception" << std::endl;
  }
  return 0;
}
