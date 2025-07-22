// Implementation
#include "market_feed_simulator.h"
#include <iostream>
#include <unistd.h>
#include <cstring>

MarketFeedSimulator::MarketFeedSimulator(const std::string& multicast_ip, int port) 
    : multicast_ip(multicast_ip), port(port), gen(rd()), 
      price_change_dist(-0.01, 0.01), volume_dist(100, 10000), 
      instrument_dist(0, 0), side_dist(0, 1) {
    setupSocket();
}

MarketFeedSimulator::~MarketFeedSimulator() {
    stop();
    if (socket_fd >= 0) {
        close(socket_fd);
    }
}

std::string MarketFeedSimulator::getDefaultInterfaceIP() {
    int temp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (temp_sock < 0) {
        return "127.0.0.1";
    }
    
    struct sockaddr_in dummy_addr;
    memset(&dummy_addr, 0, sizeof(dummy_addr));
    dummy_addr.sin_family = AF_INET;
    dummy_addr.sin_addr.s_addr = inet_addr("8.8.8.8");
    dummy_addr.sin_port = htons(80);
    
    if (connect(temp_sock, (struct sockaddr*)&dummy_addr, sizeof(dummy_addr)) < 0) {
        close(temp_sock);
        return "127.0.0.1";
    }
    
    struct sockaddr_in local_addr;
    socklen_t addr_len = sizeof(local_addr);
    if (getsockname(temp_sock, (struct sockaddr*)&local_addr, &addr_len) < 0) {
        close(temp_sock);
        return "127.0.0.1";
    }
    
    close(temp_sock);
    return std::string(inet_ntoa(local_addr.sin_addr));
}

void MarketFeedSimulator::setupSocket() {
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        throw std::runtime_error("Failed to create socket");
    }
    
    int loopback = 1;
    if (setsockopt(socket_fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loopback, sizeof(loopback)) < 0) {
        perror("Failed to enable multicast loopback");
    }
    
    unsigned char ttl = 1;
    if (setsockopt(socket_fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0) {
        perror("Failed to set multicast TTL");
    }
    
    std::string interface_ip = getDefaultInterfaceIP();
    std::cout << "Using multicast interface: " << interface_ip << std::endl;
    
    struct in_addr localInterface;
    localInterface.s_addr = inet_addr(interface_ip.c_str());
    if (setsockopt(socket_fd, IPPROTO_IP, IP_MULTICAST_IF, &localInterface, sizeof(localInterface)) < 0) {
        perror("Failed to set multicast interface");
        std::cout << "Continuing without explicit interface binding..." << std::endl;
    }
}

void MarketFeedSimulator::addInstrument(uint32_t id, const std::string& symbol, 
                                       double base_price, double volatility) {
    Instrument instrument;
    instrument.id = id;
    instrument.symbol = symbol;
    instrument.base_price = base_price;
    instrument.current_price = base_price;
    instrument.volatility = volatility;
    instrument.volume = 0;
    instrument.sequence_number = 1;
    
    instruments.push_back(instrument);
    
    if (instruments.size() > 1) {
        instrument_dist = std::uniform_int_distribution<>(0, instruments.size() - 1);
    }
}

void MarketFeedSimulator::start() {
    if (instruments.empty()) {
        throw std::runtime_error("No instruments configured");
    }
    
    running.store(true);
    simulator_thread = std::thread(&MarketFeedSimulator::simulateMarket, this);
    std::cout << "Market feed simulator started for " << multicast_ip << ":" << port << std::endl;
}

void MarketFeedSimulator::stop() {
    running.store(false);
    if (simulator_thread.joinable()) {
        simulator_thread.join();
    }
    std::cout << "Market feed simulator stopped" << std::endl;
}

void MarketFeedSimulator::simulateMarket() {
    auto last_time = std::chrono::steady_clock::now();
    
    while (running.load()) {
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            current_time - last_time).count();
        
        int effective_rate = messages_per_second * (market_open ? burst_multiplier : 1);
        int target_interval = 1000000 / effective_rate;
        
        if (elapsed >= target_interval) {
            generateMarketUpdate();
            last_time = current_time;
        } else {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    }
}

void MarketFeedSimulator::generateMarketUpdate() {
    if (instruments.empty()) return;
    
    int idx = instrument_dist(gen);
    Instrument& instrument = instruments[idx];
    
    double price_change = generatePriceChange(instrument.current_price, instrument.volatility);
    instrument.current_price += price_change;
    
    if (instrument.current_price <= 0) {
        instrument.current_price = instrument.base_price * 0.5;
    }
    
    instrument.current_price = std::round(instrument.current_price / tick_size) * tick_size;
    
    uint64_t volume = generateVolume();
    instrument.volume += volume;
    
    RawMarketMessage msg;
    msg.timestamp = getCurrentMicroseconds();
    msg.instrument_id = instrument.id;
    msg.price = instrument.current_price;
    msg.volume = volume;
    msg.side = side_dist(gen) ? 'B' : 'S'; // Buy or Sell
    msg.sequence_number = instrument.sequence_number++;
    
    sendMessage(msg);
}

void MarketFeedSimulator::sendMessage(const RawMarketMessage& msg) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, multicast_ip.c_str(), &addr.sin_addr);
    
    ssize_t bytes_sent = sendto(socket_fd, &msg, sizeof(RawMarketMessage), 0,
                                (struct sockaddr*)&addr, sizeof(addr));
    
    if (bytes_sent < 0) {
        std::cerr << "Failed to send message: " << strerror(errno) << std::endl;
    } else if (bytes_sent != sizeof(RawMarketMessage)) {
        std::cerr << "Partial send: " << bytes_sent << " of " << sizeof(RawMarketMessage) << " bytes" << std::endl;
    }
}

double MarketFeedSimulator::generatePriceChange(double current_price, double volatility) {
    // Use geometric Brownian motion for more realistic price movement
    double random_factor = price_change_dist(gen);
    return current_price * volatility * random_factor;
}

uint64_t MarketFeedSimulator::generateVolume() {
    return static_cast<uint64_t>(volume_dist(gen));
}

uint64_t MarketFeedSimulator::getCurrentMicroseconds() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}