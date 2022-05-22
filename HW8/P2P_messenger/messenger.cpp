#include <iostream>
#include <string>

#include <boost/asio.hpp>

constexpr std::uint16_t multicast_port = 30001;

class Peer {
public:
    Peer(boost::asio::io_context& io_context,
         const boost::asio::ip::address& chat_room,
         const std::string& name)
        : socket_(io_context)
        , multicast_endpoint_(chat_room, multicast_port)
        , name_(name)
    {

        boost::asio::ip::udp::endpoint listen_endpoint(chat_room, multicast_port);
        socket_.open(listen_endpoint.protocol());
        socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));
        socket_.bind(listen_endpoint);

        socket_.set_option(boost::asio::ip::multicast::join_group(chat_room));

        auto welcome_message = std::string(name_ + " connected to the chat\n");

        socket_.async_send_to(boost::asio::buffer(welcome_message), multicast_endpoint_,
                              [this](const boost::system::error_code& error_code, std::size_t bytes_sent){
            if (!error_code){
                std::cout << "Entered chat room successfully" << std::endl;
            }
        });
    }

    void do_receive(){
        socket_.async_receive_from(boost::asio::buffer(receiving_buffer_), remote_endpoint_,
                              [this](const boost::system::error_code& error_code, std::size_t bytes_received){
            if (!error_code.failed() && bytes_received > 0){
                auto received_message_string = std::string(receiving_buffer_.begin(), receiving_buffer_.begin() + bytes_received);
                if (received_message_string.find(name_) != 0){
                    std::cout.write(receiving_buffer_.data(), bytes_received);
                    std::cout << std::flush;
                }
                do_receive();
            }
        });
    }

    void do_send(){
        std::string name = name_;
        std::string message;
        std::getline(std::cin, message);
        std::string buffer = name.append(": " + message);
        socket_.async_send_to(boost::asio::buffer(buffer, maximum_message_size_), multicast_endpoint_,
                           [this, message](const boost::system::error_code& /*error_code*/, std::size_t bytes_sent){
            std::cout << "You: " << message << std::endl;
            do_send();
        });
    }

private:
    std::size_t maximum_message_size_ = 128;
    std::array<char, 128> receiving_buffer_;
    std::string name_;
    boost::asio::ip::udp::socket socket_;
    boost::asio::ip::udp::endpoint remote_endpoint_;
    boost::asio::ip::udp::endpoint multicast_endpoint_;
};


int main(int argc, char* argv[])
{
    boost::asio::thread_pool thread_pool(2);

    if(argc != 3){
        std::cerr << "Usage: ./peer <your_nickname> <multicast_address>" << std::endl;
        std::exit(1);
    }

    boost::asio::io_context io_context;
    boost::asio::ip::address chat_room(boost::asio::ip::make_address(argv[2]));
    Peer peer(io_context, chat_room, argv[1]);

    boost::asio::post(thread_pool, [&]{
        peer.do_receive();
        io_context.run();
    });
    boost::asio::post(thread_pool, [&]{
        peer.do_send();
        io_context.run();
    });
    thread_pool.join();

    return 0;
}