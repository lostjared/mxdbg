#include "argz.hpp"
#include "mxdbg/debugger.hpp"
#include "mxdbg/process.hpp"
#include "mxdbg/version_info.hpp"
#include <array>
#include <atomic>
#include <chrono>
#include <curl/curl.h>
#include <filesystem>
#include <readline/history.h>
#include <readline/readline.h>
#include <signal.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

namespace {
size_t discard_response(char *contents, size_t size, size_t count, void *) {
    (void)contents;
    return size * count;
}

std::string ollama_url(std::string host) {
    if (host.find(':') == std::string::npos) {
        host += ":11434";
    }
    return "http://" + host + "/api/tags";
}

bool check_ollama_connection(const std::string &host, std::string &error) {
    const CURLcode global_result = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (global_result != CURLE_OK) {
        error = std::string("failed to initialize libcurl: ") + curl_easy_strerror(global_result);
        return false;
    }

    CURL *curl = curl_easy_init();
    if (curl == nullptr) {
        error = "failed to initialize libcurl";
        curl_global_cleanup();
        return false;
    }

    const std::string url = ollama_url(host);
    std::array<char, CURL_ERROR_SIZE> curl_error{};
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discard_response);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_error.data());
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

    const CURLcode result = curl_easy_perform(curl);
    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    if (result != CURLE_OK) {
        error = curl_error[0] != '\0' ? curl_error.data() : curl_easy_strerror(result);
        return false;
    }
    if (status < 200 || status >= 300) {
        error = "Ollama returned HTTP " + std::to_string(status);
        return false;
    }
    return true;
}

bool show_ollama_connection_progress(const std::string &host, const std::string &model) {
    const bool interactive = isatty(STDOUT_FILENO);
    std::atomic<bool> finished = false;
    std::thread spinner;

    if (interactive) {
        spinner = std::thread([&finished, &host]() {
            constexpr char frames[] = {'|', '/', '-', '\\'};
            size_t frame = 0;
            while (!finished.load()) {
                std::cout << "\rConnecting to Ollama at " << host << " ["
                          << frames[frame++ % std::size(frames)] << "]" << std::flush;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });
    } else {
        std::cout << "Connecting to Ollama at " << host << "..." << std::endl;
    }

    std::string error;
    const bool connected = check_ollama_connection(host, error);
    finished = true;
    if (spinner.joinable()) {
        spinner.join();
        std::cout << "\r\033[2K";
    }

    if (connected) {
        std::cout << "Connected to Ollama at " << host << " (model: " << model << ")" << std::endl;
    } else {
        std::cerr << "Warning: could not connect to Ollama at " << host << ": " << error << std::endl;
    }
    return connected;
}
} // namespace

struct Arguments {
    pid_t p_id = -1;
    std::filesystem::path path;
    std::string args_str;
    bool dump_asm = false;
    bool enable_ollama = true;
};

Arguments parse_args(int argc, char **argv) {
    Arguments args;
    mx::Argz<std::string> parser(argc, argv);
    try {
        parser.addOptionSingleValue('p', "Process ID to attach to")
            .addOptionDoubleValue('P', "pid", "Process ID to attach to")
            .addOptionDoubleValue('R', "path", "Path to the executable or script")
            .addOptionSingleValue('r', "Path to executable or script to run")
            .addOptionDoubleValue('A', "args", "Additional arguments for the process")
            .addOptionSingleValue('a', "Additional arguments for the process")
            .addOptionSingleValue('e', "Dump Assembly of the executable")
            .addOptionSingleValue('d', "Dump Assembly of the executable")
            .addOptionDoubleValue('D', "dump", "Dump Assembly of the executable")
            .addOptionDouble('O', "disable-ai", "Disable AI");

        int value = 0;
        mx::Argument<std::string> arg;
        while ((value = parser.proc(arg)) != -1) {
            switch (value) {
            case 'O':
                args.enable_ollama = false;
                break;
            case 'e':
            case 'd':
            case 'D':
                args.dump_asm = true;
                args.path = std::filesystem::path(arg.arg_value);
                if (!args.path.empty() && !std::filesystem::exists(args.path)) {
                    std::cerr << "Error: Path does not exist: " << args.path << std::endl;
                    exit(EXIT_FAILURE);
                }
                break;
            case 'p':
            case 'P':
                args.p_id = std::stoi(arg.arg_value);
                break;
            case 'R':
            case 'r':
                args.path = std::filesystem::path(arg.arg_value);
                break;
            case 'a':
            case 'A':
                args.args_str = arg.arg_value;
                break;
            case '-':
            default:
                args.path = std::filesystem::path(arg.arg_value);
                break;
            }
        }
        if (args.p_id <= 0 && args.path.empty()) {
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
    std::cout << version_info << std::endl;
    Arguments args = parse_args(argc, argv);
    if (args.enable_ollama) {
        const char *mxdbg_host = getenv("MXDBG_HOST");
        const char *mxdbg_model = getenv("MXDBG_MODEL");
        if (mxdbg_host != nullptr && mxdbg_model != nullptr && mxdbg_host[0] != '\0' && mxdbg_model[0] != '\0') {
            show_ollama_connection_progress(mxdbg_host, mxdbg_model);
        }
    }
    mx::Debugger debugger(args.enable_ollama);
    std::string history_filename;
    try {
        const char *home_folder = getenv("HOME");
        if (home_folder) {
            history_filename = std::string(home_folder) + "/.mxdbg_history";
        } else {
            history_filename = "./mxdbg_history";
        }
        read_history(history_filename.c_str());
        if (args.dump_asm) {
            if (args.path.empty()) {
                std::cerr << "Error: No path provided for dumping assembly." << std::endl;
                return 1;
            }
            debugger.dump_file(args.path);
            return 0;
        }
        if (args.p_id > 0) {
            if (!debugger.attach(args.p_id)) {
                return 1;
            }
            std::cout << "Attached to process with PID: " << debugger.get_pid() << std::endl;
        } else if (!args.path.string().empty()) {
            if (!debugger.launch(args.path, args.args_str)) {
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
                if (!debugger.command(command)) {
                    break;
                } else
                    continue;
            } else {
                free(line);
            }
        }
        while (debugger.is_running()) {
            kill(debugger.get_pid(), SIGTERM);
            debugger.detach();
            debugger.wait_for_stop();
            if (debugger.get_pid() == -1) {
                // std::cout << "No process running." << std::endl;
            } else {
                std::cout << "Process with PID: " << std::dec << debugger.get_pid() << " has stopped." << std::endl;
            }
        }
        if (debugger.get_pid() == -1) {
            // std::cout << "No process running." << std::endl;
        } else {
            std::cout << "Process with PID: " << std::dec << debugger.get_pid() << " has exited." << std::endl;
        }
        write_history(history_filename.c_str());
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        write_history(history_filename.c_str());
        return 1;
    }
    return 0;
}
