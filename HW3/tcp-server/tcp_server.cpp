#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <socket_wrapper/socket_class.h>
#include <socket_wrapper/socket_headers.h>
#include <socket_wrapper/socket_wrapper.h>

namespace server_state
{
    bool exit = false;
}

const int MAX_BUFFER_SIZE = 1024;

bool send_message(socket_wrapper::Socket& sock, const std::string& message)
{
    ssize_t    bytes_count = 0;
    size_t     req_pos     = 0;
    auto const req_buffer  = &(message.c_str()[0]);
    auto const req_length  = message.length();

    while (true)
    {
        if ((bytes_count =
                 send(sock, req_buffer + req_pos, req_length - req_pos, 0)) < 0)
        {
            if (EINTR == errno)
                continue;
        }
        else
        {
            if (!bytes_count)
                break;

            req_pos += bytes_count;

            if (req_pos >= req_length)
            {
                break;
            }
        }
    }

    return true;
}

bool communicate(socket_wrapper::Socket& sock, std::vector<char> &buffer)
{
    std::mutex mu;
    while (true)
    {
        auto recv_bytes = recv(sock, buffer.data(), buffer.size() - 1, 0);

        mu.lock();
        std::cout << recv_bytes << " was received..." << std::endl;
        mu.unlock();

        if (recv_bytes > 0)
        {
            buffer[recv_bytes] = '\0';

            mu.lock();
            std::cout << "------------\n"
                      << std::string(buffer.begin(),
                                     std::next(buffer.begin(), recv_bytes))
                      << std::endl;
            mu.unlock();
            
            send_message(sock, buffer.data());

            continue;
        }
        else if (-1 == recv_bytes)
        {
            if (EINTR == errno)
                continue;
            if (0 == errno)
                break;
            // std::cerr << errno << ": " <<
            // sock_wrap.get_last_error_string() << std::endl;
            break;
        }

        break;
    }
    return true;
}

int main(int argc, const char* argv[])
{
    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " <port>" << std::endl;
        return EXIT_FAILURE;
    }

    socket_wrapper::SocketWrapper sock_wrap;

    const int port{ std::stoi(argv[1]) };

    socket_wrapper::Socket sock = { AF_INET, SOCK_STREAM, IPPROTO_TCP };

    std::cout << "Starting TCP server on the port " << port << "...\n";

    if (!sock)
    {
        std::cerr << sock_wrap.get_last_error_string() << std::endl;
        return EXIT_FAILURE;
    }

    sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port   = htons(port),
    };

    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) != 0)
    {
        std::cerr << sock_wrap.get_last_error_string() << std::endl;
        // Socket will be closed in the Socket destructor.
        return EXIT_FAILURE;
    }

    const int queue = 5;

    listen(sock, queue);

    std::vector<char> buffer;
    buffer.resize(MAX_BUFFER_SIZE);

    // string to store command
    std::string command_string, success_msg;

    // socket address used to store client address
    struct sockaddr_storage client_address     = { 0 };
    socklen_t               client_address_len = sizeof(client_address);
    ssize_t                 recv_len           = 0;

    std::cout << "Running TCP server...\n" << std::endl;

    // new socket to accept
    socket_wrapper::Socket   newsock = { 0 };
    std::vector<std::thread> clients;

    while (!server_state::exit)
    {
        // accepting client
        newsock = accept(sock,
                         reinterpret_cast<sockaddr*>(&client_address),
                         &client_address_len);

        if (!newsock)
        {
            throw std::runtime_error("ERROR on accept!");
        }
        else
        {
            server_state::exit = communicate(newsock, buffer);
        }
    }

    return EXIT_SUCCESS;
}