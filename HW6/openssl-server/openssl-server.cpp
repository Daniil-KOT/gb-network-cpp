#include <fstream>
#include <iostream>
#include <string>
#include <vector>


#include <socket_wrapper/socket_class.h>
#include <socket_wrapper/socket_headers.h>
#include <socket_wrapper/socket_wrapper.h>

extern "C"
{
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
}

const int MAX_BUFFER_SIZE = 4096;

void send_message(SSL* ssl, const std::string& message)
{
    ssize_t    bytes_count = 0;
    size_t     req_pos     = 0;
    auto const req_buffer  = &(message.c_str()[0]);
    auto const req_length  = message.length();

    while (true)
    {
        if ((bytes_count = SSL_write(ssl, message.c_str(), message.size())) < 0)
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
}

void communicate(SSL* ssl, std::vector<char>& buffer)
{
    while (true)
    {
        auto recv_bytes = SSL_read(ssl, buffer.data(), buffer.size());

        std::cout << recv_bytes << " was received..." << std::endl;

        if (recv_bytes > 0)
        {
            buffer[recv_bytes] = '\0';
            send_message(ssl, buffer.data());

            continue;
        }
        else if (-1 == recv_bytes)
        {
            if (EINTR == errno)
                continue;
            if (0 == errno)
                break;

            break;
        }

        break;
    }
}

bool register_client(std::string data)
{
    std::ofstream out("client_data.txt", std::ios::app);

    if (!out.is_open())
    {
        std::cerr << "Cannot open client data file!" << std::endl;
        return false;
    }

    out.write(data.c_str(), data.length());
    out << std::endl;

    out.close();
    return true;
}

bool find_client_data(std::string data)
{
    std::ifstream client_data("client_data.txt");

    if (!client_data.is_open())
    {
        std::cerr << "Cannot open client data file!" << std::endl;
        return false;
    }

    std::string data_read;

    auto pass_pos = data.find(" ");
    if (pass_pos != std::string::npos)
    {
        auto login     = data.substr(0, pass_pos);
        bool has_login = false;

        while (std::getline(client_data, data_read))
        {
            if (data_read == data)
            {
                client_data.close();
                return true;
            }

            if (data_read.find(login) != std::string::npos)
                has_login = true;
        }

        if (!has_login)
        {
            client_data.close();
            std::cout << "Adding new client: " << login << std::endl;
            return register_client(data);
        }
    }

    client_data.close();
    return false;
}

bool authorize(SSL* ssl)
{
    const std::string token = "1eF093Plsx76AJIDjfzlI28931xllJ";
    SSL_write(ssl, token.c_str(), token.length());

    std::string data;
    data.resize(MAX_BUFFER_SIZE);
    auto recv_bytes = SSL_read(ssl, &data[0], data.size());

    if (recv_bytes > 0)
    {
        if (find_client_data(data))
        {
            std::string accept = "Authorized";
            SSL_write(ssl, accept.c_str(), accept.length());
            return true;
        }
        else
        {
            std::string decline = "Wrong login or password";
            SSL_write(ssl, decline.c_str(), decline.length());
        }
    }

    return false;
}

socket_wrapper::Socket create_socket(int port)
{
    socket_wrapper::Socket sock(AF_INET, SOCK_STREAM, 0);

    if (!sock)
    {
        std::cerr << "Unable to create socket!" << std::endl;
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;

    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
    {
        std::cerr << "Unable to bind!" << std::endl;
        exit(EXIT_FAILURE);
    }

    if (listen(sock, 5) < 0)
    {
        std::cerr << "Unable to listen!" << std::endl;
        exit(EXIT_FAILURE);
    }

    return sock;
}

SSL_CTX* create_context()
{
    const SSL_METHOD* method;
    SSL_CTX*          ctx;

    method = TLS_server_method();

    ctx = SSL_CTX_new(method);

    if (!ctx)
    {
        std::cerr << "Unable to create SSL context!" << std::endl;
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}

void configure_context(SSL_CTX* ctx)
{
    if (SSL_CTX_use_certificate_file(ctx, "server.pem", SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, "key.pem", SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}

void cleanup(SSL* ssl, SSL_CTX* ctx)
{
    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " <port>\n";
        return EXIT_FAILURE;
    }

    socket_wrapper::SocketWrapper sock_wrap;
    socket_wrapper::Socket        sock = create_socket(std::stoi(argv[1]));

    SSL_CTX* ctx = create_context();
    configure_context(ctx);

    std::vector<char> buffer;
    buffer.resize(MAX_BUFFER_SIZE);

    // socket address used to store client address
    struct sockaddr_storage client_address     = { 0 };
    socklen_t               client_address_len = sizeof(client_address);
    ssize_t                 recv_len           = 0;

    std::cout << "Running TCP server...\n" << std::endl;

    // new socket to accept
    socket_wrapper::Socket newsock = { 0 };
    SSL*                   ssl;
    bool                   authorized = false;

    while (true)
    {
        // accepting client
        newsock = accept(sock,
                         reinterpret_cast<sockaddr*>(&client_address),
                         &client_address_len);

        authorized = false;

        if (!newsock)
        {
            std::cerr << "Error on accept!" << std::endl;
        }
        else
        {
            ssl = SSL_new(ctx);
            SSL_set_fd(ssl, newsock);

            if (SSL_accept(ssl) <= 0)
            {
                ERR_print_errors_fp(stderr);
                break;
            }
            else
            {
                while (!authorized)
                {
                    authorized = authorize(ssl);
                }

                communicate(ssl, buffer);
            }
        }
    }

    cleanup(ssl, ctx);

    return EXIT_SUCCESS;
}