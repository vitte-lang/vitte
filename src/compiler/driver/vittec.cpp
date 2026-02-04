#include "driver.hpp"

#include <exception>
#include <iostream>
#include <csignal>
#include <cstdlib>

using vitte::driver::run;

/* -------------------------------------------------
 * Signal handling
 * ------------------------------------------------- */

static void signal_handler(int sig) {
    const char* name = nullptr;

    switch (sig) {
        case SIGSEGV: name = "SIGSEGV"; break;
        case SIGABRT: name = "SIGABRT"; break;
        case SIGFPE:  name = "SIGFPE";  break;
        case SIGILL:  name = "SIGILL";  break;
        default:      name = "UNKNOWN"; break;
    }

    std::cerr
        << "[vittec] fatal signal: "
        << name << " (" << sig << ")\n";

    std::_Exit(128 + sig);
}

static void install_signal_handlers() {
    std::signal(SIGSEGV, signal_handler);
    std::signal(SIGABRT, signal_handler);
    std::signal(SIGFPE,  signal_handler);
    std::signal(SIGILL,  signal_handler);
}

/* -------------------------------------------------
 * Entry point
 * ------------------------------------------------- */

int main(int argc, char** argv) {
    install_signal_handlers();

    try {
        return run(argc, argv);
    }
    catch (const std::exception& e) {
        std::cerr
            << "[vittec] uncaught C++ exception: "
            << e.what() << "\n";
    }
    catch (...) {
        std::cerr
            << "[vittec] unknown fatal error\n";
    }

    return EXIT_FAILURE;
}
