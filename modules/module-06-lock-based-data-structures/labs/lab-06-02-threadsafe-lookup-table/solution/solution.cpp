#include <vector>
#include <memory>
#include <mutex>
#include <algorithm>
#include <functional>
#include <list>
#include <utility>
#include <shared_mutex>
#include <thread>
#include <iostream>
#include <cstdlib>

template <typename Key, typename Value,
          typename Hash = std::hash<Key>>
class threadsafe_lookup_table {
    class bucket_type {
    public:
        using bucket_value = std::pair<Key, Value>;
        using bucket_data = std::list<bucket_value>;
        using bucket_iterator = typename bucket_data::iterator;

        bucket_data data;
        mutable std::shared_mutex mutex;

        bucket_iterator find_entry_for(Key const& key) {
            return std::find_if(data.begin(), data.end(),
                [&](bucket_value const& item) {
                    return item.first == key;
                });
        }

        Value value_for(Key const& key, Value const& default_value) const {
            std::shared_lock<std::shared_mutex> lock(mutex);
            auto const found = std::find_if(data.begin(), data.end(),
                [&](bucket_value const& item) {
                    return item.first == key;
                });
            return (found == data.end()) ? default_value : found->second;
        }

        void add_or_update_mapping(Key const& key, Value const& value) {
            std::unique_lock<std::shared_mutex> lock(mutex);
            bucket_iterator const found_entry = find_entry_for(key);
            if (found_entry == data.end()) {
                data.push_back(bucket_value(key, value));
            } else {
                found_entry->second = value;
            }
        }

        void remove_mapping(Key const& key) {
            std::unique_lock<std::shared_mutex> lock(mutex);
            bucket_iterator const found_entry = find_entry_for(key);
            if (found_entry != data.end()) {
                data.erase(found_entry);
            }
        }
    };

    std::vector<std::unique_ptr<bucket_type>> buckets;
    Hash hasher;

    bucket_type& get_bucket(Key const& key) const {
        std::size_t const bucket_index = hasher(key) % buckets.size();
        return *buckets[bucket_index];
    }

public:
    using key_type = Key;
    using mapped_type = Value;

    threadsafe_lookup_table(unsigned num_buckets = 19,
                            Hash const& hasher_ = Hash())
        : buckets(num_buckets), hasher(hasher_) {
        for (unsigned i = 0; i < num_buckets; ++i) {
            buckets[i].reset(new bucket_type);
        }
    }

    threadsafe_lookup_table(threadsafe_lookup_table const&) = delete;
    threadsafe_lookup_table& operator=(
        threadsafe_lookup_table const&) = delete;

    Value value_for(Key const& key,
                    Value const& default_value = Value()) const {
        return get_bucket(key).value_for(key, default_value);
    }

    void add_or_update_mapping(Key const& key, Value const& value) {
        get_bucket(key).add_or_update_mapping(key, value);
    }

    void remove_mapping(Key const& key) {
        get_bucket(key).remove_mapping(key);
    }
};

int main() {
    const int writers = 4;
    const int readers = 4;
    const int per_writer = 10000;
    threadsafe_lookup_table<int, int> table;

    std::vector<std::thread> threads;
    for (int w = 0; w < writers; ++w) {
        threads.emplace_back([&, w] {
            for (int i = 0; i < per_writer; ++i) {
                int key = w * per_writer + i;
                table.add_or_update_mapping(key, key * 2);
            }
        });
    }
    for (auto& th : threads) th.join();

    std::vector<std::thread> readers_threads;
    for (int r = 0; r < readers; ++r) {
        readers_threads.emplace_back([&] {
            for (int w = 0; w < writers; ++w) {
                for (int i = 0; i < per_writer; ++i) {
                    int key = w * per_writer + i;
                    int val = table.value_for(key, -1);
                    if (val != key * 2) {
                        std::cout << "FAIL: bad value for key " << key
                                  << ": " << val << "\n";
                        std::exit(1);
                    }
                }
            }
        });
    }
    for (auto& th : readers_threads) th.join();

    std::cout << "OK\n";
    return 0;
}