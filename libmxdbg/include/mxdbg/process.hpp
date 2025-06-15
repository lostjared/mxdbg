#ifndef _PROCESS_HPP_H_
#define _PROCESS_HPP_H_

#include <string_view>
#include <string>
#include <filesystem>
#include <memory>
#include <vector>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
namespace mx {

    class Process {
    public:
        static std::unique_ptr<Process> launch(const std::filesystem::path &executable, const char *args);
        static std::unique_ptr<Process> attach(pid_t pid);
        virtual ~Process() = default;
    private:
        Process() = delete;
        Process(const Process &) = delete;
        Process &operator=(const Process &) = delete;

        pid_t pid;
        std::string executable;
        std::vector<std::string> args;    
    };
}

#endif