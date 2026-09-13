#include <atomic>
#include <iostream>
#include <map>
#include <shared_mutex>
#include <string>
#include <thread>
#include <vector>

class dns_cache {
public:
    std::string find(const std::string& name) const {
        std::shared_lock<std::shared_mutex> lock(m_);
        auto it = entries_.find(name);
        return it == entries_.end() ? std::string{} : it->second;
    }

    void update(const std::string& name, const std::string& entry) {
        std::lock_guard<std::shared_mutex> lock(m_);
        entries_[name] = entry;
    }

private:
    std::map<std::string, std::string> entries_;
    mutable std::shared_mutex m_;
};

int main() {
    dns_cache cache;

    const std::vector<std::string> base_names = {
        "www.example.com", "api.example.com", "mail.example.com"};
    for (size_t i = 0; i < base_names.size(); ++i) {
        cache.update(base_names[i], "10.0.0." + std::to_string(i + 1));
    }

    const int writers_count = 2;
    const int readers_count = 4;
    const int writer_keys = 500;
    const int reader_iters = 1000;

    std::atomic<bool> mismatch{false};

    std::vector<std::thread> threads;
    for (int w = 0; w < writers_count; ++w) {
        threads.emplace_back([&cache, w] {
            for (int i = 0; i < writer_keys; ++i) {
                std::string name = "host" + std::to_string(w * writer_keys + i);
                std::string entry = "10.0.1." + std::to_string(w * 100 + i);
                cache.update(name, entry);
            }
        });
    }

    for (int r = 0; r < readers_count; ++r) {
        threads.emplace_back([&cache, &mismatch, &base_names] {
            for (int i = 0; i < reader_iters; ++i) {
                std::string name;
                std::string expected;
                if (i % 2 == 0) {
                    int idx = (i / 2) % static_cast<int>(base_names.size());
                    name = base_names[idx];
                    expected = "10.0.0." + std::to_string(idx + 1);
                } else {
                    int w = i % writers_count;
                    int k = (i / 2) % writer_keys;
                    name = "host" + std::to_string(w * writer_keys + k);
                    expected = "10.0.1." + std::to_string(w * 100 + k);
                }
                std::string value = cache.find(name);
                if (!value.empty() && value != expected) {
                    mismatch.store(true);
                }
            }
        });
    }
    for (auto& t : threads) t.join();

    for (int w = 0; w < writers_count; ++w) {
        for (int i = 0; i < writer_keys; ++i) {
            std::string name = "host" + std::to_string(w * writer_keys + i);
            std::string expected = "10.0.1." + std::to_string(w * 100 + i);
            if (cache.find(name) != expected) {
                mismatch.store(true);
            }
        }
    }

    if (mismatch.load()) {
        std::cout << "Checks failed\n";
        return 1;
    }
    std::cout << "All checks passed\n";
    return 0;
}