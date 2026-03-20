/* 
    MXDBG - Debugger with AI 
    coded by Jared Bruni (jaredbruni@protonmail.com)
    https://lostsidedead.biz
*/
#include "mxdbg/exception.hpp"
#include <csignal>
#include <iostream>
#include <format>

namespace mx {
    std::string format_hex64(uint64_t value) {
        return std::format("0x{:016X}", value);
    }

    std::string format_hex32(uint32_t value) {
        return std::format("0x{:08X}", value);
    }

    std::string format_hex16(uint16_t value) {
        return std::format("0x{:04X}", value);
    }
    
    std::string format_hex8(uint8_t value) {
        return std::format("0x{:02X}", static_cast<unsigned>(value));
    }

    std::string format_hex_no_prefix(uint64_t value) {
        return std::format("{:X}", value);
    }


    std::string format_signal(uint32_t sig) {
        switch (sig) {
            case SIGHUP:  return "SIGHUP (Hangup)";
            case SIGINT:  return "SIGINT (Interrupt)";
            case SIGQUIT: return "SIGQUIT (Quit)";
            case SIGILL:  return "SIGILL (Illegal instruction)";
            case SIGTRAP: return "SIGTRAP (Trace/breakpoint trap)";
            case SIGABRT: return "SIGABRT (Aborted)";
            case SIGBUS:  return "SIGBUS (Bus error)";
            case SIGFPE:  return "SIGFPE (Floating point exception)";
            case SIGKILL: return "SIGKILL (Killed)";
            case SIGUSR1: return "SIGUSR1 (User defined signal 1)";
            case SIGSEGV: return "SIGSEGV (Segmentation fault)";
            case SIGUSR2: return "SIGUSR2 (User defined signal 2)";
            case SIGPIPE: return "SIGPIPE (Broken pipe)";
            case SIGALRM: return "SIGALRM (Alarm clock)";
            case SIGTERM: return "SIGTERM (Terminated)";
            case SIGCHLD: return "SIGCHLD (Child exited)";
            case SIGCONT: return "SIGCONT (Continued)";
            case SIGSTOP: return "SIGSTOP (Stopped)";
            case SIGTSTP: return "SIGTSTP (Stopped (tty input))";
            case SIGTTIN: return "SIGTTIN (Stopped (tty input))";
            case SIGTTOU: return "SIGTTOU (Stopped (tty output))";
            case SIGURG:  return "SIGURG (Urgent I/O condition)";
            case SIGXCPU: return "SIGXCPU (CPU time limit exceeded)";
            case SIGXFSZ: return "SIGXFSZ (File size limit exceeded)";
            case SIGVTALRM: return "SIGVTALRM (Virtual timer expired)";
            case SIGPROF: return "SIGPROF (Profiling timer expired)";
            case SIGWINCH: return "SIGWINCH (Window size changed)";
            case SIGIO:   return "SIGIO (I/O possible)";
            case SIGPWR:  return "SIGPWR (Power failure)";
            case SIGSYS:  return "SIGSYS (Bad system call)";
            
            default:
                if (sig >= SIGRTMIN && sig <= SIGRTMAX) {
                    return std::format("SIGRTMIN+{} (Real-time signal)", sig - SIGRTMIN);
                }
                return std::format("Unknown signal ({})", sig);
        }
    }
}