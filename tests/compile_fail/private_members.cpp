// This file must NOT compile: it shows that code outside AirportBST cannot reach
// the root pointer or the Node type. Check with:
//   g++ -std=c++17 -fsyntax-only -Isrc tests/compile_fail/private_members.cpp
// Expected: two errors, one for 'root' and one for 'Node' (both private).

#include "AirportBST.h"

int main() {
    AirportBST tree;
    tree.root = nullptr;          // error: 'root' is a private member
    AirportBST::Node* n = nullptr; // error: 'Node' is a private member
    (void)n;
    return 0;
}
