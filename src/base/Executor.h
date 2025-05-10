#include <sys/wait.h>
#include <unistd.h>

#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

struct ExecResult {
  int exit_code;
  std::string stdout_str;
  std::string stderr_str;
};

class Executor {
 private:
  ExecResult compile_result_;
  ExecResult result_;

 public:
  Executor() = default;
  ~Executor() = default;

  ExecResult result() const { return result_; }
  ExecResult result_compile() const { return compile_result_; }

  void exec(const std::string& code) {
    if (code.empty()) {
      result_.exit_code = -1;
    }
    if (code.size() > 10 * 1024) {
      result_.exit_code = -2;
      result_.stdout_str = "Code is too long";
      result_.stderr_str = "";
    }
    // write into source file
    std::fstream file("main.cc", std::ios::out | std::ios::trunc);  // trunc to clear or create the file
    file << "// This is a test file\n";
    file << "#include <iostream>\n";
    file << "#include <string>\n";
    file << code;
    file.close();
    // compile
    char* const argv_compile[] = {
        (char*)"clang++",
        (char*)"./main.cc",
        (char*)"-o",
        (char*)"./main",
        nullptr
    };
    compile_result_ = execWithPipes("/usr/bin/clang++", argv_compile);
    // execute
    char* const argv_exec[] = {
        (char*)"main",
        nullptr
    };
    result_ = execWithPipes("./main", argv_exec);
  }

 private:
  ExecResult execWithPipes(const std::string& cmd, char* const argv[]) {
    int stdout_pipe[2], stderr_pipe[2];
    pipe(stdout_pipe);
    pipe(stderr_pipe);

    pid_t pid = fork();
    if (pid == 0) {
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);

        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        close(stderr_pipe[0]);
        close(stderr_pipe[1]);

        // char cwd[1024];
        // getcwd(cwd, sizeof(cwd));
        // std::cerr << "Before execv, cwd: " << cwd << std::endl;

        execv(cmd.c_str(), argv);
        perror("execv failed: ");
        _exit(127);
    }

    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    char buffer[4096];
    std::string stdout_output, stderr_output;

    ssize_t count;
    while ((count = read(stdout_pipe[0], buffer, sizeof(buffer))) > 0) {
      stdout_output.append(buffer, count);
    }
    while ((count = read(stderr_pipe[0], buffer, sizeof(buffer))) > 0) {
      stderr_output.append(buffer, count);
    }

    close(stdout_pipe[0]);
    close(stderr_pipe[0]);

    int status = 0;
    waitpid(pid, &status, 0);

    ExecResult result;
    result.exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    result.stdout_str = stdout_output;
    result.stderr_str = stderr_output;
    return result;
  }
};