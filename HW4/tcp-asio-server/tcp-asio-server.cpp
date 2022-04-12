#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/system/error_code.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

using boost::asio::ip::tcp;

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

    void read()
    {
        socket_.async_read_some(
            boost::asio::buffer(message_, 1024),
            boost::bind(&TcpConnection::handle_read,
                        shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
    }

    void write(std::string message)
    {
        std::copy(message.begin(), message.end(), response_buf_);
        socket_.async_write_some(
            boost::asio::buffer(response_buf_, message.size()),
            boost::bind(&TcpConnection::handle_write,
                        shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
    }

    void handle_write(const boost::system::error_code&,
                      size_t bytes_transferred)
    {
        std::cout << "Bytes transferred: " << bytes_transferred << std::endl;
        read();
    }

    void handle_read(const boost::system::error_code& error, size_t bytes_read)
    {
        if (!error)
        {
            std::string response(message_, bytes_read);
            write(response + "\n");
        }
        else
        {
            std::cerr << "Error: " << error.what() << std::endl;
        }
    }

private:
    tcp::socket socket_;
    char        message_[1024];
    char        response_buf_[1024];
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

        acceptor_.async_accept(
            new_connection->socket(),
            [this, new_connection](const boost::system::error_code& error)
            { this->handle_accept(new_connection, error); });
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

int main(int argc, const char* argv[])
{
    try
    {
        boost::asio::io_context io_context;
        TcpServer               server(io_context, std::stoi(argv[1]));

        io_context.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
}