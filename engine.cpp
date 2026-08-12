#include <iostream>
#include <map>
#include <list>
#include <string>
#include <algorithm>
#include <chrono>
#include <unordered_map> // NEW: Required for O(1) lookups

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

    // Hash map mapping Order ID to its exact memory location in the linked list
    // we dont use raw pointer of Order* here because A raw pointer only points to the data, but an iterator points to the underlying linked-list node. 
    // The iterator is required so the erase function can properly work in O(1) time without breaking the chain.
    // by work I mean, if we use raw pointer, we would have to traverse the list to find the node to erase, which is O(n).
    unordered_map<string, list<Order>::iterator> order_map;

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
                        order_map.erase(resting.id); // Clean up hash map
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
                // insert returns the exact pointer to the new order
                auto& queue = bids[order.price];
                auto it = queue.insert(queue.end(), order);
                order_map[order.id] = it; // Store it for O(1) cancellation later
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
                        order_map.erase(resting.id); // Clean up hash map
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
                auto& queue = asks[order.price];
                auto it = queue.insert(queue.end(), order);
                order_map[order.id] = it;
            }
        }
    }

    // O(1) Order Cancellation
    void cancelOrder(const string& order_id) {
        // Find the order in the hash map in O(1)
        auto map_it = order_map.find(order_id);
        if (map_it == order_map.end()) {
            cout << "Order " << order_id << " not found (already filled or cancelled).\n";
            return; 
        }

        // Get the exact pointer to the node in the linked list
        auto list_it = map_it->second; 
        double price = list_it->price;
        bool is_buy = list_it->is_buy;

        // Unhook it from the linked list in O(1) without shifting memory this can be considered a plus over vector
        if (is_buy) {
            bids[price].erase(list_it); 
            if (bids[price].empty()) {
                bids.erase(price); // Clean up price level if empty
            }
        } else {
            asks[price].erase(list_it);
            if (asks[price].empty()) {
                asks.erase(price);
            }
        }

        // 4. Remove from hash map
        order_map.erase(map_it); 
        cout << "Order " << order_id << " cancelled successfully\n";
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
    
    // Test of the matching engine
    cout << "Running logic test...\n";
    engine.addOrder({"T1", false, 151.00, 100, 1}); 
    engine.addOrder({"T2", true, 150.00, 35, 2});   
    engine.printBook();
    
    cout << "Aggressive buy: 120 shares @ $151\n";
    engine.addOrder({"T3", true, 151.00, 120, 3}); 
    engine.printBook();

    // Test for O(1) Cancellation
    cout << "--------------------------------\n";
    cout << "Testing O(1) Order Cancellation...\n";
    engine.addOrder({"C1", true, 140.00, 50, 4});
    engine.addOrder({"C2", true, 140.00, 30, 5}); // C1 and C2 are at the same price
    engine.printBook();

    cout << "Cancelling order C1 (front of the $140 queue)...\n";
    engine.cancelOrder("C1");
    engine.printBook();
    cout << "--------------------------------\n\n";

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