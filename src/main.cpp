// Airport Code Lookup Service with a Binary Search Tree (EECE 2560 mini project).
//
//   ./airports                    start with an empty tree; type "load data/airports.dat"
//   ./airports data/airports.dat  load the file first, then read commands
//   ./airports data/airports.dat < session.txt   run a script; commands are echoed
//
// Type "help" for the commands.

#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include <unistd.h>

#include "CommandLoop.h"

int main(int argc, char* argv[]) {
    std::vector<std::string> startup;
    if (argc > 1) startup.push_back(std::string("load ") + argv[1]);

    // Typed commands already show on the terminal; piped ones are echoed after the prompt.
    bool echo = !isatty(fileno(stdin));
    return runCommandLoop(std::cin, std::cout, echo, startup);
}
