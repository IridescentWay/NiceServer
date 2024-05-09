#include "nice_server.h"

int main() {
    // 数据库信息
    std::string user = "admin";
    std::string pswd = "7428@0203";
    std::string dbName = "server_db";

    NiceServer server(9006, 1);
    server.server_init(user, pswd, dbName, 8, 8);
    server.server_loop();
    return 0;
}