// Лаба 3.4. Реализуй потокобезопасный DNS-кэш.
// Требования — в task.md. Здесь только класс dns_cache.
#include <map>
#include <shared_mutex>
#include <string>

class dns_cache {
    // TODO: добавь приватные поля
    //   std::map<std::string, std::string> entries_;
    //   mutable std::shared_mutex m_;
    // и методы:
    //   std::string find(const std::string& name) const
    //       — возвращает запись (или пустую строку), использует std::shared_lock;
    //   void update(const std::string& name, const std::string& entry)
    //       — кладёт запись в кэш, использует std::lock_guard<std::shared_mutex>.
};