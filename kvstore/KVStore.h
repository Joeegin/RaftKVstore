//
// Created by 乔益金 on 25-6-26.
//

#ifndef KVSTORE_H
#define KVSTORE_H


#include <string>
#include <unordered_map>
#include <optional>

class KVStore {
public:
    KVStore(const std::string& filename);
    ~KVStore();

    void put(const std::string &key,const std::string &value );
    std::optional<std::string> get(const std::string &key) const;

    void loadFromDisk();
    void saveToDisk() const;
private:
    std::unordered_map<std::string ,std::string> _store;
    std::string _filename;

};



#endif //KVSTORE_H
