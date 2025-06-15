#include<mxdbg/process.hpp>
#include"argz.hpp"
#include<filesystem>

struct Arguments {
    pid_t p_id = -1;
    std::filesystem::path path;
    std::vector<std::string> args;
};

Arguments parse_args(int argc, char **argv) {
    Arguments args;
    mx::Argz<std::string> parser(argc, argv);
    parser.addOptionSingle('p', "Process ID to attach to")
          .addOptionDoubleValue('P', "path", "Path to the executable or script")
          .addOptionSingleValue('a', "Additional arguments for the process");
    int value = 0;
    mx::Argument<std::string> arg;
    while((value = parser.proc(arg)) != -1) {
        switch(value) {
            case 'p':
                args.p_id = std::stoi(arg.arg_value);
                break;
            case '-':
                args.path = std::filesystem::path(arg.arg_value);
                break;
        }
    }
    if(args.p_id <= 0 && args.path.empty()) {
        std::cerr << "Error: No process ID or path provided." << std::endl;
        parser.help(std::cout);
        exit(EXIT_FAILURE);
    }
    return args;
}

class Debugger {
public:
    Debugger() = default;
    ~Debugger() = default;

    bool attach(pid_t pid) { 
        process = mx::Process::attach(pid);
        if(!process) {
            std::cerr << "Failed to attach to process with PID: " << pid << std::endl;
            return false;
        }
        p_id = pid;
        return true;
    }

    bool launch(const std::filesystem::path &exe, const std::vector<std::string> &args) {
        process = mx::Process::launch(exe, args);
        if(!process) {
            std::cerr << "Failed to launch process: " << exe << std::endl;
            return false;
        }
        p_id = process->get_pid();
        return true;
    }

    void conntinue_execution() {
        if(process) {
            process->continue_execution();
        } else {
            std::cerr << "No process attached or launched." << std::endl;
        }
    }

    void wait_for_stop() {
        if(process) {
            process->wait_for_stop();
        } else {
            std::cerr << "No process attached or launched." << std::endl;
        }
    }

    pid_t get_pid() const {
        return p_id;
    }

    bool is_running() const {
        return process && process->is_running();
    }
private:

    std::unique_ptr<mx::Process> process;
    std::vector<std::string> args;
    pid_t p_id = -1;    
};
     


int main(int argc, char **argv) {
    Arguments args = parse_args(argc, argv);
    Debugger debugger;
    try {
        if(args.p_id > 0) {
            if(!debugger.attach(args.p_id)) {
                return 1;
            }
        } else if(!args.path.string().empty()) {
            if(!debugger.launch(args.path, args.args)) {
                return 1;
            } 
        }
        std::cout << "Process stopped. PID: " << debugger.get_pid() << std::endl;
        debugger.conntinue_execution();
        std::cout << "Continuing execution of process with PID: " << debugger.get_pid() << std::endl;   
        while(debugger.is_running()) {
            debugger.wait_for_stop();
            std::cout << "Process with PID: " << debugger.get_pid() << " has stopped." << std::endl;
            
        }
        std::cout << "Process with PID: " << debugger.get_pid() << " has exited." << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}