#include <string>
#include <iostream>
#include <cuchar>
#include <functional>

#include "../include/helper.h"
#include "../include/db.h"
#include "../thirdparty/sqlite/sqlite3.h"


enum class Action {
    rename_file,
    list_file,
    calc_sha256,
    remove_duplicate
};

std::string target_dir;
Action action;
std::string str_toremove;
std::string db_path;

const std::string target_name = "-target";
const std::string str_remove = "-r";
const std::string str_db_path = "-db";

const std::string action_name = "-action";
const std::string action_list = "list";
const std::string action_remove = "rename";
const std::string action_sha256 = "sha256";
const std::string action_duplicate = "duplicate";

bool parseArg(int argc, char* argv[]);
void rename();
void list();
void calSHA256();
void removeDuplicate();

//-target d:\ttt3 -action rename -r 【公众号：书单严选】
//-action list -target d:\ttt3
//-action sha256 -target C:\Users\shaocq\Downloads\LenovoID-Pad-RoW.apk
//-action duplicate -db f:\ebook.db -target d:\tt2

DB db;

int main(int argc, char* argv[])
{
    auto r = parseArg(argc, argv);
    if (!r) {
        std::cout << "Unknown command\n";
        return 0;
    }
    if (!db_path.empty()) {
        db.open_db(db_path);
    }

    switch (action)
    {
    case Action::rename_file:
        rename();
        break;
    case Action::list_file:
        list();
        break;
    case Action::calc_sha256:
        calSHA256();
        break;
    case Action::remove_duplicate:
        removeDuplicate();
        break;
    default:
        std::cout << "Unknown command\n";
        break;
    }

    return 0;
}

void sha256Processor(std::string path) {
    auto r = sha256file(path);
    std::cout << r << std::endl;
    //store in db
}

void calSHA256() {
    if (target_dir.empty()) {
        std::cout << "Bad argument paramters\n";
        return;
    }
    list(target_dir, sha256Processor);
}

void printFilePath(std::string path) {
    std::cout << path << std::endl;
}

void list() {
    if (target_dir.empty()) {
        std::cout << "Bad argument paramters\n";
        return;
    }

    list(target_dir, printFilePath);
}

void rename() {
    if (target_dir.empty() || str_toremove.empty()) {
        std::cout << "Bad argument paramters\n";
        return;
    }
    auto processor = std::bind(renameFile, std::placeholders::_1, str_toremove);
    list(target_dir, processor);
}

void removeProcessor(std::string path) {
    std::cout << "processing file: " << path << std::endl;
    auto r = sha256file(path);
    if (r.empty()) {
        return;
    }
    if (db.is_saved(r)) {
        auto p = db.getPath(r);
        if (p != path) {
            std::cout << "Dupicated file found: " << path << std::endl;
            deleteFile(path);
        }
        else {
            std::cout << "Same file file found: " << path << std::endl;
        }
    }
    else {
        db.insert(path, r);
    }
}

void removeDuplicate() {
    if (target_dir.empty() || db_path.empty()) {
        std::cout << "Bad argument paramters\n";
        return;
    }
    list(target_dir, removeProcessor);
}

bool parseArg(int argc, char* argv[]) {
    for (int i = 1; i + 1 < argc; ) {
        std::string p = argv[i++];
        std::string value = argv[i++];
        std::cout << p << std::endl;
        std::cout << value << std::endl;
        if (p == action_name) {
            if (value == action_remove) {
                action = Action::rename_file;
            }
            else if (value == action_list) {
                action = Action::list_file;
            }
            else if (value == action_sha256) {
                action = Action::calc_sha256;
            }
            else if (value == action_duplicate) {
                action = Action::remove_duplicate;
            }
        }
        else if (p == target_name) {
            target_dir = value;
        }
        else if (p == str_remove) {
            str_toremove = value;
        }
        else if (p == str_db_path) {
            db_path = value;
        }
        else {
            return false;
        }
    }

    return true;
}