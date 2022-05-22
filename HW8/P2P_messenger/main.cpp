#include <iostream>
#include <boost/asio.hpp>
#include "messenger.h"

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

    boost::asio::post(thread_pool, [&]
    {
        peer.do_receive();
        io_context.run();
    });

    boost::asio::post(thread_pool, [&]
    { 
        peer.do_send();
        io_context.run();
    });

    thread_pool.join();

    return EXIT_SUCCESS;
}