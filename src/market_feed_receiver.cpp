#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include <thread>
#include <cerrno>

#include "market_feed_simulator.h"

class UDPMulticastReceiver {
private:
    int socket_fd;
    std::string multicast_ip;
    int port;
    
public:
    UDPMulticastReceiver(const std::string& ip, int port) 
        : multicast_ip(ip), port(port), socket_fd(-1) {
        setupSocket();
    }
    
    ~UDPMulticastReceiver() {
        if (socket_fd >= 0) {
            close(socket_fd);
        }
    }
    
    void setupSocket() {
        // Create UDP socket
        socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (socket_fd < 0) {
            throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
        }
        
        int opt = 1;
        if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            throw std::runtime_error("Failed to set SO_REUSEADDR: " + std::string(strerror(errno)));
        }
        
        int bufsize = 65536;
        if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize)) < 0) {
            std::cerr << "Warning: Failed to set receive buffer size: " << strerror(errno) << std::endl;
        }
        
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;
        
        if (bind(socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            throw std::runtime_error("Failed to bind socket: " + std::string(strerror(errno)));
        }
        
        struct ip_mreq mreq;
        memset(&mreq, 0, sizeof(mreq));
        mreq.imr_multiaddr.s_addr = inet_addr(multicast_ip.c_str());
        mreq.imr_interface.s_addr = INADDR_ANY;
        
        if (setsockopt(socket_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
            throw std::runtime_error("Failed to join multicast group " + multicast_ip + ": " + std::string(strerror(errno)));
        }
        
        std::cout << "Successfully joined multicast group " << multicast_ip << ":" << port << std::endl;
    }
    
    void listen() {
        char buffer[1024];
        int message_count = 0;
        auto start_time = std::chrono::steady_clock::now();
        
        std::cout << "Listening for multicast data on " << multicast_ip << ":" << port << "..." << std::endl;
        
        while (true) {
            ssize_t bytes = recv(socket_fd, buffer, sizeof(buffer), 0);
            
            if (bytes > 0) {
                message_count++;
                
                std::cout << "\n=== Message #" << message_count << " (" << multicast_ip << ") ===\n";
                std::cout << "Received " << bytes << " bytes\n";
                
                // Show raw bytes (first 20 bytes)
                std::cout << "Raw bytes: ";
                for (int i = 0; i < std::min(20, (int)bytes); i++) {
                    printf("%02x ", (unsigned char)buffer[i]);
                }
                std::cout << std::endl;
                
                // Parse as our message structure
                if (bytes >= sizeof(RawMarketMessage)) {
                    RawMarketMessage* msg = reinterpret_cast<RawMarketMessage*>(buffer);
                    
                    std::cout << "Parsed message:\n";
                    std::cout << "  Timestamp: " << msg->timestamp << std::endl;
                    std::cout << "  Instrument ID: " << msg->instrument_id << std::endl;
                    std::cout << "  Price: $" << msg->price << std::endl;
                    std::cout << "  Volume: " << msg->volume << std::endl;
                    std::cout << "  Side: " << msg->side << std::endl;
                    std::cout << "  Sequence: " << msg->sequence_number << std::endl;
                    
                    // Calculate latency (rough estimate)
                    auto now = std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
                    auto latency = now - msg->timestamp;
                    std::cout << "  Latency: " << latency << " microseconds" << std::endl;
                } else {
                    std::cout << "Message too small for RawMarketMessage (" << bytes << " bytes)\n";
                }
                
                // Show message rate every 100 messages
                if (message_count % 100 == 0) {
                    auto now = std::chrono::steady_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
                    if (elapsed > 0) {
                        std::cout << "\n*** Message Rate: " << message_count / elapsed << " msgs/sec ***\n";
                    }
                }
                
            } else if (bytes == 0) {
                std::cout << "Connection closed\n";
                break;
            } else {
                std::cerr << "recv error: " << strerror(errno) << std::endl;
                break;
            }
        }
    }
};

int main() {
    std::cout << "Dual Exchange Receiver\n\n";
    
    try {
        std::thread nyse_thread([]{
            try {
                UDPMulticastReceiver nyse("224.1.1.1", 9001);
                std::cout << "NYSE thread started\n";
                nyse.listen();
            } catch (const std::exception& e) {
                std::cerr << "NYSE receiver error: " << e.what() << std::endl;
            }
        });
        
        std::thread nasdaq_thread([]{
            try {
                UDPMulticastReceiver nasdaq("224.1.1.2", 9002);
                std::cout << "NASDAQ thread started\n";
                nasdaq.listen();
            } catch (const std::exception& e) {
                std::cerr << "NASDAQ receiver error: " << e.what() << std::endl;
            }
        });
        
        nyse_thread.join();
        nasdaq_thread.join();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}