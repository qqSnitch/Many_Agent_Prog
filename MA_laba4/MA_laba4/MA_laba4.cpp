//#include <iostream>
//#include <vector>
//#include <cmath>
//#include <fstream>
//
//class DynamicSystem {
//private:
//    // Параметры системы
//    double T;      // Постоянная времени объекта
//    double k;      // Коэффициент усиления регулятора
//    double tau;    // Запаздывание
//    double dt;     // Шаг по времени
//
//    // Векторы для хранения состояний
//    std::vector<double> x;      // Текущее состояние
//    std::vector<double> u;      // Управление
//    std::vector<double> time;   // Время
//
//    // Вспомогательные переменные для хранения истории
//    std::vector<double> x_history;
//
//public:
//    DynamicSystem(double T_val, double k_val, double tau_val, double dt_val)
//        : T(T_val), k(k_val), tau(tau_val), dt(dt_val) {
//        // Начальные условия
//        x.push_back(0.0);
//        u.push_back(0.0);
//        time.push_back(0.0);
//
//        // Инициализация истории
//        x_history.resize(static_cast<int>(tau / dt) + 2, 0.0);
//    }
//
//    // Функция Хевисайда
//    double heaviside(double t) {
//        return (t >= 0) ? 1.0 : 0.0;
//    }
//
//    // Получить запаздывающее состояние
//    double getDelayedState(double current_time) {
//        double delayed_time = current_time - tau;
//        if (delayed_time <= 0) return 0.0;
//
//        int index = static_cast<int>(delayed_time / dt);
//        if (index >= x.size()) return x.back();
//        if (index < 0) return 0.0;
//
//        return x[index];
//    }
//
//    // Один шаг моделирования
//    void step() {
//        double current_time = time.back() + dt;
//
//        // Цель - функция Хевисайда
//        double g = heaviside(current_time);
//
//        // Запаздывающее состояние
//        double x_delayed = getDelayedState(current_time);
//
//        // Управление (действие агента)
//        double u_current = k * (g - x_delayed);
//
//        // Обновление состояния системы
//        double x_prev = x.back();
//        double x_current = (T / (T + dt)) * x_prev + (dt / (T + dt)) * u_current;
//
//        // Сохраняем результаты
//        x.push_back(x_current);
//        u.push_back(u_current);
//        time.push_back(current_time);
//    }
//
//    // Моделирование до заданного времени
//    void simulate(double t_stop) {
//        int steps = static_cast<int>((t_stop - time.back()) / dt);
//        for (int i = 0; i < steps; ++i) {
//            step();
//        }
//    }
//
//    // Анализ устойчивости
//    bool isStable(double threshold = 0.1) {
//        // Проверяем, установилось ли состояние около 1
//        int check_points = 10;
//        double avg_value = 0.0;
//
//        for (int i = x.size() - check_points; i < x.size(); ++i) {
//            if (i >= 0) avg_value += x[i];
//        }
//        avg_value /= check_points;
//
//        return std::abs(avg_value - 1.0) < threshold;
//    }
//
//    // Поиск критического запаздывания
//    static double findCriticalTau(double T_val, double k_val, double dt_val,
//        double tau_max = 5.0, double tau_step = 0.1) {
//        double critical_tau = 0.0;
//
//        for (double tau_test = 0.0; tau_test <= tau_max; tau_test += tau_step) {
//            DynamicSystem system(T_val, k_val, tau_test, dt_val);
//            system.simulate(100.0); // Моделируем до 100 секунд
//
//            if (!system.isStable()) {
//                critical_tau = tau_test;
//                break;
//            }
//        }
//
//        return critical_tau;
//    }
//
//    // Запись результатов в файл
//    void saveToFile(const std::string& filename) {
//        std::ofstream file(filename);
//        file << "Time,State,Control\n";
//        for (size_t i = 0; i < time.size(); ++i) {
//            file << time[i] << "," << x[i] << "," << u[i] << "\n";
//        }
//        file.close();
//    }
//
//    // Геттеры для доступа к данным
//    const std::vector<double>& getTime() const { return time; }
//    const std::vector<double>& getState() const { return x; }
//    const std::vector<double>& getControl() const { return u; }
//};
//
//// Функция для исследования зависимости критического tau от параметров
//void studyTauDependence() {
//    std::ofstream file("tau_dependence.csv");
//    file << "T,k,critical_tau\n";
//
//    double dt = 0.01;
//
//    // Исследуем зависимость от T
//    for (double T = 0.5; T <= 3.0; T += 0.2) {
//        double k = 2.0;
//        double critical_tau = DynamicSystem::findCriticalTau(T, k, dt);
//        file << T << "," << k << "," << critical_tau << "\n";
//        std::cout << "T=" << T << ", k=" << k << ", critical_tau=" << critical_tau << std::endl;
//    }
//
//    // Исследуем зависимость от k
//    for (double k = 0.5; k <= 5.0; k += 0.2) {
//        double T = 1.0;
//        double critical_tau = DynamicSystem::findCriticalTau(T, k, dt);
//        file << T << "," << k << "," << critical_tau << "\n";
//        std::cout << "T=" << T << ", k=" << k << ", critical_tau=" << critical_tau << std::endl;
//    }
//
//    file.close();
//}
//
//int main() {
//    // Параметры по умолчанию
//    double T = 1.0;    // Постоянная времени объекта
//    double k = 2.0;    // Коэффициент усиления регулятора
//    double tau = 0.5;  // Запаздывание
//    double dt = 0.01;  // Шаг по времени
//    double t_stop = 15.0; // Время моделирования
//
//    // Пример 1: Устойчивая система
//    std::cout << "Example 1: Stable system (tau = 0.5)" << std::endl;
//    DynamicSystem system1(T, k, 0, dt);
//    system1.simulate(t_stop);
//    system1.saveToFile("stable_system.csv");
//    std::cout << "System stable?: " << (system1.isStable() ? "YES" : "NO") << std::endl;
//
//    // Пример 2: Неустойчивая система
//    std::cout << "\nExample 2: Unstable system (tau = 2.0)" << std::endl;
//    DynamicSystem system2(T, k, 2.0, dt);
//    system2.simulate(t_stop);
//    system2.saveToFile("unstable_system.csv");
//    std::cout << "System stable?: " << (system2.isStable() ? "YES" : "NO") << std::endl;
//
//    // Исследование зависимости критического tau от параметров
//    studyTauDependence();
//}




#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>

// Функция Хевисайда
double heaviside(double t) {
    return (t >= 0) ? 1.0 : 0.0;
}

// Класс для моделирования динамической системы второго порядка
class DynamicSystem {
private:
    double a0, a1, a2;  // Коэффициенты дифференциального уравнения: a2*x'' + a1*x' + a0*x = u
    double k;           // Коэффициент усиления регулятора
    double tau;         // Запаздывание
    double dt;          // Шаг по времени

    // Состояния системы (с учетом запаздывания)
    std::vector<double> x_history;      // История x(t)
    std::vector<double> dx_history;     // История x'(t)

public:
    DynamicSystem(double a0, double a1, double a2, double k, double tau, double dt)
        : a0(a0), a1(a1), a2(a2), k(k), tau(tau), dt(dt) {
        // Инициализация начальных условий
        x_history.push_back(0.0);
        dx_history.push_back(0.0);
    }

    // Вычисление следующего состояния системы
    void step(double t) {
        // Текущий индекс в истории
        int current_idx = x_history.size() - 1;

        // Определение индекса для запаздывающего состояния
        int delay_steps = static_cast<int>(tau / dt);
        int delayed_idx = std::max(0, current_idx - delay_steps);

        // Получение запаздывающего состояния
        double x_delayed = x_history[delayed_idx];

        // Вычисление управления (пропорциональный регулятор)
        double g = heaviside(t - tau);  // Цель с учетом запаздывания
        double u = k * (g - x_delayed);

        // Решение дифференциального уравнения методом Эйлера
        // a2*x'' + a1*x' + a0*x = u
        // Преобразуем в систему уравнений первого порядка:
        // x' = v
        // v' = (u - a1*v - a0*x) / a2

        double x_prev = x_history[current_idx];
        double v_prev = dx_history[current_idx];

        double v_new = v_prev + dt * (u - a1 * v_prev - a0 * x_prev) / a2;
        double x_new = x_prev + dt * v_prev;

        x_history.push_back(x_new);
        dx_history.push_back(v_new);
    }

    // Получение текущего состояния
    double getCurrentState() const {
        return x_history.back();
    }

    // Получение всей истории состояний
    const std::vector<double>& getHistory() const {
        return x_history;
    }

    // Очистка истории (для нового моделирования)
    void clearHistory() {
        x_history.clear();
        dx_history.clear();
        x_history.push_back(0.0);
        dx_history.push_back(0.0);
    }
};

// Функция для исследования устойчивости системы
void stabilityAnalysis(double a0, double a1, double a2, double k,
    double tau_min, double tau_max, double tau_step,
    double dt, double simulation_time) {

    std::ofstream output("stability_analysis.txt");
    output << "tau,is_stable,max_overshoot,settling_time\n";

    for (double tau = tau_min; tau <= tau_max; tau += tau_step) {
        DynamicSystem system(a0, a1, a2, k, tau, dt);

        // Моделирование системы
        int steps = static_cast<int>(simulation_time / dt);
        double max_overshoot = 0.0;
        double settling_time = simulation_time;
        bool is_stable = true;

        for (int i = 0; i < steps; ++i) {
            double t = i * dt;
            system.step(t);

            double current_state = system.getCurrentState();

            // Проверка на устойчивость (расходимость)
            if (std::abs(current_state) > 1.0) {
                is_stable = false;
                break;
            }

            // Определение максимального перерегулирования
            if (std::abs(current_state - 1.0) > max_overshoot) {
                max_overshoot = std::abs(current_state - 1.0);
            }

            // Определение времени установления (в пределах 2% от установившегося значения)
            if (is_stable && std::abs(current_state - 1.0) < 0.02 && t < settling_time) {
                settling_time = t;
            }
        }

        output << tau << " " << (is_stable ? 1 : 0) << " "
            << max_overshoot << " " << settling_time << "\n";

        std::cout << "tau = " << tau << ": " << (is_stable ? "Stable" : "Unstable")
            << ", Max overshoot = " << max_overshoot
            << ", Settling time = " << settling_time << std::endl;
    }

    output.close();
}

// Функция для построения временных характеристик при заданном запаздывании
void plotTimeResponse(double a0, double a1, double a2, double k,
    double tau, double dt, double simulation_time,
    const std::string& filename) {

    std::ofstream output(filename);
    output << "time,state,goal\n";

    DynamicSystem system(a0, a1, a2, k, tau, dt);
    int steps = static_cast<int>(simulation_time / dt);

    for (int i = 0; i < steps; ++i) {
        double t = i * dt;
        system.step(t);

        double state = system.getCurrentState();
        double goal = heaviside(t);

        output << t << " " << state << " " << goal << "\n";
    }

    output.close();
}

int main() {
    // Параметры системы
    double a0 = 1.0;    // Коэффициент при x
    double a1 = 2.0;    // Коэффициент при x'
    double a2 = 1.0;    // Коэффициент при x''
    double k = 2.0;     // Коэффициент усиления регулятора

    double dt = 0.01;   // Шаг по времени
    double simulation_time = 20.0; // Время моделирования

    // Анализ устойчивости в зависимости от запаздывания
    std::cout << "Stable system analisys:" << std::endl;
    stabilityAnalysis(a0, a1, a2, k, 0.0, 2.0, 0.1, dt, simulation_time);

    // Построение временных характеристик для разных запаздываний
    plotTimeResponse(a0, a1, a2, k, 0.1, dt, simulation_time, "response_tau_0.1.txt");
    plotTimeResponse(a0, a1, a2, k, 0.5, dt, simulation_time, "response_tau_0.5.txt");
    plotTimeResponse(a0, a1, a2, k, 1.0, dt, simulation_time, "response_tau_1.0.txt");
    plotTimeResponse(a0, a1, a2, k, 1.5, dt, simulation_time, "response_tau_1.5.txt");
}