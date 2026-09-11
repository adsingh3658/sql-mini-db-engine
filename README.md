# Mini SQL Database Engine — Detailed Explanation

**Field:** SQL / Databases
**Language:** C++17
**One-line pitch:** An in-memory database that parses and executes real SQL
(`CREATE`, `INSERT`, `SELECT` with `WHERE` and `ORDER BY`) — showing the three
layers every database has.

---

## 1. Why this project stands out

Most candidates *use* SQL; few have *built* an engine that runs it. Doing so
proves you understand parsing, in-memory storage, and query execution — and it's
a memorable talking point.

## 2. The three layers (all present here)

1. **Parser** — a tokenizer + hand-written recursive parsing turns SQL text into
   a structured command (which verb, which table, which columns, filters, sort).
2. **Storage** — a `Table` is ordered column names + a vector of rows; the
   database is a map of table name → table.
3. **Executor** — runs the parsed command: projects the requested columns,
   filters rows by `WHERE`, and sorts by `ORDER BY`.

## 3. Supported SQL

```sql
CREATE TABLE users (id, name, age)
INSERT INTO users VALUES (1, Alice, 30)
SELECT * FROM users
SELECT name, age FROM users WHERE age > 28
SELECT * FROM users ORDER BY age DESC
TABLES                         -- list tables (a small extension)
```

Values are stored as strings but compared **numerically** when both operands
look like numbers, so `age > 28` behaves correctly.

## 4. Code structure

```
sql-mini-db-engine/
├── src/
│   ├── database.hpp   # tokenizer + parser + storage + executor
│   └── main.cpp       # scripted demo + interactive SQL REPL
├── build.ps1
└── EXPLANATION.md
```

## 5. Build & run (Windows)

```powershell
cd software\sql-mini-db-engine
./build.ps1
./minidb.exe
```

## 6. Code walkthrough (in order)

Everything is in `src/database.hpp`.

### 6.1 Storage

```cpp
struct Table {
    std::vector<std::string> columns;
    std::vector<std::vector<std::string>> rows;
    int column_index(const std::string& name) const;   // name -> position
};
std::unordered_map<std::string, Table> tables_;         // table name -> table
```

### 6.2 `execute()` — the dispatcher

Tokenizes the SQL and routes on the first keyword:

```cpp
if (verb == "CREATE")      create_table(tokens);
else if (verb == "INSERT") insert(tokens);
else if (verb == "SELECT") select(tokens);
```

Errors are thrown as exceptions and caught here, so bad SQL prints a clean
message instead of crashing.

### 6.3 Tokenizer

`tokenize()` splits words while treating `(`, `)`, and `,` as their own tokens,
which makes parsing the parenthesized lists trivial:

```cpp
if (ch == '(' || ch == ')' || ch == ',') { flush(); out.push_back(std::string(1, ch)); }
```

### 6.4 CREATE / INSERT

`read_paren_list()` collects the comma-separated identifiers inside `( ... )`.
`create_table()` uses it for column names; `insert()` uses it for values and
checks the count matches the table's columns.

### 6.5 SELECT — the interesting one

It parses the requested columns (or `*`), the table, an optional `WHERE col op
val`, and an optional `ORDER BY col [ASC|DESC]`. Filtering applies the comparison:

```cpp
for (const auto& row : tbl.rows)
    if (!has_where || compare(row[w_idx], w_op, w_val)) result.push_back(&row);
```

`compare()` is the type-coercion trick — numeric comparison when both operands
look like numbers, else lexicographic:

```cpp
if (is_number(a) && is_number(b)) { double x=std::stod(a), y=std::stod(b); ... }
```

Sorting uses `compare` too, and `print_result()` prints a header + the projected
columns.

### 6.6 `main.cpp`

Runs a scripted CREATE/INSERT/SELECT demo, then drops into an interactive SQL
REPL.


