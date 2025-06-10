#pragma once

#include <string>
#include <vector>
#include <map>
#include <tuple>

#include "../thirdparty/sqlite/sqlite3.h"

class DB {
public:
	DB();
	~DB();
	bool open_db(std::string dbname);
	bool insert(std::string name, std::string hash);
	bool insert(std::vector<std::tuple<std::string, std::string>> data);
	bool is_saved(std::string hash);
	std::string getPath(std::string hash);
	bool query_by_hash(const std::string& hash);
private:
 	bool execute_sql(const char* sql);
	std::map<std::string, std::string> ebook_list;
	sqlite3* db;
};