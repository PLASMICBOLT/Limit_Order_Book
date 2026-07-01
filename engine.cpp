#include <iostream>
#include <map>
#include <list>
#include <string>
#include <algorithm>
#include <chrono>

using namespace std;
using namespace std::chrono;

// Basic order structure
struct Order {
    string id;
    bool is_buy;
    double price;
    int qty;
    long long timestamp;
};

class MatchingEngine {
private:
    // bids sorted descending (highest price first)
    map<double, list<Order>, greater<double>> bids;
    
    // asks sorted ascending (lowest price first)
    map<double, list<Order>> asks;

public:
    void addOrder(Order order) {
        if (order.is_buy) {
            // keep matching while we have quantity and there are sellers
            while (order.qty > 0 && !asks.empty()) {
                auto best_ask = asks.begin();
                
                // if buyer price is >= cheapest seller, we have a match
                if (order.price >= best_ask->first) {
                    auto& resting = best_ask->second.front();
                    int trade_qty = min(order.qty, resting.qty);
                    
                    order.qty -= trade_qty;
                    resting.qty -= trade_qty;
                    
                    // remove seller if their order is completely filled (O(1) pop)
                    if (resting.qty == 0) {
                        best_ask->second.pop_front();
                        // clean up the map if the price level is empty
                        if (best_ask->second.empty()) {
                            asks.erase(best_ask);
                        }
                    }
                } else {
                    break; // spread not crossed
                }
            }
            // if buyer still wants more, add them to the book
            if (order.qty > 0) {
                bids[order.price].push_back(order);
            }
        } else {
            // same logic for sellers matching against buyers
            while (order.qty > 0 && !bids.empty()) {
                auto best_bid = bids.begin();
                
                if (order.price <= best_bid->first) {
                    auto& resting = best_bid->second.front();
                    int trade_qty = min(order.qty, resting.qty);
                    
                    order.qty -= trade_qty;
                    resting.qty -= trade_qty;
                    
                    if (resting.qty == 0) {
                        best_bid->second.pop_front();
                        if (best_bid->second.empty()) {
                            bids.erase(best_bid);
                        }
                    }
                } else {
                    break;
                }
            }
            if (order.qty > 0) {
                asks[order.price].push_back(order);
            }
        }
    }

    // quick debug print to check book state
    void printBook() {
        cout << "\n--- ASKS ---\n";
        for (auto it = asks.rbegin(); it != asks.rend(); ++it) {
            int total_vol = 0;
            for (const auto& o : it->second) total_vol += o.qty;
            cout << "$" << it->first << " : " << total_vol << "\n";
        }

        cout << "--- BIDS ---\n";
        for (auto it = bids.begin(); it != bids.end(); ++it) {
            int total_vol = 0;
            for (const auto& o : it->second) total_vol += o.qty;
            cout << "$" << it->first << " : " << total_vol << "\n";
        }
        cout << "\n";
    }
};

int main() {
    MatchingEngine engine;
    
    // Visual test of the matching engine
    cout << "Running logic test...\n";
    engine.addOrder({"T1", false, 151.00, 100, 1}); 
    engine.addOrder({"T2", true, 150.00, 35, 2});   
    engine.printBook();
    
    cout << "Aggressive buy: 120 shares @ $151\n";
    engine.addOrder({"T3", true, 151.00, 120, 3}); 
    engine.printBook();

    // Performance benchmark
    int num_orders = 100000;
    cout << "Benchmarking with " << num_orders << " random orders...\n";

    auto start = high_resolution_clock::now();

    for (int i = 0; i < num_orders; ++i) {
        bool is_buy = rand() % 2; 
        double price = 100.0 + (rand() % 100); 
        int qty = 10 + (rand() % 90); 
        
        engine.addOrder({"ID_" + to_string(i), is_buy, price, qty, i});
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);

    cout << "Processed 100k orders in: " << duration.count() << " ms\n";

    return 0;
}