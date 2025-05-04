#include <unistd.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>

struct ExecResult {
    int exit_code;
    std::string stdout_str;
    std::string stderr_str;
};

class Executor {
private:
    ExecResult result_;

public:
    Executor() = default;
    ~Executor() = default;

    ExecResult result() const { return result_; }

    bool exec(const std::string& code) {
        if (code.empty()) {
            result_.exit_code = -1;
        }
        if (code.size() > 4096) {
            result_.exit_code = -2;
            result_.stdout_str = "Command is too long";
            result_.stderr_str = "";
        }
        // write into source file
        std::fstream file("main.cc", std::ios::out | std::ios::trunc);  // trunc to clear or create the file
        file << "// This is a test file\n";
        file << "#include <iostream>\n";
        file << "#include <string>\n";
        file << code;
        // compile
        char* const argv_compile[] = {"g++", "main.cc", "-o", "main", nullptr};
        result_ = execWithPipes("/usr/bin/g++", argv_compile);
        // execute
        char* const argv_exec[] = {"./main", nullptr};
        result_ = execWithPipes("./main", argv_exec);
    }

private:
ExecResult execWithPipes(const std::string& cmd, char* const* argv) {
    int stdout_pipe[2], stderr_pipe[2];
    pipe(stdout_pipe);
    pipe(stderr_pipe);

    pid_t pid = fork();
    if (pid == 0) {
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);

        close(stdout_pipe[0]); close(stdout_pipe[1]);
        close(stderr_pipe[0]); close(stderr_pipe[1]);

        execv(cmd.c_str(), argv);
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