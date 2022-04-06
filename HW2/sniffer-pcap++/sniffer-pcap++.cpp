#include <iostream>
#include <string.h>
#include <string>
#include <cctype>
#include <cerrno>

#include "PcapFileDevice.h"
#include "PcapLiveDeviceList.h"
#include "PcapFilter.h"
#include "SystemUtils.h"
#include "stdlib.h"

// Default snap length (maximum bytes per packet to capture).
const auto SNAP_LEN = 1518;
const auto MAX_PACKET_TO_CAPTURE = 10;

int main(int argc, const char * const argv[])
{
    std::string dev_name;
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
        std::cout
            << "Usage: " << argv[0] << " [interface]\n\n"
            << "Options:\n"
            << "    interface    Listen on <interface> for packets.\n"
            << std::endl;

        exit(EXIT_FAILURE);
    }
    else
    {
        dev = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByName(dev_name);

        if (dev == nullptr)
        {
            std::cerr << "Couldn't find device with name: '" << dev_name << "'" << std::endl;
            return EXIT_FAILURE;
        }
    }

    // Print interface info.
    std::cout
        << "Interface info:\n" 
        << "    Interface name: " << dev->getName() << "\n"
        << "    Interface description: " << dev->getDesc() << "\n"
        << "    MAC address: " << dev->getMacAddress() << "\n"
        << "    Default gateway: " << dev->getDefaultGateway() << "\n"
        << "    Interface MTU: " << dev->getMtu() << std::endl;

    // Open capture device.
    if (!dev->open())
    {
        std::cerr << "Couldn't open device '" << dev->getName() << "'" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Setting filter.
    pcpp::ProtoFilter fl(pcpp::IP);
    if (!dev->setFilter(fl))
    {
        std::cerr << "Couldn't install filter 'IP'!" << std::endl;
        exit(EXIT_FAILURE);
    }

    pcpp::PcapFileWriterDevice pcapWriter("output.pcap", pcpp::LINKTYPE_ETHERNET);

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

    std::cout << "Capture complete." << std::endl;

    return EXIT_SUCCESS;
}
