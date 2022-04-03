#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <process.h>
#ifndef getpid
#define getpid _getpid
#endif
#else
#include <unistd.h>
#endif

#include <socket_wrapper/socket_class.h>
#include <socket_wrapper/socket_headers.h>
#include <socket_wrapper/socket_wrapper.h>

using namespace std::chrono_literals;
using namespace std::chrono;

// Define the Packet Constants
// ping packet size
const size_t ping_packet_size = 64;
const auto   ping_sleep_rate  = 1000000us;
const size_t MAX_PACKET_SIZE  = 1024;

// Gives the timeout delay for receiving packets.
const auto recv_timeout = 1s;

// Echo Request.
const int ICMP_ECHO = 8;

// Echo Response.
const int ICMP_ECHO_REPLY = 0;

#pragma pack(push, 1)
// Windows doesn't have this structure.
struct icmphdr
{
    uint8_t  type; /* message type */
    uint8_t  code; /* type sub-code */
    uint16_t checksum;
    union
    {
        struct
        {
            uint16_t id;
            uint16_t sequence;
        } echo;           /* echo datagram */
        uint32_t gateway; /* gateway address */
        struct
        {
            uint16_t __unused;
            uint16_t mtu;
        } frag; /* path mtu discovery */
    } un;
};

//
// IPv4 Header (without any IP options)
//
typedef struct ip_hdr
{
    unsigned char ip_verlen;       // 4-bit IPv4 version
                                   // 4-bit header length (in 32-bit words)
    unsigned char  ip_tos;         // IP type of service
    unsigned short ip_totallength; // Total length
    unsigned short ip_id;          // Unique identifier
    unsigned short ip_offset;      // Fragment offset field
    unsigned char  ip_ttl;         // Time to live
    unsigned char  ip_protocol;    // Protocol(TCP,UDP etc)
    unsigned short ip_checksum;    // IP checksum
    unsigned int   ip_srcaddr;     // Source address
    unsigned int   ip_destaddr;    // Source address
} IPV4_HDR, *PIPV4_HDR;
#pragma pack(pop)

void CreatePacket(char* icmp_data, int datasize, uint16_t sequence)
{
    icmphdr* header   = nullptr;
    char*    datapart = nullptr;

    header                   = reinterpret_cast<icmphdr*>(icmp_data);
    header->type             = ICMP_ECHO;
    header->code             = 0;
    header->checksum         = 0;
    header->un.echo.id       = htons(getpid());
    header->un.echo.sequence = sequence;

    datapart = icmp_data + sizeof(icmphdr);
    memset(datapart, 'a', datasize - sizeof(icmphdr));
}

void DecodePacket(char*                                 buf,
                  std::chrono::steady_clock::time_point start_time,
                  sockaddr_in*                          from)
{
    ip_hdr*        ip_header   = reinterpret_cast<ip_hdr*>(buf);
    icmphdr*       icmp_header = nullptr;
    unsigned short ip_hdr_len  = ip_header->ip_verlen & 0x0f * sizeof(uint32_t);
    static int     icmpcount   = 0;
    const int      ICMP_MIN    = 8;

    icmp_header = reinterpret_cast<icmphdr*>(buf + ip_hdr_len);

    auto end_time = std::chrono::steady_clock::now();

    if ((icmp_header->type == ICMP_ECHO_REPLY) && (icmp_header->code == 0))
    {
        std::cout << "Receiving packet from " << inet_ntoa(from->sin_addr)
                  << ":\nICMP sequence = " << icmp_header->un.echo.sequence
                  << "\nResponse with id = " << icmp_header->un.echo.id
                  << "\nTime: "
                  << std::round(std::chrono::duration_cast<
                                std::chrono::duration<double, std::milli>>(end_time - start_time).count() * 10) / 10
                  << " ms\n\n";
    }
    icmpcount++;
}

uint16_t Checksum(char* buffer)
{
    const uint16_t* buf = reinterpret_cast<const uint16_t*>(buffer);
    size_t          len = sizeof(buffer);
    uint32_t        sum = 0;

    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;
    if (1 == len)
        sum += *reinterpret_cast<const uint8_t*>(buf);
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);

    uint16_t result = sum;

    return ~result;
}

int main(int argc, const char* argv[])
{

    if (argc != 3)
    {
        std::cout << "Usage: " << argv[0] << " <number of pings> <host-name>\n";
        return EXIT_FAILURE;
    }

    socket_wrapper::SocketWrapper sock_wrap;
    std::string                   hostname  = { argv[2] };
    const struct hostent*         dest_addr = gethostbyname(hostname.c_str());

    if (dest_addr == nullptr)
    {
        if (sock_wrap.get_last_error_code())
        {
            std::cout << "DEST_ADDR\n";
            std::cerr << sock_wrap.get_last_error_string() << std::endl;
        }

        return EXIT_FAILURE;
    }

    socket_wrapper::Socket sock(AF_INET, SOCK_RAW, IPPROTO_ICMP);

    if (!sock)
    {
        std::cerr << sock_wrap.get_last_error_string() << std::endl;
        return EXIT_FAILURE;
    }

    sockaddr_in addr = 
    { .sin_family = static_cast<short unsigned>(dest_addr->h_addrtype),
      // Automatic port number.
      .sin_port = htons(0) };

    addr.sin_addr.s_addr =
        *reinterpret_cast<const in_addr_t*>(dest_addr->h_addr);

    sockaddr_in recv_addr;

    std::cout << "Pinging \"" << dest_addr->h_name << "\" ["
              << inet_ntoa(addr.sin_addr) << "]" << std::endl;

    int ttl = 255;
    if (setsockopt(sock,
                   IPPROTO_IP,
                   IP_TTL,
                   reinterpret_cast<const char*>(&ttl),
                   sizeof(ttl)) != 0)
    {
        throw std::runtime_error("TTL setting failed!");
    }

    auto tv = duration_cast<milliseconds>(recv_timeout).count();
    if (setsockopt(sock,
                   SOL_SOCKET,
                   SO_RCVTIMEO,
                   reinterpret_cast<const char*>(&tv),
                   sizeof(tv)) != 0)
    {
        throw std::runtime_error("Recv timeout setting failed!");
    }

    std::cout << "TTL = " << ttl << std::endl;
    std::cout << "Recv timeout seconds = " << tv << " ms\n\n";

    int      pings      = 1;
    uint16_t sequence_n = 0;

    char* icmp_data = reinterpret_cast<char*>(
        HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_PACKET_SIZE));
    char* recvbuf = reinterpret_cast<char*>(
        HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_PACKET_SIZE));

    memset(icmp_data, 0, MAX_PACKET_SIZE);

    while (pings != std::stoi(argv[1]))
    {
        sequence_n++;
        CreatePacket(icmp_data, ping_packet_size, sequence_n);

        icmphdr* hdr = (icmphdr*)icmp_data;

        std::cout << "Sending packet " << ntohs(hdr->un.echo.sequence) << " to "
                  << dest_addr->h_name
                  << " request with id = " << ntohs(hdr->un.echo.id)
                  << std::endl;

        if (sendto(sock,
                   icmp_data,
                   sizeof(icmp_data),
                   0,
                   reinterpret_cast<const struct sockaddr*>(&addr),
                   sizeof(addr)) < sizeof(icmp_data))
        {
            std::cerr << "Packet was not sent!\n";
            continue;
        }

        socklen_t addr_len   = sizeof(sockaddr);
        recv_addr.sin_family = AF_INET;
        recv_addr.sin_addr   = addr.sin_addr;

        auto start_time = std::chrono::steady_clock::now();

        if (recvfrom(sock,
                     recvbuf,
                     sizeof(recvbuf),
                     0,
                     reinterpret_cast<sockaddr*>(&recv_addr),
                     &addr_len) < sizeof(recvbuf))
        {
            std::cerr << "Packet was not received!\n";
            continue;
        }

        DecodePacket(recvbuf, start_time, &recv_addr);
        ++pings;
    }

    HeapFree(GetProcessHeap(), 0, recvbuf);
    HeapFree(GetProcessHeap(), 0, icmp_data);

    return EXIT_SUCCESS;
}