#include "httpserver.hpp"
#include <iostream>
using namespace std;

int main() {
    myHttpServer::HttpServer server(8522);
    server.ServerRun();
    return 0;
}
