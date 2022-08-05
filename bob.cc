#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

struct CommandLineArgs {
    const char* bob_file;
};

void fatal(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(1);
}

#define USAGE "usage: bob BOBFILE"
void readCommandLineArgs(int argc, char** argv, CommandLineArgs* out_args) {
    if (argc < 2) {
        fatal(USAGE);
    }
    if (argc > 2) {
        fatal(USAGE);
    }
    out_args->bob_file = argv[1];
}

const char* readEntireFile(const char* filename) {
    std::ifstream file_stream(filename);
    std::string content(
        (std::istreambuf_iterator<char>(file_stream)),
        (std::istreambuf_iterator<char>())
    );
    char* str = new char[content.size()];
    content.copy(str, content.size());
    return str;
}

struct Command {
    const char* program_name;
    const char** arguments;
    size_t argument_count;
};

Command* parseCommand(const char* source) {
    Command* command = new Command;
    const char* delimiters = " \t\n";
    const char* iter = source;
    bool ended = false;
    
    auto get_next_token =
        [&iter, &delimiters, &ended]() {
            const char* end = strpbrk(iter, delimiters);
            if (end == nullptr) {
                ended = true;
                size_t len = strlen(iter);
                char* token = new char[len + 1];
                strncpy(token, iter, len);
                token[len] = '\0';
                return (const char*) token;
            }
            char* token = new char[end - iter + 1];
            strncpy(token, iter, end - iter);
            token[end - iter] = '\0';
            iter = end + 1;
            return (const char*) token;
        };
    
    command->program_name = get_next_token();

    std::vector<const char*> arguments;
    arguments.push_back(command->program_name);
    while (!ended) {
        const char* token = get_next_token();
        if (token[0] == '\0') {
            delete[] ((char*) token);
            continue;
        }
        arguments.push_back(token);
    }
    arguments.push_back(nullptr);
    command->arguments = new const char*[arguments.size()];
    command->argument_count = arguments.size();
    for (size_t i = 0; i < arguments.size(); i++) {
        command->arguments[i] = arguments[i];
    }
    
    return command;
}

void destroyCommand(Command* command) {
    delete[] command->program_name;
    for (size_t i = 1; i < command->argument_count; i++) {
        delete[] command->arguments[i];
    }
    delete[] command->arguments;
    delete command;
}

struct CommandResults {
    const char* out;
    const char* err;
    int32_t code;
};

typedef int fd_t;

void runCommand(Command* command) {
    enum {
        READ_END = 0,
        WRITE_END = 1
    };
    fd_t stdout_pipe[2];
    fd_t stderr_pipe[2];
    pipe(stdout_pipe);
    pipe(stderr_pipe);

    pid_t pid = fork();
    if (pid == 0) {
        // child
        dup2(stdout_pipe[WRITE_END], STDOUT_FILENO);
        dup2(stderr_pipe[WRITE_END], STDERR_FILENO);

        execvp(command->program_name, (char* const*) command->arguments);
        // an error occured
        perror("child process");
    }

    // parent

    fcntl(stdout_pipe[READ_END], F_SETFL, O_NONBLOCK);
    fcntl(stderr_pipe[READ_END], F_SETFL, O_NONBLOCK);
    
    std::vector<char> stdout_buffer(64);
    size_t stdout_index = 0;

    std::vector<char> stderr_buffer(64);
    size_t stderr_index = 0;

    auto read_into_buffer =
        [](fd_t fd, std::vector<char>& buffer, size_t& index) -> bool {
            ssize_t ret = read(fd, buffer.data() + index, 64);
            if (ret == 0) {
                return false;
            }
            if (ret < 0) {
                return false; // TEMPORARY
            }
            #if 0
            for (size_t i = index; i < index + ret; i++) {
                std::cout << buffer[i];
            }
            #endif
            buffer.resize(buffer.size() + ret);
            index += ret;
            return true;
        };

    int proc_status;
    bool ended = false;
    bool pipe_empty = true;
    while (!(ended && pipe_empty)) {
        // try to read, and update pipe_empty with whether we did succesfully read
        pipe_empty = true;
        pipe_empty = pipe_empty && !read_into_buffer(stdout_pipe[READ_END], stdout_buffer, stdout_index);
        pipe_empty = pipe_empty && !read_into_buffer(stderr_pipe[READ_END], stderr_buffer, stderr_index);
        
        // has the child process exited?
        if (!ended) {
            pid_t wait_result = waitpid(pid, &proc_status, WNOHANG);
            if (wait_result != 0) {
                ended = true;
            }
        }
    }

    auto print_buffer =
        [](const std::vector<char>& buffer) {
            for (char c : buffer) {
                std::cout << c;
            }
            std::cout << '\n';
        };
    std::cout << "====STDOUT====\n";
    print_buffer(stdout_buffer);
    std::cout << "====STDERR====\n";
    print_buffer(stderr_buffer);
}

int main(int argc, char** argv) {
    CommandLineArgs args;
    readCommandLineArgs(argc, argv, &args);

    const char* source = readEntireFile(args.bob_file);

    Command* command = parseCommand(source);
    /*
    std::cout << "'" << command->program_name << "':\n";
    for (size_t i = 0; i < command->argument_count; i++) {
        std::cout << "  '" << ((command->arguments[i] == nullptr) ? "(null)" : command->arguments[i]) << "'\n";
        }*/
    runCommand(command);
    destroyCommand(command);

    return 0;
}
