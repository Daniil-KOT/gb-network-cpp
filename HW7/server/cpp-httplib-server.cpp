#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"

int main(int argc, char* argv[])
{
    httplib::SSLServer svr("./server.pem", "./key.pem");

    svr.Get("/hi",
            [](const httplib::Request&, httplib::Response& res)
            { res.set_content("Hello World!", "text/plain"); });

    svr.Post("/server/control/exit",
             [&](const httplib::Request&, httplib::Response& res)
             { svr.stop(); });

    svr.listen("0.0.0.0", 8080);

    std::cout << "Server stopped!" << std::endl;
    return EXIT_SUCCESS;
}