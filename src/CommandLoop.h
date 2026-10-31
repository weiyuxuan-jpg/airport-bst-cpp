#ifndef COMMAND_LOOP_H
#define COMMAND_LOOP_H

#include <iosfwd>
#include <string>
#include <vector>

// The text interface of the lookup service (docs/pseudocode.md, section 12).
//
// Reads one command per line from in and writes the answers to out, until "quit" or the
// end of the input. The startup commands run first (main passes "load FILE" when a file is
// named on the command line). Each command is printed after a "> " prompt when echo is
// true, so a session piped in from a file reads like the project's sample session.
// Returns 0 (the program's exit code).
int runCommandLoop(std::istream& in, std::ostream& out, bool echo,
                   const std::vector<std::string>& startup);

#endif
