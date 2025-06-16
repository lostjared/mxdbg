#include<mxdbg/process.hpp>
#include<mxdbg/debugger.hpp>
#include"argz.hpp"
#include<filesystem>
#include<readline/readline.h>
#include<readline/history.h>
#include<thread>
#include<chrono>

struct Arguments {
    pid_t p_id = -1;
    std::filesystem::path path;
    std::string args_str;
};

Arguments parse_args(int argc, char **argv) {
    Arguments args;
    mx::Argz<std::string> parser(argc, argv);
    try {
        parser.addOptionSingleValue('p', "Process ID to attach to")
            .addOptionDoubleValue('x', "path", "Path to the executable or script")
            .addOptionDoubleValue('a', "args", "Additional arguments for the process");
        int value = 0;
        mx::Argument<std::string> arg;
        while((value = parser.proc(arg)) != -1) {
            switch(value) {
                case 'p':
                    args.p_id = std::stoi(arg.arg_value);
                    break;
                case 'x':
                    args.path = std::filesystem::path(arg.arg_value);
                    break;
                case 'a':
                    args.args_str = arg.arg_value;
                    break;
                case '-':
                default:
                    args.path = std::filesystem::path(arg.arg_value);
                    break;
                }
        }    
        if(args.p_id <= 0 && args.path.empty()) {
            std::cerr << "Error: No process ID or path provided." << std::endl;
            parser.help(std::cout);
            exit(EXIT_FAILURE);
        }
    } catch (const mx::ArgException<std::string> &e) {
        std::cerr << "Argument parsing error: " << e.text() << std::endl;
        parser.help(std::cout);
        exit(EXIT_FAILURE);
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        parser.help(std::cout);
        exit(EXIT_FAILURE);
    }
    return args;
}

int main(int argc, char **argv) {
    Arguments args = parse_args(argc, argv);
    mx::Debugger debugger;
    std::string history_filename;
    try {
        const char *home_folder = getenv("HOME");
        if(home_folder) {
            history_filename = std::string(home_folder) + "/.mxdbg_history";
        } else {
            history_filename = "./mxdbg_history";
        }
        read_history(history_filename.c_str());
        if(args.p_id > 0) {
            if(!debugger.attach(args.p_id)) {
                return 1;
            }
            std::cout << "Attached to process with PID: " << debugger.get_pid() << std::endl;
        } else if(!args.path.string().empty()) {
            if(!debugger.launch(args.path, args.args_str)) {
                return 1;
            } 
            std::cout << "Process launched with PID: " << debugger.get_pid() << std::endl;
        }
        std::cout << "Process stopped. PID: " << debugger.get_pid() << std::endl;
        char *line;
        while ((line = readline("mx $> ")) != nullptr) {
            if (strlen(line) > 0) {
                add_history(line);
                std::string command(line);
                free(line);    
                if(!debugger.command(command)) {
                    break;
                }
            } else {
                free(line);
            }
        }
        while(debugger.is_running()) {
            debugger.wait_for_stop();
            std::cout << "Process with PID: " << debugger.get_pid() << " has stopped." << std::endl;
        }
        std::cout << "Process with PID: " << debugger.get_pid() << " has exited." << std::endl;
        write_history(history_filename.c_str());
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        write_history(history_filename.c_str());
        return 1;
    }
    return 0;
}