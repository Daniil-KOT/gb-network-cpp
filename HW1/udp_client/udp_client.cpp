#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

#include <socket_wrapper/socket_class.h>
#include <socket_wrapper/socket_headers.h>
#include <socket_wrapper/socket_wrapper.h>


int main(int argc, char const* argv[])
{
    const size_t BUFSIZE = 256;

    if (argc != 3)
    {
        std::cout << "Usage: " << argv[0] << " <ip> <port>\n";
        return EXIT_FAILURE;
    }

    socket_wrapper::SocketWrapper sock_wrap;
    const int                     port = std::stoi(argv[2]);

    // creating socket
    socket_wrapper::Socket sock(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    std::cout << "Starting UDP client on the port " << port << "...\n";

    if (!sock)
    {
        std::cerr << sock_wrap.get_last_error_string() << std::endl;
        return EXIT_FAILURE;
    }

    /*
        two message arrays are needed
        to correctly process commands
        (":exit" command in this case)

        it is not useful in this example,
        but if we imagine that the server
        sends an answer different from the client's message,
        then the single array will be overwritten
        and the command won't be processed correctly
    */
    char message_sent[BUFSIZE]     = "";
    char message_received[BUFSIZE] = "";
    bool exit                      = false;

    // setting up server address info
    struct sockaddr_in server_address = {
        .sin_family = AF_INET,
        .sin_port   = htons(port),
    };
    server_address.sin_addr.S_un.S_addr = inet_addr(argv[1]);
    socklen_t server_address_len        = sizeof(sockaddr_in);
    ssize_t   recv_len                  = 0;

    std::cout << "Running UDP client...\n\n";

    while (!exit)
    {
        std::cout << "$> ";
        std::cin.getline(message_sent, BUFSIZE);

        sendto(sock,
               message_sent,
               strlen(message_sent),
               0,
               reinterpret_cast<const sockaddr*>(&server_address),
               server_address_len);

        recv_len = recvfrom(sock,
                            message_received,
                            sizeof(message_received) - 1,
                            0,
                            reinterpret_cast<sockaddr*>(&server_address),
                            &server_address_len);

        if (recv_len > 0)
        {
            std::cout << message_received << std::endl;
        }

        if (message_sent[0] == ':')
        {
            std::string command = message_sent;
            if (command == ":exit")
                exit = true;
        }

        std::cout << std::endl;
    }
    return EXIT_SUCCESS;
}