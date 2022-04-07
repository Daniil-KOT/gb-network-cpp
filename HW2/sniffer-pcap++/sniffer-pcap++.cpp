#include <cctype>
#include <cerrno>
#include <iostream>
#include <string.h>
#include <string>


#include "EthLayer.h"
#include "HttpLayer.h"
#include "IPv4Layer.h"
#include "Packet.h"
#include "PcapFileDevice.h"
#include "PcapFilter.h"
#include "PcapLiveDeviceList.h"
#include "SystemUtils.h"
#include "TcpLayer.h"
#include "stdlib.h"


// Default snap length (maximum bytes per packet to capture).
const auto SNAP_LEN              = 1518;
const auto MAX_PACKET_TO_CAPTURE = 10;

std::string getProtocolTypeAsString(pcpp::ProtocolType protocolType)
{
    switch (protocolType)
    {
        case pcpp::Ethernet:
            return "Ethernet";
        case pcpp::IPv4:
            return "IPv4";
        case pcpp::TCP:
            return "TCP";
        case pcpp::ICMP:
            return "ICMP";
        case pcpp::HTTPRequest:
        case pcpp::HTTPResponse:
            return "HTTP";
        default:
            return "Unknown";
    }
}

void packetInfo(pcpp::Packet& parsedPacket, int packetNum)
{
    pcpp::Layer* layer = parsedPacket.getFirstLayer();

    for (; layer != nullptr; layer = layer->getNextLayer())
    {
        std::cout << "Packet " << packetNum << ":\n"
                  << "\tLayer type: "
                  << getProtocolTypeAsString(layer->getProtocol()) << ";\n"
                  << "\tTotal data: " << layer->getDataLen() << " bytes;\n"
                  << "\tLayer data: " << layer->getHeaderLen() << " bytes;\n"
                  << "\tLayer payload: " << layer->getLayerPayloadSize()
                  << " bytes;\n";

        if (layer->getProtocol() == pcpp::IPv4)
        {
            pcpp::IPv4Layer* ipLayer = (pcpp::IPv4Layer*)layer;
            std::cout << "\tReceived from: " << ipLayer->getSrcIPAddress()
                      << ";\n"
                      << "\tSent to: " << ipLayer->getDstIPAddress() << ";\n"
                      << "\tTTL: " << (int)ipLayer->getIPv4Header()->timeToLive
                      << ";\n";
        }

        std::cout << std::endl;
    }
}

int main(int argc, const char* const argv[])
{
    std::string           dev_name;
    pcpp::PcapLiveDevice* dev = nullptr;

    // Number of packets to capture.
    int num_packets = MAX_PACKET_TO_CAPTURE;

    // Check for capture device name on command-line.
    if (2 == argc)
    {
        dev_name = argv[1];
    }
    else if (argc > 2)
    {
        std::cerr << "error: unrecognized command-line options\n" << std::endl;
        std::cout << "Usage: " << argv[0] << " [interface]\n\n"
                  << "Options:\n"
                  << "    interface    Listen on <interface> for packets.\n"
                  << std::endl;

        exit(EXIT_FAILURE);
    }
    else
    {
        dev = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByName(
            dev_name);

        if (dev == nullptr)
        {
            std::cerr << "Couldn't find device with name: '" << dev_name << "'"
                      << std::endl;
            return EXIT_FAILURE;
        }
    }

    // Print interface info.
    std::cout << "Interface info:\n"
              << "    Interface name: " << dev->getName() << "\n"
              << "    Interface description: " << dev->getDesc() << "\n"
              << "    MAC address: " << dev->getMacAddress() << "\n"
              << "    Default gateway: " << dev->getDefaultGateway() << "\n"
              << "    Interface MTU: " << dev->getMtu() << std::endl;

    // Open capture device.
    if (!dev->open())
    {
        std::cerr << "Couldn't open device '" << dev->getName() << "'"
                  << std::endl;
        exit(EXIT_FAILURE);
    }

    // Setting filter.
    pcpp::ProtoFilter fl(pcpp::IP);
    if (!dev->setFilter(fl))
    {
        std::cerr << "Couldn't install filter 'IP'!" << std::endl;
        exit(EXIT_FAILURE);
    }

    pcpp::PcapFileWriterDevice pcapWriter("output.pcap",
                                          pcpp::LINKTYPE_ETHERNET);

    if (!pcapWriter.open())
    {
        std::cerr << "Cannot open .pcap file for writing!" << std::endl;
        exit(EXIT_FAILURE);
    }

    pcpp::RawPacketVector packets;

    std::cout << "Starting packet capture...\n";

    // Starting asynchronous packets capture.
    dev->startCapture(packets);

    // Slleping for 10 sec.
    pcpp::multiPlatformSleep(10);

    // Stopping asynchronous packets capture.
    dev->stopCapture();

    std::cout << "Writing packets into output.pcap...\n";
    pcapWriter.writePackets(packets);

    pcapWriter.close();

    pcpp::Packet parsedPacket;
    int          packetNumber = 1;

    // printing packets information
    for (auto it = packets.begin(); it != packets.end(); ++it)
    {
        parsedPacket.setRawPacket(*it, false);
        packetInfo(parsedPacket, packetNumber);
        ++packetNumber;
    }

    std::cout << "Capture complete.\n";

    return EXIT_SUCCESS;
}
