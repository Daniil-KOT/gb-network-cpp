#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>


extern "C"
{
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>

}

const int MAX_BUFFER_SIZE = 4096;

void init_ssl()
{
    SSL_load_error_strings();
    SSL_library_init();
}

void cleanup(BIO* bio, SSL_CTX* ctx)
{
    BIO_free_all(bio);
    SSL_CTX_free(ctx);
}

SSL_CTX* create_context()
{
    const SSL_METHOD* method;
    SSL_CTX*          ctx;

    method = TLS_client_method();

    if (!method)
    {
        std::cerr << "Unable to create method!" << std::endl;
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    ctx = SSL_CTX_new(method);

    if (!ctx)
    {
        std::cerr << "Unable to create SSL context!" << std::endl;
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}

bool calculate_hash(std::string& passwd)
{
    EVP_MD_CTX* evp_ctx = EVP_MD_CTX_new();

    if (!evp_ctx)
    {
        std::cerr << "Unable to create EVP context!" << std::endl;
        ERR_print_errors_fp(stderr);
        EVP_MD_CTX_free(evp_ctx);
        return false;
    }

    if (!EVP_DigestInit_ex(evp_ctx, EVP_sha512(), nullptr))
    {
        std::cerr << "Unable to init EVP digest!" << std::endl;
        ERR_print_errors_fp(stderr);
        EVP_MD_CTX_free(evp_ctx);
        return false;
    }

    if (!EVP_DigestUpdate(evp_ctx, passwd.c_str(), passwd.length()))
    {
        std::cerr << "Unable to update EVP digest!" << std::endl;
        ERR_print_errors_fp(stderr);
        EVP_MD_CTX_free(evp_ctx);
        return false;
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int  hash_length = 0;

    if (!EVP_DigestFinal_ex(evp_ctx, hash, &hash_length))
    {
        std::cerr << "Unable to calculate hash!" << std::endl;
        ERR_print_errors_fp(stderr);
        EVP_MD_CTX_free(evp_ctx);
        return false;
    }

    std::stringstream ss;

    for (int i = 0; i < hash_length; ++i)
    {
        ss << std::hex << (int)hash[i];
    }

    passwd = ss.str();

    EVP_MD_CTX_free(evp_ctx);

    return true;
}

bool authorize(BIO* bio, std::string& login, std::string& passwd)
{
    std::string token, response;
    token.resize(256);
    response.resize(256);

    auto recv_bytes = BIO_read(bio, &token[0], token.size());

    if (recv_bytes > 0)
    {
        passwd += token;
    }
    else
    {
        std::cerr << "Could not get token!" << std::endl;
        return false;
    }

    if (!calculate_hash(passwd))
    {
        return false;
    }

    login += " " + passwd;
    BIO_puts(bio, login.c_str());

    recv_bytes = BIO_read(bio, &response[0], response.size());

    std::cout << response << std::endl;

    if (recv_bytes > 0)
    {
        if (response.find("Authorized") != std::string::npos)
            return true;
    }

    return false;
}

std::pair<BIO*, SSL*> secure_connect(const std::string hostname, SSL_CTX* ctx)
{
    std::string host, port;
    auto        port_start = hostname.find(':');

    if (port_start == std::string::npos)
    {
        host = hostname;
        port = "443";
    }
    else
    {
        host = hostname.substr(0, port_start);
        port = hostname.substr(port_start + 1);
    }

    BIO* bio = BIO_new_ssl_connect(ctx);

    if (!bio)
    {
        std::cerr << "Unable to create BIO!" << std::endl;
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    SSL* ssl = nullptr;

    BIO_get_ssl(bio, &ssl);

    SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

    std::cout << "Connecting to:\t" << host << std::endl;

    if (SSL_set_tlsext_host_name(ssl, host.c_str()) != 1)
    {
        cleanup(bio, ctx);
        std::cerr << "Unable to set hostname!" << std::endl;
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    BIO_set_conn_hostname(bio, host.c_str());
    BIO_set_conn_port(bio, port.c_str());

    if (!SSL_CTX_load_verify_locations(
            ctx, "/etc/ssl/certs/ca-certificates.crt", "/etc/ssl/certs/"))
    {
        std::cerr << "Unable to load certificate!" << std::endl;
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (BIO_do_connect(bio) <= 0)
    {
        cleanup(bio, ctx);
        std::cerr << "Unable to connect!" << std::endl;
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    long verify_flag = SSL_get_verify_result(ssl);
    switch (verify_flag)
    {
        // Verification error handling by code example.
        case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
        case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
            std::cerr << "##### Certificate verification error: self-signed ("
                      << X509_verify_cert_error_string(verify_flag) << ", "
                      << verify_flag << ") but continuing..." << std::endl;
            break;
        case X509_V_OK:
            std::cout << "##### Certificate verification passed..."
                      << std::endl;
            break;
        default:
            std::cerr << "##### Certificate verification error ("
                      << X509_verify_cert_error_string(verify_flag) << ", "
                      << verify_flag << ") but continuing..." << std::endl;
    }

    return { bio, ssl };
}

std::string create_get_request(const std::string& host, const std::string& page)
{
    std::stringstream ss;

    ss << "GET " + page + " HTTP/1.1\r\n"
       << "Host: " + host + "\r\n"
       << "Accept-Language: en-en\r\n"
       << "Accept-Encoding: gzip, deflate\r\n"
       << "Connection: Keep-Alive\r\n\r\n";

    return ss.str();
}

bool send_request(BIO*               bio,
                  const std::string  hostname,
                  const std::string& request)
{
    if (request.find("get") != std::string::npos)
    {
        std::string page     = "";
        auto        page_pos = request.find('/');

        if (page_pos != std::string::npos)
        {
            page = request.substr(page_pos);
        }

        auto port_start = hostname.find(':');
        auto host       = (port_start == std::string::npos)
                              ? hostname
                              : hostname.substr(0, port_start);

        BIO_puts(bio, create_get_request(host, page).c_str());

        return true;
    }

    return false;
}

void recv_answer(BIO* bio, std::vector<char>& buffer)
{
    size_t recv_bytes;
    do
    {
        BIO_read_ex(bio, buffer.data(), buffer.size() - 1, &recv_bytes);

        std::cout << recv_bytes << " was received..." << std::endl;

        if (recv_bytes > 0)
        {
            buffer[recv_bytes] = '\0';
            std::cout << "------------\n"
                      << std::string(buffer.begin(),
                                     std::next(buffer.begin(), recv_bytes))
                      << std::endl;
        }
        else if (-1 == recv_bytes)
        {
            if (EINTR == errno)
                continue;
            if (0 == errno)
                break;
            break;
        }

    } while (recv_bytes > 0 || BIO_should_retry(bio));
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " <host>\n\n";
        return EXIT_FAILURE;
    }

    std::string host = argv[1];

    SSL_CTX* ctx     = create_context();
    auto     bio_ssl = secure_connect(host, ctx);

    std::vector<char> buffer;
    buffer.resize(MAX_BUFFER_SIZE);

    std::string request, login, passwd;

    if (host.find("localhost") != std::string::npos ||
        host.find("127.0.0.1") != std::string::npos)
    {
        bool authorized = false;

        while (!authorized)
        {
            std::cout << "Login: " << std::flush;
            if (!std::getline(std::cin, login))
                break;

            std::cout << "Password: " << std::flush;
            if (!std::getline(std::cin, passwd))
                break;

            authorized = authorize(bio_ssl.first, login, passwd);
        }
    }

    while (true)
    {
        std::cout << "> " << std::flush;
        if (!std::getline(std::cin, request))
            break;

        if (!send_request(bio_ssl.first, argv[1], request))
        {
            std::cout << "Wrong request!\n\n"
                      << "Example: get /homepage" << std::endl;
            continue;
        }

        std::cout << "Request was sent, reading response..." << std::endl;

        recv_answer(bio_ssl.first, buffer);

        std::cout << "FlAG" << std::endl;
    }

    return EXIT_SUCCESS;
}