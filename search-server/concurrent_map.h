#include <cstdlib>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "log_duration.h"
#include "test_framework.h"

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
private:
    struct Paired {
        std::map<Key, Value> data;
        std::mutex mutex;
    };
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
    };

    explicit ConcurrentMap(size_t bucket_count) : pairs_(bucket_count) {}

    Access operator[](const Key& key) {
        auto index = static_cast<uint64_t>(key) % pairs_.size();
        auto& pair = pairs_[index];
        return {std::lock_guard(pair.mutex), pair.data[key]};
    }
    
    auto Erase(const Key& key) {
        auto index = static_cast<uint64_t>(key) % pairs_.size();
        auto& pair = pairs_[index];
        std::lock_guard guard(pair.mutex);
        return pair.data.erase(key);
    }
    
    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for (auto& [data, lock] : pairs_) {
            std::lock_guard g(lock);
            result.insert(data.begin(), data.end());
        }
        return result;
    }

private:
    std::vector<Paired> pairs_;
};