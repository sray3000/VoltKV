#include <iostream>
#include <thread>
#include <chrono>

#include "KVStore.h"

void testPermanentKey() {
    KVStore store;

    store.set("name", "Satyaki");

    auto value = store.get("name");

    if(!value || *value != "Satyaki") {
        std::cerr << "FAIL: permanent key\n";
        exit(1);
    }
}

void testValueBeforeExpiry() {
    KVStore store;

    store.set(
        "temp",
        "hello",
        std::chrono::seconds(1)
    );

    auto value = store.get("temp");

    if(!value || *value != "hello") {
        std::cerr << "FAIL: key unavailable before expiry\n";
        exit(1);
    }
}

void testGetAfterExpiry() {
    KVStore store;

    store.set(
        "temp",
        "hello",
        std::chrono::seconds(1)
    );

    std::this_thread::sleep_for(
        std::chrono::milliseconds(1100)
    );

    auto value = store.get("temp");

    if(value) {
        std::cerr << "FAIL: expired key still exists\n";
        exit(1);
    }
}

void testExistsBeforeExpiry() {
    KVStore store;

    store.set(
        "exists_test",
        "value",
        std::chrono::seconds(1)
    );

    if(!store.exists("exists_test")) {
        std::cerr << "FAIL: key should exist before expiry\n";
        exit(1);
    }
}

void testExistsAfterExpiry() {
    KVStore store;

    store.set(
        "exists_test",
        "value",
        std::chrono::seconds(1)
    );

    std::this_thread::sleep_for(
        std::chrono::milliseconds(1100)
    );

    if(store.exists("exists_test")) {
        std::cerr << "FAIL: expired key reported by EXISTS\n";
        exit(1);
    }
}

void testRecreateExpiredKey() {
    KVStore store;

    store.set(
        "temp",
        "old_value",
        std::chrono::seconds(1)
    );

    std::this_thread::sleep_for(
        std::chrono::milliseconds(1100)
    );

    store.set(
        "temp",
        "new_value"
    );

    auto value = store.get("temp");

    if(!value || *value != "new_value") {
        std::cerr << "FAIL: expired key could not be recreated\n";
        exit(1);
    }
}

void testOverwriteResetsTTL() {
    KVStore store;

    store.set(
        "session",
        "old",
        std::chrono::seconds(1)
    );

    std::this_thread::sleep_for(
        std::chrono::milliseconds(500)
    );

    store.set(
        "session",
        "new",
        std::chrono::seconds(2)
    );

    std::this_thread::sleep_for(
        std::chrono::milliseconds(700)
    );

    auto value = store.get("session");

    if(!value || *value != "new") {
        std::cerr << "FAIL: overwriting key did not reset TTL\n";
        exit(1);
    }
}

int main() {

    testPermanentKey();
    testValueBeforeExpiry();
    testGetAfterExpiry();
    testExistsBeforeExpiry();
    testExistsAfterExpiry();
    testRecreateExpiredKey();
    testOverwriteResetsTTL();

    std::cout << "All TTL tests passed.\n";

    return 0;
}