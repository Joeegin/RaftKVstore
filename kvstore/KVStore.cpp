//
// Created by 乔益金 on 25-6-26.
//

#include "KVStore.h"
#include <fstream>
#include <iostream>

KVStore::KVStore(const std::string &filename):_filename(filename) {
    loadFromDisk();
}

KVStore::~KVStore() {
    saveToDisk();
}

void KVStore::put(const std::string &key, const std::string &value) {
    _store[key]=value;
    saveToDisk();
}

std::optional<std::string> KVStore::get(const std::string &key) const {

    auto it=_store.find(key);
    if (it!=_store.end()) {
        return it->second;
    }
    return std::nullopt;
}

void KVStore::loadFromDisk() {
    std::ifstream in(_filename);
    if (!in) {
        return;
    }
    std::string key,value;
    while (in >> key >> value) {
        _store[key]=value;
    }
}

void KVStore::saveToDisk() const {
    std::ofstream out(_filename);
    if (!out) {
        std::cerr << "Failed to open file for writing: " << _filename << std::endl;
        return;
    }
    for (const auto& [key, value] : _store) {
        out << key << '\t' << value << '\n';
    }
}
