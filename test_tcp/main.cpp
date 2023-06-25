#include <iostream>
#include <sstream>
#include "TCPServer.h"


int main() {
    KHAS::TCPServer server{ "127.0.0.1", "8000" };

    server.redirectUrl("/", "templates/index.html");
    server.redirectUrl("/index", "templates/index.html");

    server.run();

    return 0;
}