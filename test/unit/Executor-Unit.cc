#include <iostream>

#include <string>

#include "base/Executor.h"

int main() {
    std::string code = R"(
int main() {
    int a = 0;
    int b = 10;
    printf("%d", 5);
    std::cout << "hello from cpp!" << std::endl;
    return 0;
}
    )";
    Executor executor;
    executor.exec(code);
    ExecResult result = executor.result();
    std::cout << "Exit code: " << result.exit_code << std::endl;
    std::cout << "Stdout: " << result.stdout_str << std::endl;
    std::cout << "Stderr: " << result.stderr_str << std::endl;
}