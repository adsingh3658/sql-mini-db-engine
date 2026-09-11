// ============================================================================
//  main.cpp  -  Mini SQL Database Engine demo + REPL
//  ---------------------------------------------------------------------------
//  Runs a scripted set of SQL statements to show the engine working, then drops
//  into an interactive SQL prompt.
//
//  Build:  g++ -std=c++17 -O2 src/main.cpp -o minidb.exe
//  Run:    ./minidb.exe
// ============================================================================
#include "database.hpp"
#include <iostream>
#include <string>

int main() {
    std::cout << "==========================================================\n";
    std::cout << "            MINI SQL DATABASE ENGINE (in-memory)           \n";
    std::cout << "==========================================================\n";

    db::Database database;

    // ---- Scripted demo so results show immediately -----------------------
    const char* script[] = {
        "CREATE TABLE users (id, name, age)",
        "INSERT INTO users VALUES (1, Alice, 30)",
        "INSERT INTO users VALUES (2, Bob, 25)",
        "INSERT INTO users VALUES (3, Carol, 35)",
        "INSERT INTO users VALUES (4, Dave, 28)",
    };
    for (const auto* stmt : script) {
        std::cout << "\nSQL> " << stmt << "\n";
        database.execute(stmt);
    }

    const char* queries[] = {
        "SELECT * FROM users",
        "SELECT name, age FROM users WHERE age > 28",
        "SELECT * FROM users ORDER BY age DESC",
    };
    for (const auto* q : queries) {
        std::cout << "\nSQL> " << q << "\n";
        database.execute(q);
    }

    // ---- Interactive REPL ------------------------------------------------
    std::cout << "\n--- Enter SQL (or 'quit'). Try your own queries! ---\n";
    std::string line;
    while (true) {
        std::cout << "SQL> ";
        if (!std::getline(std::cin, line) || line == "quit" || line.empty()) break;
        database.execute(line);
    }
    return 0;
}
