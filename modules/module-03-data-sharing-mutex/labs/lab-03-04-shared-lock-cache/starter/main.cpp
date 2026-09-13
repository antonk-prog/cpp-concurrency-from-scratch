// Лаба 3.4. Напиши потокобезопасный DNS-кэш с нуля.
// Требования — в task.md. Здесь каркас, который компилируется.
#include <map>
#include <shared_mutex>
#include <string>

class dns_cache {
    // Напиши: entries_ (std::map<std::string, std::string>),
    // mutable std::shared_mutex m_,
    // методы find() (через shared_lock) и update() (через lock_guard).
};

int main() {
    dns_cache cache;
    (void)cache;
    // Напиши: предзагрузку базовых записей, 4 читателя и 2 писателя
    // (формулы — в task.md), финальные проверки после join(),
    // вывод "All checks passed".
    return 0;
}