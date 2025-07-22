#include "market_feed_simulator.h"
#include <iostream>
#include <chrono>
#include <thread>

int main() {
    try {
        MarketFeedSimulator nyse_simulator("224.1.1.1", 9001);
        MarketFeedSimulator nasdaq_simulator("224.1.1.2", 9002);
        
        nyse_simulator.addInstrument(1, "AAPL", 150.00, 0.025);
        nyse_simulator.addInstrument(2, "GOOGL", 2500.00, 0.030);
        nyse_simulator.addInstrument(3, "MSFT", 300.00, 0.020);
        nyse_simulator.addInstrument(4, "TSLA", 800.00, 0.050);
        nyse_simulator.addInstrument(5, "AMZN", 3200.00, 0.030);
        
        nasdaq_simulator.addInstrument(101, "NVDA", 400.00, 0.040);
        nasdaq_simulator.addInstrument(102, "META", 250.00, 0.035);
        nasdaq_simulator.addInstrument(103, "NFLX", 400.00, 0.035);
        nasdaq_simulator.addInstrument(104, "AMD", 80.00, 0.045);
        nasdaq_simulator.addInstrument(105, "INTC", 50.00, 0.025);
        
        nyse_simulator.setMessageRate(800);
        nasdaq_simulator.setMessageRate(600);
        
        nyse_simulator.start();
        nasdaq_simulator.start();
        
        std::cout << "Market simulators running..." << std::endl;
        std::cout << "NYSE: 224.1.1.1:9001" << std::endl;
        std::cout << "NASDAQ: 224.1.1.2:9002" << std::endl;
        
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "Market opening - enabling burst mode" << std::endl;
        nyse_simulator.setBurstMode(true);
        nasdaq_simulator.setBurstMode(true);
        
        std::this_thread::sleep_for(std::chrono::seconds(10));
        
        std::cout << "Normal trading mode" << std::endl;
        nyse_simulator.setBurstMode(false);
        nasdaq_simulator.setBurstMode(false);
        
        std::this_thread::sleep_for(std::chrono::seconds(20));
        
        std::cout << "Market closing" << std::endl;
        nyse_simulator.setMarketOpen(false);
        nasdaq_simulator.setMarketOpen(false);
        
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        nyse_simulator.stop();
        nasdaq_simulator.stop();
        
        std::cout << "Simulation complete" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}