#include <algorithm>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/system/error_code.hpp>
#include <boost/thread.hpp>
#include <cerrno>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#if !defined(_WIN32)
extern "C"
{
#include <signal.h>
}
#else
#include <cwctype>
#endif

namespace fs = std::filesystem;
using boost::asio::ip::tcp;

#if defined(_WIN32)
const wchar_t separ = fs::path::preferred_separator;
#endif

class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
    typedef std::shared_ptr<TcpConnection> pointer;

    static pointer create(boost::asio::io_context& io_context)
    {
        return pointer(new TcpConnection(io_context));
    }

    tcp::socket& socket() { return socket_; }

    void start() { read(); }

private:
    TcpConnection(boost::asio::io_context& io_context)
        : socket_(io_context)
    {
    }

    fs::path get_filename(const std::string& message)
    {
        auto cur_path  = fs::current_path().wstring();
        auto file_path = fs::weakly_canonical(message).wstring();

#if defined(_WIN32)
        std::transform(cur_path.begin(),
                       cur_path.end(),
                       cur_path.begin(),
                       [](wchar_t c) { return std::towlower(c); });
        std::transform(file_path.begin(),
                       file_path.end(),
                       file_path.begin(),
                       [](wchar_t c) { return std::towlower(c); });
#endif

        if (file_path.find(cur_path) == 0)
        {
            file_path = file_path.substr(cur_path.length());
        }

        return fs::weakly_canonical(cur_path + separ + file_path);
    }

    std::vector<char> read_file_bytes(const fs::path& file_path)
    {
        std::ifstream     file(file_path, std::ifstream::binary);
        std::vector<char> tmp(0);
        if (file)
        {
            file.seekg(0, std::ios::end);
            size_t len = file.tellg();
            tmp.resize(len);
            file.seekg(0, std::ios::beg);

            if (len)
            {
                file.read(&tmp[0], len);
            }
        }
        return std::move(tmp);
    }

    void send_file(std::vector<char>& content, int& offset)
    {
        socket_.async_write_some(
            boost::asio::buffer(content.data() + offset, chunk_ - 1),
            boost::bind(&TcpConnection::handle_send_file,
                        shared_from_this(),
                        content,
                        offset,
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
    }

    void handle_send_file(std::vector<char>& content,
                          int&               offset,
                          const boost::system::error_code&,
                          size_t bytes_transferred)
    {
        if (offset + bytes_transferred < content.size())
        {
            std::cout << "Sending file...\n"
                      << "Bytes transferred:\t"
                      << bytes_transferred 
                      << std:: endl;

            if (chunk_ > content.size() - offset)
            {
                chunk_ = content.size() - offset;
                std::cout << "\nChanging chunk size to:\t"
                          << chunk_ 
                          << "\nContinue sending...\n";
            }
            else
            {
                offset += bytes_transferred;
            }

            send_file(content, offset);
        }
        else
        {
            std::cout << "File was successfully sent!\n\n";
            chunk_ = 4096;
            offset = 0;
            read();
        }
    }

    void read()
    {
        socket_.async_read_some(
            boost::asio::buffer(message_, 4096),
            boost::bind(&TcpConnection::handle_read,
                        shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
    }

    void write(std::string& message)
    {
        auto content = read_file_bytes(get_filename(message));
        int  offset  = 0;

        if (content.size() > 0)
        {
            send_file(content, offset);
        }
        else
        {
            message += "\nFile not found!";

            socket_.async_write_some(
                boost::asio::buffer(message),
                boost::bind(&TcpConnection::handle_write,
                            shared_from_this(),
                            message,
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
        }
    }

    void handle_write(std::string& content,
                      const boost::system::error_code&,
                      size_t bytes_transferred)
    {
        std::cout << "Bytes transferred: " << bytes_transferred
                  << "\nContent: " << content
                  << "\nUser: " << boost::this_thread::get_id() << "\n\n";
        read();
    }

    void handle_read(const boost::system::error_code& error, size_t bytes_read)
    {
        if (!error)
        {
            std::string response(message_, bytes_read);
            write(response);
        }
        else
        {
            std::cerr << "Error: " << error.what() << std::endl;
        }
    }

private:
    tcp::socket  socket_;
    size_t chunk_ = 4096;
    char         message_[4096];
    char         response_buf_[4096];
};

class TcpServer
{
public:
    TcpServer(boost::asio::io_context& io_context, int port)
        : io_context_(io_context)
        , acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
    {
        start_accept();
    }

private:
    void start_accept()
    {
        TcpConnection::pointer new_connection =
            TcpConnection::create(io_context_);

        acceptor_.async_accept(new_connection->socket(),
                               boost::bind(&TcpServer::handle_accept,
                                           this,
                                           new_connection,
                                           boost::asio::placeholders::error));
    }

    void handle_accept(TcpConnection::pointer           new_connection,
                       const boost::system::error_code& error)
    {
        if (!error)
        {
            new_connection->start();
        }
        start_accept();
    }

private:
    boost::asio::io_context& io_context_;
    tcp::acceptor            acceptor_;
};

void start_listen(int                      thread_count,
                  boost::thread_group&     clients,
                  boost::asio::io_context& io_context)
{
    for (int i = 0; i < thread_count; ++i)
    {
        clients.create_thread([&io_context]() { io_context.run(); });
    }
}

int main(int argc, const char* argv[])
{
    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " <port>" << std::endl;
        return EXIT_FAILURE;
    }

    try
    {
        boost::thread_group     clients;
        boost::asio::io_context io_context;
        TcpServer               server(io_context, std::stoi(argv[1]));

        start_listen(5, clients, io_context);
        clients.join_all();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}