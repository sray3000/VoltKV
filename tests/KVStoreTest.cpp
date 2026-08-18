#include <cassert>
#include <iostream>
#include "../include/KVStore.h"

void testSetAndGet() {
    KVStore store;

    store.set("name", "Satyaki");

    auto value = store.get("name");

    assert(value.has_value());
    assert(value.value() == "Satyaki");
}

void testUpdate() {
    KVStore store;

    store.set("name", "Satyaki");
    store.set("name", "Alex");

    auto value = store.get("name");

    assert(value.has_value());
    assert(value.value() == "Alex");
}

void testGetMissingKey() {
    KVStore store;

    auto value = store.get("unknown");

    assert(!value.has_value());
}

void testExists() {
    KVStore store;

    assert(!store.exists("name"));

    store.set("name", "Satyaki");

    assert(store.exists("name"));
}

void testErase() {
    KVStore store;

    store.set("name", "Satyaki");

    assert(store.exists("name"));

    bool status = store.erase("name");

    assert(status);
    assert(!store.exists("name"));
    assert(!store.get("name").has_value());
}

void testEraseMissingKey() {
    KVStore store;

    bool status = store.erase("unknown");

    assert(!status);
}

void testEmptyValue() {
    KVStore store;

    store.set("empty", "");

    auto value = store.get("empty");

    assert(value.has_value());
    assert(value.value().empty());
}

void testMultipleKeys() {
    KVStore store;

    store.set("name", "Satyaki");
    store.set("age", "22");
    store.set("city", "Guwahati");

    assert(store.get("name").value() == "Satyaki");
    assert(store.get("age").value() == "22");
    assert(store.get("city").value() == "Guwahati");

    assert(store.exists("name"));
    assert(store.exists("age"));
    assert(store.exists("city"));
}

void testDeleteOneKeyDoesNotAffectOthers() {
    KVStore store;

    store.set("name", "Satyaki");
    store.set("age", "22");

    assert(store.erase("name"));

    assert(!store.exists("name"));
    assert(store.exists("age"));
    assert(store.get("age").value() == "22");
}

int main() {
    testSetAndGet();
    testUpdate();
    testGetMissingKey();
    testExists();
    testErase();
    testEraseMissingKey();
    testEmptyValue();
    testMultipleKeys();
    testDeleteOneKeyDoesNotAffectOthers();

    std::cout << "All KVStore tests passed!\n";

    return 0;
}