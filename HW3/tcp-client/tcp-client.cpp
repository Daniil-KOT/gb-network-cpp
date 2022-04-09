#include <chrono>
#include <exception>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#   define ioctl ioctlsocket
#else
extern "C"
{
#   include <netinet/tcp.h>
#   include <sys/ioctl.h>
// #   include <fcntl.h>
}
#endif

#include <socket_wrapper/socket_headers.h>
#include <socket_wrapper/socket_wrapper.h>
#include <socket_wrapper/socket_class.h>


using namespace std::chrono_literals;

const auto MAX_RECV_BUFFER_SIZE = 256;


bool send_request(socket_wrapper::Socket &sock, const std::string &request)
{
    ssize_t bytes_count = 0;
    size_t req_pos = 0;
    auto const req_buffer = &(request.c_str()[0]);
    auto const req_length = request.length();

    while (true)
    {
        if ((bytes_count = send(sock, req_buffer + req_pos, req_length - req_pos, 0)) < 0)
        {
            if (EINTR == errno) continue;
        }
        else
        {
            if (!bytes_count) break;

            req_pos += bytes_count;

            if (req_pos >= req_length)
            {
                break;
            }
        }
    }

    return true;
}

void recv_answer(socket_wrapper::Socket &sock, std::vector<char> &buffer)
{
    while (true)
    {
        auto recv_bytes = recv(sock, buffer.data(), buffer.size() - 1, 0);

        std::cout
            << recv_bytes
            << " was received..."
            << std::endl;

        if (recv_bytes > 0)
        {
            buffer[recv_bytes] = '\0';
            std::cout << "------------\n" << std::string(buffer.begin(), std::next(buffer.begin(), recv_bytes)) << std::endl;
            continue;
        }
        else if (-1 == recv_bytes)
        {
            if (EINTR == errno) continue;
            if (0 == errno) break;
            // std::cerr << errno << ": " << sock_wrap.get_last_error_string() << std::endl;
            break;
        }

        break;
    }
}

int main(int argc, const char * const argv[])
{
    if (argc != 3)
    {
        std::cout << "Usage: " << argv[0] << " <host> <port>" << std::endl;
        return EXIT_FAILURE;
    }

    socket_wrapper::SocketWrapper sock_wrap;
    socket_wrapper::Socket sock{0};

    const std::string host_name = { argv[1] }, port = { argv[2] };

    addrinfo hints =
    {
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM
    };

    addrinfo* serv_info = nullptr;

    if (getaddrinfo(host_name.c_str(), port.c_str(), &hints, &serv_info) < 0)
    {
        std::cerr << "Cannot resolve address: "
                  << host_name
                  << " and port: "
                  << port
                  << std::endl;

        return EXIT_FAILURE;
    }

    for (auto ai = serv_info; ai != nullptr; ai = ai->ai_next)
    {
        sock = {ai->ai_family, ai->ai_socktype, ai->ai_protocol};

        if (!sock)
        {
            std::cerr << sock_wrap.get_last_error_string() << std::endl;
            return EXIT_FAILURE;
        }

        if (connect(sock, ai->ai_addr, ai->ai_addrlen) != 0)
        {
            std::cerr << sock_wrap.get_last_error_string() << std::endl;
            return EXIT_FAILURE;
        }
        else
        {
            std::cout << "Connected to \"" << host_name << "\"..." << std::endl;
            break;
        }
    }

    std::string request;
    std::vector<char> buffer;
    buffer.resize(MAX_RECV_BUFFER_SIZE);

    const IoctlType flag = 1;

    // Put the socket in non-blocking mode:
    if (ioctl(sock, FIONBIO, const_cast<IoctlType*>(&flag)) < 0)
    {
        std::cerr << sock_wrap.get_last_error_string() << std::endl;
        return EXIT_FAILURE;
    }

    // Disable Naggles's algorithm.
    if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char *>(&flag), sizeof(flag)) < 0)
    {
        std::cerr << sock_wrap.get_last_error_string() << std::endl;
        return EXIT_FAILURE;
    }

    while (true)
    {
        std::cout << "> " << std::flush;
        if (!std::getline(std::cin, request)) break;

        std::cout
            << "Sending request: \"" << request << "\"..."
            << std::endl;

        request += "\r\n";

        if (!send_request(sock, request))
        {
            std::cerr << sock_wrap.get_last_error_string() << std::endl;
            return EXIT_FAILURE;
        }

        std::cout
            << "Request was sent, reading response..."
            << std::endl;

        std::this_thread::sleep_for(2ms);

        recv_answer(sock, buffer);
    }

    return EXIT_SUCCESS;
}

