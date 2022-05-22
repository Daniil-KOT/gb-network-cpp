#include <iostream>
#include <string>
#include <array>
#include <random>

#include <boost/asio.hpp>

constexpr uint16_t multicast_port = 30001;

class Peer {
public:
    Peer(boost::asio::io_context& io_context,
         const boost::asio::ip::address& chat_room,
         const std::string& name);

    void do_receive();

    void do_send();

    void send_info();

private:
    size_t maximum_message_size_ = 128;
    std::array<char, 128> receiving_buffer_;
    std::string name_;
    uint16_t uid_;
    boost::asio::ip::udp::socket socket_;
    boost::asio::ip::udp::endpoint remote_endpoint_; 
    boost::asio::ip::udp::endpoint multicast_endpoint_;
};