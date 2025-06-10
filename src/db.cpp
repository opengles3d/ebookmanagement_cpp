#include "../include/db.h"

#include <iostream>
#include <windows.h>

// 将 UTF - 8 编码的 std::u8string 转换为 std::wstring
std::wstring utf8_to_wstring(const std::u8string & u8str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(u8str.c_str()), -1, nullptr, 0) - 1;//remove '\0'
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(u8str.c_str()), -1, &wstr[0], size_needed);
    return wstr;
}

// 将 std::wstring 转换为 std::string
std::string wstring_to_string(const std::wstring& wstr) {
    int size_needed = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr) - 1;//remove '\0'
    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &str[0], size_needed, nullptr, nullptr);
    return str;
}

// 将 UTF-8 编码的 std::u8string 转换为 std::string
std::string u8string_to_string(const std::u8string& u8str) {
    std::wstring wstr = utf8_to_wstring(u8str);
    return wstring_to_string(wstr);
}

// 将 std::string 转换为 std::wstring
std::wstring string_to_wstring(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &wstr[0], size_needed);
    return wstr;
}

// 将 std::wstring 转换为 UTF-8 编码的 std::u8string
std::u8string wstring_to_u8string(const std::wstring& wstr) {
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], size_needed, nullptr, nullptr);
    return std::u8string(reinterpret_cast<const char8_t*>(str.c_str()));
}

// 将 std::string 转换为 UTF-8 编码的 std::u8string
std::u8string string_to_u8string(const std::string& str) {
    std::wstring wstr = string_to_wstring(str);
    return wstring_to_u8string(wstr);
}

DB::DB() : db{nullptr} {
 
}

DB::~DB() {
    if (db == nullptr) {
        return;
    }
    int rc = sqlite3_close(db);
    if (rc != SQLITE_OK) {
        std::cout << "Closed Database failed!" << std::endl;
    }
}

bool DB::open_db(std::string dbname) {
    if (dbname.empty()) {
        return false;
    }
    int exit = sqlite3_open(dbname.c_str(), &db);
    if (exit) {
        std::cout << "DB Open Error: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    
    // Save SQL to create a table
    std::string create_table_sql = "CREATE TABLE IF NOT EXISTS EBOOK ("  \
        "ID INTEGER PRIMARY KEY AUTOINCREMENT," \
        "NAME           VCHAR(1024)    NOT NULL,"  \
        "HASH           VCHAR(1024)    NOT NULL);";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, create_table_sql.c_str(), -1, &stmt, NULL);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cout << "DB Create Error: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    sqlite3_finalize(stmt);

    std::string get_all_sql = "SELECT * FROM 'EBOOK';";
    sqlite3_stmt* get_all_stmt;
    rc = sqlite3_prepare_v2(db, get_all_sql.c_str(), -1, &get_all_stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cout << "DB Read Error: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    int cols = sqlite3_column_count(get_all_stmt);
    while (sqlite3_step(get_all_stmt) == SQLITE_ROW)
    {
        std::string hash{ (char*)sqlite3_column_text(get_all_stmt, 2) };
        std::u8string path{ (char8_t*)sqlite3_column_text(get_all_stmt, 1) };
        auto str = u8string_to_string(path);
        ebook_list.emplace(hash, str);
    }
    rc = sqlite3_reset(get_all_stmt);
    rc = sqlite3_finalize(get_all_stmt);
    return true;
}

bool DB::insert(std::string name, std::string hash) {
    // Save SQL insert data
    sqlite3_stmt* stmt;
    std::string  sql = "INSERT INTO EBOOK ('NAME', 'HASH') VALUES (?, ?);";
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cout << "sqlite3_prepare_v2 failed!" << std::endl;
        return false;
    }
    auto u8name = string_to_u8string(name);
    sqlite3_bind_text(stmt, 1, (char*)u8name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, hash.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cout << "insert failed!" << std::endl;
        return false;
    }
    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    ebook_list.emplace(hash, name);

    return true;
}

bool DB::insert(std::vector<std::tuple<std::string, std::string>> data) {
    sqlite3_stmt* stmt;
    std::string  sql = "INSERT INTO EBOOK ('NAME', 'HASH') VALUES (?, ?);";
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cout << "sqlite3_prepare_v2 failed!" << std::endl;
        return false;
    }
    if(!execute_sql("BEGIN TRANSACTION;")) {
        return false;
    }
    for (auto& [name, hash] : data) {
        auto u8name = string_to_u8string(name);
        sqlite3_bind_text(stmt, 1, (char*)u8name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, hash.c_str(), -1, SQLITE_TRANSIENT);
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            std::cout << "insert failed!" << std::endl;
            execute_sql("ROLLBACK;");
            sqlite3_finalize(stmt);
            return false;
        }
        sqlite3_reset(stmt); // Reset the statement to be reused
    }

    sqlite3_finalize(stmt);    
    if (!execute_sql("COMMIT;")) {
        return false;
    }
    return true;
}

bool DB::is_saved(std::string hash) {
    return ebook_list.contains(hash);
}

std::string DB::getPath(std::string hash) {
    return ebook_list[hash];
}

bool DB::query_by_hash(const std::string& hash) {
    const char* sql = "SELECT NAME, HASH FROM EBOOK WHERE HASH = ?;";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, hash.c_str(), -1, SQLITE_STATIC);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const unsigned char* name = sqlite3_column_text(stmt, 0);
        const unsigned char* hash = sqlite3_column_text(stmt, 1);
        std::cout << "Name: " << name << ", Hash: " << hash << std::endl;
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "Query failed!" << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool DB::execute_sql(const char* sql) {
    char* errmsg = 0;
    int rc = sqlite3_exec(db, sql, 0, 0, &errmsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errmsg << std::endl;
        sqlite3_free(errmsg);
        return false;
    }
    return true;
}

 
 /*
int callback(void* NotUsed, int argc, char** argv, char** azColName) {
    // Return successful
    return 0;
}


int select_callback(void* NotUsed, int argc, char** argv, char** azColName) {
    for (int i = 0; i < argc; i++) {

        // Show column name, value, and newline
        std::cout << azColName[i] << ": " << argv[i] << std::endl;

    }

    // Insert a newline
    std::cout << std::endl;
    // Return successful
    return 0;
}

int main(int argc, char* argv[]) {
    // Pointer to SQLite connection
    sqlite3* db;

    // Save the connection result
    int exit = 0;
    exit = sqlite3_open("example.db", &db);

    // Test if there was an error
    if (exit) {

        std::cout << "DB Open Error: " << sqlite3_errmsg(db) << std::endl;

    }
    else {

        std::cout << "Opened Database Successfully!" << std::endl;
    }

    // Save SQL to create a table
    std::string create_table_sql = "CREATE TABLE IF NOT EXISTS EBOOK ("  \
        "ID INTEGER PRIMARY KEY AUTOINCREMENT," \
        "NAME           TEXT    NOT NULL,"  \
        "HASH           TEXT    NOT NULL);";

    // Run the SQL (convert the string to a C-String with c_str() )
    //char* zErrMsg = 0;
    //exit = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, create_table_sql.c_str(), -1, &stmt, NULL);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    // Save SQL insert data
    //std::string  sql = "INSERT INTO EBOOK ('NAME', 'HASH') VALUES ('Jeff', '123456');";

    // Run the SQL (convert the string to a C-String with c_str() )
    char* zErrMsg = 0;
    //exit = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);
    sqlite3_stmt* stmt2;
    std::string  sql2 = "INSERT INTO EBOOK ('NAME', 'HASH') VALUES (?, ?);";
    rc = sqlite3_prepare_v2(db, sql2.c_str(), -1, &stmt2, NULL);
    sqlite3_bind_text(stmt2, 1, "Jerry", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt2, 2, "8034934", -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt2);
    sqlite3_clear_bindings(stmt2);
    sqlite3_reset(stmt2);
    sqlite3_finalize(stmt2);

    // Save SQL insert data
    //std::string sql = "SELECT * FROM 'EBOOK';";

    // Run the SQL (convert the string to a C-String with c_str() )
    //exit = sqlite3_exec(db, sql.c_str(), select_callback, 0, &zErrMsg);

    std::string sql3 = "SELECT * FROM 'EBOOK';";
    sqlite3_stmt* stmt3;
    rc = sqlite3_prepare_v2(db, sql3.c_str(), -1, &stmt3, NULL);
    //sqlite3_step(stmt3);
    int cols = sqlite3_column_count(stmt3);
    while (sqlite3_step(stmt3) == SQLITE_ROW)
    {
        for (int i = 0; i < cols; i++) {
            std::cout << std::string((char*)sqlite3_column_text(stmt3, i)) << std::endl;
        }
    }
    rc = sqlite3_reset(stmt3);
    rc = sqlite3_finalize(stmt3);
    // Close the connection
    rc = sqlite3_close(db);

    return (0);
}
*/