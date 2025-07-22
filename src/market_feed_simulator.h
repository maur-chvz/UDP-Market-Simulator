#ifndef MARKET_FEED_SIMULATOR_H
#define MARKET_FEED_SIMULATOR_H

#include <vector>
#include <random>
#include <thread>
#include <atomic>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <cerrno>

struct Instrument {
    uint32_t id;
    std::string symbol;
    double base_price;
    double current_price;
    double volatility;
    uint64_t volume;
    uint32_t sequence_number;
};

struct RawMarketMessage {
    uint64_t timestamp;
    uint32_t instrument_id;
    double price;
    uint64_t volume;
    char side;
    uint32_t sequence_number;
}__attribute__((packed));

class MarketFeedSimulator {
private:
    std::vector<Instrument> instruments;
    std::string multicast_ip;
    int port;
    int socket_fd;
    std::atomic<bool> running{false};
    std::thread simulator_thread;
    
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_real_distribution<> price_change_dist;
    std::uniform_real_distribution<> volume_dist;
    std::uniform_int_distribution<> instrument_dist;
    std::uniform_int_distribution<> side_dist;
    
    double tick_size = 0.01;
    int messages_per_second = 1000;
    int burst_multiplier = 5;
    bool market_open = true;
    
public:
    MarketFeedSimulator(const std::string& multicast_ip, int port);
    ~MarketFeedSimulator();
    
    void addInstrument(uint32_t id, const std::string& symbol, double base_price, double volatility = 0.02);
    void setMessageRate(int msgs_per_sec) { messages_per_second = msgs_per_sec; }
    void setBurstMode(bool enabled) { burst_multiplier = enabled ? 10 : 1; }
    void setMarketOpen(bool open) { market_open = open; }
    
    void start();
    void stop();
    
private:
    void setupSocket();
    std::string getDefaultInterfaceIP();
    void simulateMarket();
    void generateMarketUpdate();
    void sendMessage(const RawMarketMessage& msg);
    double generatePriceChange(double current_price, double volatility);
    uint64_t generateVolume();
    uint64_t getCurrentMicroseconds();
};

#endif