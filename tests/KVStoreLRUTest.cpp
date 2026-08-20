#include <iostream>
#include <cstdlib>
#include <thread>
#include <chrono>

#include "KVStore.h"

void testEvictsLeastRecentlyUsed() {
    KVStore store(3);

    store.set("A", "1");
    store.set("B", "2");
    store.set("C", "3");

    // A is currently the least recently used.
    store.set("D", "4");

    if(store.exists("A")) {
        std::cerr << "FAIL: A should have been evicted\n";
        exit(1);
    }

    if(!store.exists("B") ||
       !store.exists("C") ||
       !store.exists("D")) {
        std::cerr << "FAIL: incorrect eviction\n";
        exit(1);
    }
}

void testGetUpdatesLRUOrder() {
    KVStore store(3);

    store.set("A", "1");
    store.set("B", "2");
    store.set("C", "3");

    // Make A the most recently used.
    auto value = store.get("A");

    if(!value || *value != "1") {
        std::cerr << "FAIL: GET A\n";
        exit(1);
    }

    // B should now be the least recently used.
    store.set("D", "4");

    if(store.exists("B")) {
        std::cerr << "FAIL: B should have been evicted\n";
        exit(1);
    }

    if(!store.exists("A") ||
       !store.exists("C") ||
       !store.exists("D")) {
        std::cerr << "FAIL: incorrect LRU order after GET\n";
        exit(1);
    }
}

void testSetUpdatesLRUOrder() {
    KVStore store(3);

    store.set("A", "1");
    store.set("B", "2");
    store.set("C", "3");

    // Updating A should make A most recently used.
    store.set("A", "updated");

    store.set("D", "4");

    if(store.exists("B")) {
        std::cerr << "FAIL: B should have been evicted\n";
        exit(1);
    }

    auto value = store.get("A");

    if(!value || *value != "updated") {
        std::cerr << "FAIL: A was not updated correctly\n";
        exit(1);
    }
}

void testEraseUpdatesLRU() {
    KVStore store(3);

    store.set("A", "1");
    store.set("B", "2");
    store.set("C", "3");

    if(!store.erase("B")) {
        std::cerr << "FAIL: B could not be erased\n";
        exit(1);
    }

    store.set("D", "4");

    if(store.exists("A") == false ||
       store.exists("C") == false ||
       store.exists("D") == false) {
        std::cerr << "FAIL: incorrect state after erase\n";
        exit(1);
    }
}

void testTTLEntryParticipatesInLRU() {
    KVStore store(3);

    store.set("A", "1");
    store.set(
        "B",
        "2",
        std::chrono::seconds(10)
    );
    store.set("C", "3");

    // Access A, making it most recently used.
    auto value = store.get("A");

    if(!value || *value != "1") {
        std::cerr << "FAIL: GET A\n";
        exit(1);
    }

    // Current order:
    // A -> C -> B
    //
    // Therefore B is the least recently used.
    store.set("D", "4");

    if(store.exists("B")) {
        std::cerr << "FAIL: TTL entry B should have been evicted\n";
        exit(1);
    }

    if(!store.exists("A") ||
       !store.exists("C") ||
       !store.exists("D")) {
        std::cerr << "FAIL: incorrect TTL/LRU interaction\n";
        exit(1);
    }
}

void testUpdatingTTLEntry() {
    KVStore store(3);

    store.set(
        "A",
        "old",
        std::chrono::seconds(1)
    );

    store.set("B", "2");
    store.set("C", "3");

    store.set(
        "A",
        "new",
        std::chrono::seconds(10)
    );

    auto value = store.get("A");

    if(!value || *value != "new") {
        std::cerr << "FAIL: TTL entry was not updated\n";
        exit(1);
    }

    if(!store.exists("B") ||
       !store.exists("C")) {
        std::cerr << "FAIL: updating existing TTL entry caused eviction\n";
        exit(1);
    }

    std::this_thread::sleep_for(
        std::chrono::milliseconds(1200)
    );

    value = store.get("A");

    if(!value) {
        std::cerr << "FAIL: TTL was not reset on update\n";
        exit(1);
    }
}

int main() {

    testEvictsLeastRecentlyUsed();
    testGetUpdatesLRUOrder();
    testSetUpdatesLRUOrder();
    testEraseUpdatesLRU();
    testTTLEntryParticipatesInLRU();
    testUpdatingTTLEntry();

    std::cout << "All LRU tests passed.\n";

    return 0;
}