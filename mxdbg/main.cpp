#include<mxdbg/process.hpp>
#include"argz.hpp"
#include<filesystem>

struct Arguments {
    int p_id;
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
     


int main(int argc, char **argv) {
    
    Arguments args = parse_args(argc, argv);

    return 0;
}