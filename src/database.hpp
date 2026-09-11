// ============================================================================
//  database.hpp
//  ---------------------------------------------------------------------------
//  A mini in-memory SQL database engine.
//
//  It parses and executes a useful subset of SQL:
//     CREATE TABLE users (id, name, age)
//     INSERT INTO users VALUES (1, Alice, 30)
//     SELECT * FROM users
//     SELECT name, age FROM users WHERE age > 25
//     SELECT * FROM users ORDER BY age DESC
//
//  WHAT YOU LEARN
//  A database has three layers, all present here in miniature:
//     1. PARSER    - turn SQL text into a structured command.
//     2. STORAGE   - hold tables (columns + rows) in memory.
//     3. EXECUTOR  - run the command against storage (filter, project, sort).
//
//  Values are stored as strings but compared numerically when both sides look
//  like numbers, so `age > 25` works as expected.
// ============================================================================
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <cstdlib>   // strtod
#include <cctype>    // isspace, toupper
#include <stdexcept>

namespace db {

// A table = ordered column names + a list of rows (each row = values per column).
struct Table {
    std::vector<std::string> columns;
    std::vector<std::vector<std::string>> rows;

    int column_index(const std::string& name) const {
        for (size_t i = 0; i < columns.size(); ++i)
            if (columns[i] == name) return (int)i;
        return -1;
    }
};

class Database {
public:
    // Execute one SQL statement. Prints results or an error message.
    void execute(const std::string& sql) {
        auto tokens = tokenize(sql);
        if (tokens.empty()) return;
        std::string verb = upper(tokens[0]);

        try {
            if (verb == "CREATE")      create_table(tokens);
            else if (verb == "INSERT") insert(tokens);
            else if (verb == "SELECT") select(tokens);
            else if (verb == "TABLES") list_tables();
            else std::cout << "ERROR: unknown command '" << tokens[0] << "'\n";
        } catch (const std::exception& e) {
            std::cout << "ERROR: " << e.what() << "\n";
        }
    }

private:
    std::unordered_map<std::string, Table> tables_;

    // ----- tiny tokenizer: splits words and the punctuation ( ) , -----------
    static std::vector<std::string> tokenize(const std::string& s) {
        std::vector<std::string> out;
        std::string cur;
        auto flush = [&] { if (!cur.empty()) { out.push_back(cur); cur.clear(); } };
        for (char ch : s) {
            if (ch == '(' || ch == ')' || ch == ',') { flush(); out.push_back(std::string(1, ch)); }
            else if (std::isspace((unsigned char)ch)) flush();
            else cur += ch;
        }
        flush();
        return out;
    }

    static std::string upper(std::string s) {
        for (auto& c : s) c = (char)toupper((unsigned char)c);
        return s;
    }

    // Is this string a number? (used to decide numeric vs string comparison)
    static bool is_number(const std::string& s) {
        if (s.empty()) return false;
        char* end = nullptr;
        std::strtod(s.c_str(), &end);
        return end == s.c_str() + s.size();
    }

    // Compare two values; numeric if both look numeric, else lexicographic.
    static bool compare(const std::string& a, const std::string& op, const std::string& b) {
        if (is_number(a) && is_number(b)) {
            double x = std::stod(a), y = std::stod(b);
            if (op == "=") return x == y;  if (op == "!=") return x != y;
            if (op == ">") return x >  y;  if (op == "<")  return x <  y;
            if (op == ">=") return x >= y; if (op == "<=") return x <= y;
        } else {
            if (op == "=") return a == b;  if (op == "!=") return a != b;
            if (op == ">") return a >  b;  if (op == "<")  return a <  b;
            if (op == ">=") return a >= b; if (op == "<=") return a <= b;
        }
        return false;
    }

    // Collect the comma-separated identifiers inside a (...) group starting at i.
    static std::vector<std::string> read_paren_list(const std::vector<std::string>& t, size_t& i) {
        std::vector<std::string> items;
        if (i >= t.size() || t[i] != "(") throw std::runtime_error("expected '('");
        ++i;
        while (i < t.size() && t[i] != ")") {
            if (t[i] != ",") items.push_back(t[i]);
            ++i;
        }
        if (i >= t.size()) throw std::runtime_error("missing ')'");
        ++i; // skip ')'
        return items;
    }

    // ----- CREATE TABLE name (c1, c2, ...) ---------------------------------
    void create_table(const std::vector<std::string>& t) {
        // t = CREATE TABLE <name> ( cols )
        if (t.size() < 4 || upper(t[1]) != "TABLE") throw std::runtime_error("syntax: CREATE TABLE name (cols)");
        const std::string name = t[2];
        size_t i = 3;
        Table tbl;
        tbl.columns = read_paren_list(t, i);
        tables_[name] = tbl;
        std::cout << "Created table '" << name << "' with " << tbl.columns.size() << " columns.\n";
    }

    // ----- INSERT INTO name VALUES (v1, v2, ...) ---------------------------
    void insert(const std::vector<std::string>& t) {
        if (t.size() < 5 || upper(t[1]) != "INTO") throw std::runtime_error("syntax: INSERT INTO name VALUES (...)");
        const std::string name = t[2];
        auto it = tables_.find(name);
        if (it == tables_.end()) throw std::runtime_error("no such table: " + name);
        if (upper(t[3]) != "VALUES") throw std::runtime_error("expected VALUES");
        size_t i = 4;
        auto values = read_paren_list(t, i);
        if (values.size() != it->second.columns.size())
            throw std::runtime_error("column count mismatch");
        it->second.rows.push_back(values);
        std::cout << "1 row inserted.\n";
    }

    // ----- SELECT cols FROM name [WHERE col op val] [ORDER BY col [ASC|DESC]]
    void select(const std::vector<std::string>& t) {
        size_t i = 1;
        std::vector<std::string> wanted;             // requested columns (or *)
        while (i < t.size() && upper(t[i]) != "FROM") {
            if (t[i] != ",") wanted.push_back(t[i]);
            ++i;
        }
        if (i >= t.size()) throw std::runtime_error("expected FROM");
        ++i;                                         // skip FROM
        const std::string name = t[i++];
        auto it = tables_.find(name);
        if (it == tables_.end()) throw std::runtime_error("no such table: " + name);
        const Table& tbl = it->second;

        // Optional WHERE clause: col op value
        std::string w_col, w_op, w_val; bool has_where = false;
        std::string order_col; bool order_desc = false, has_order = false;
        while (i < t.size()) {
            std::string kw = upper(t[i]);
            if (kw == "WHERE") {
                w_col = t[i + 1]; w_op = t[i + 2]; w_val = t[i + 3];
                has_where = true; i += 4;
            } else if (kw == "ORDER") {
                // ORDER BY col [ASC|DESC]
                order_col = t[i + 2];
                if (i + 3 < t.size() && upper(t[i + 3]) == "DESC") order_desc = true;
                has_order = true;
                i += (i + 3 < t.size() && (upper(t[i+3])=="DESC"||upper(t[i+3])=="ASC")) ? 4 : 3;
            } else ++i;
        }

        // Which columns to output (expand '*').
        std::vector<int> out_cols;
        if (wanted.size() == 1 && wanted[0] == "*") {
            for (size_t c = 0; c < tbl.columns.size(); ++c) out_cols.push_back((int)c);
        } else {
            for (auto& w : wanted) {
                int idx = tbl.column_index(w);
                if (idx < 0) throw std::runtime_error("no such column: " + w);
                out_cols.push_back(idx);
            }
        }

        // Filter rows by WHERE.
        std::vector<const std::vector<std::string>*> result;
        int w_idx = has_where ? tbl.column_index(w_col) : -1;
        if (has_where && w_idx < 0) throw std::runtime_error("no such column: " + w_col);
        for (const auto& row : tbl.rows)
            if (!has_where || compare(row[w_idx], w_op, w_val)) result.push_back(&row);

        // Sort by ORDER BY.
        if (has_order) {
            int o_idx = tbl.column_index(order_col);
            if (o_idx < 0) throw std::runtime_error("no such column: " + order_col);
            std::sort(result.begin(), result.end(),
                [&](const std::vector<std::string>* a, const std::vector<std::string>* b) {
                    bool less = compare((*a)[o_idx], "<", (*b)[o_idx]);
                    return order_desc ? !less && (*a)[o_idx] != (*b)[o_idx] : less;
                });
        }

        print_result(tbl, out_cols, result);
    }

    void print_result(const Table& tbl, const std::vector<int>& cols,
                       const std::vector<const std::vector<std::string>*>& rows) {
        // Header.
        for (int c : cols) std::cout << tbl.columns[c] << "\t";
        std::cout << "\n";
        for (int c : cols) { (void)c; std::cout << "------\t"; }
        std::cout << "\n";
        for (auto* row : rows) {
            for (int c : cols) std::cout << (*row)[c] << "\t";
            std::cout << "\n";
        }
        std::cout << "(" << rows.size() << " rows)\n";
    }

    void list_tables() {
        std::cout << "Tables:\n";
        for (auto& [name, tbl] : tables_)
            std::cout << "  " << name << " (" << tbl.columns.size() << " cols, "
                      << tbl.rows.size() << " rows)\n";
    }
};

} // namespace db
