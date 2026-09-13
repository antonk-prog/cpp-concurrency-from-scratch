// Лаба 3.7. Напиши два потокобезопасных синглтона с нуля.
// Требования — в task.md. Здесь только класс Singleton и геттеры.
#include <memory>
#include <mutex>

class Singleton {
    // Напиши: конструктор, увеличивающий static int instances.
};

// Вариант 1: геттер на std::call_once + std::once_flag
// Вариант 2: геттер на static-локальной переменной
// (объяви здесь оба геттера)