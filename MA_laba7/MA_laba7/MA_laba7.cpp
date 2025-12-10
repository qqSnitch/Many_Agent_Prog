#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <memory>
#include <limits>

using namespace std;

const int AREA_SIZE = 100;
const double PI = 3.1415;

struct SimulationParams {
    int n = 100; // количество агентов
    int m = 5; // количество изначально зараженных
    double v_h = 1.0; // базовая скорость здорового
    double r_h = 10.0; // базовый радиус видимости
    double alpha_min = 90.0; // минимальный угол видимости
    double alpha_max = 150.0; // максимальный угол видимости
    int t_init = 10; // время до появления первых зараженных
    int t_inc_min = 50; // минимальный инкубационный период
    int t_inc_max = 100; // максимальный инкубационный период
    int t_move_min = 10; // минимальное время движения в одном направлении
    int t_move_max = 30; // максимальное время движения
    int T = 1000; // максимальное время моделирования
    double recovery_chance = 0.01; // шанс выздоровления зомби
    double re_zombie_chance = 0.25; // шанс повторного заражения выздоровевшего
};

class Vector2D {
public:
    double x, y;

    Vector2D(double x = 0, double y = 0) : x(x), y(y) {}

    Vector2D operator+(const Vector2D& other) const {
        return Vector2D(x + other.x, y + other.y);
    }

    Vector2D operator-(const Vector2D& other) const {
        return Vector2D(x - other.x, y - other.y);
    }

    Vector2D operator*(double scalar) const {
        return Vector2D(x * scalar, y * scalar);
    }

    double length() const {
        return sqrt(x * x + y * y);
    }

    Vector2D normalized() const {
        double len = length();
        if (len < 1e-10) return Vector2D(0, 0);
        return Vector2D(x / len, y / len);
    }

    double dot(const Vector2D& other) const {
        return x * other.x + y * other.y;
    }

    double angle() const {
        return atan2(y, x);
    }

    // Поворот вектора на угол
    Vector2D rotated(double angle) const {
        double cos_a = cos(angle);
        double sin_a = sin(angle);
        return Vector2D(x * cos_a - y * sin_a, x * sin_a + y * cos_a);
    }

    // Расстояние до другой точки
    double distanceTo(const Vector2D& other) const {
        double dx = x - other.x;
        double dy = y - other.y;
        return sqrt(dx * dx + dy * dy);
    }
};

enum class AgentState {
    HEALTHY,
    INFECTED,
    ZOMBIE,
    RECOVERED
};

class Agent {
private:
    static int next_id;
    int id;

public:
    AgentState state;
    Vector2D position;
    Vector2D velocity;
    Vector2D direction;
    double speed;

    // Параметры видимости
    double view_angle; // в градусах
    double view_radius;

    // Таймеры
    int move_timer;
    int move_duration;
    int incubation_timer;
    int incubation_duration;

    // Цель для преследования/избегания
    weak_ptr<Agent> target_weak;

    // Уникальные характеристики для разных состояний
    double base_speed;
    double base_view_angle;
    double base_view_radius;

    Agent(double x, double y, double view_angle_deg, double view_radius_val)
        : id(next_id++), state(AgentState::HEALTHY), position(x, y),
        view_angle(view_angle_deg), view_radius(view_radius_val) {

        base_speed = 1.0;
        base_view_angle = view_angle_deg;
        base_view_radius = view_radius_val;

        // Случайное начальное направление
        random_device rd;
        mt19937 gen(rd());
        uniform_real_distribution<> dir_dist(0, 2 * PI);
        double angle = dir_dist(gen);
        direction = Vector2D(cos(angle), sin(angle));

        speed = base_speed;
        velocity = direction * speed;

        // Инициализация таймеров
        move_timer = 0;
        incubation_timer = -1;

        // Установка случайной длительности движения
        uniform_int_distribution<> move_dur_dist(10, 30);
        move_duration = move_dur_dist(gen);
    }

    ~Agent() {
        target_weak.reset();
    }

    int getId() const { return id; }

    shared_ptr<Agent> getTarget() const {
        return target_weak.lock();
    }

    void setTarget(const shared_ptr<Agent>& target) {
        target_weak = target;
    }

    void clearTarget() {
        target_weak.reset();
    }

    // Проверка, видит ли агент другого агента
    bool canSee(const shared_ptr<Agent>& other) const {
        if (!other) return false;

        // Проверка расстояния
        double dist = position.distanceTo(other->position);
        if (dist > view_radius) return false;

        // Проверка угла
        if (state == AgentState::HEALTHY || state == AgentState::ZOMBIE) {
            Vector2D to_other = other->position - position;
            double angle_to_other = to_other.angle();
            double agent_angle = direction.angle();

            // Нормализация углов
            double angle_diff = fmod(angle_to_other - agent_angle + 3 * PI, 2 * PI) - PI;
            double half_view = (view_angle * PI / 180.0) / 2.0;

            return abs(angle_diff) <= half_view;
        }

        return false; // Для зараженных и выздоровевших - ограниченная видимость
    }

    // Получить радиус действия
    double getActionRadius() const {
        if (state == AgentState::ZOMBIE) {
            return view_radius * 0.93; // На 7% меньше радиуса видимости
        }
        return 0;
    }

    // Обновить состояние на основе текущего статуса
    void updateState(const SimulationParams& params, mt19937& gen) {
        switch (state) {
        case AgentState::HEALTHY:
            updateHealthy(params, gen);
            break;
        case AgentState::INFECTED:
            updateInfected(params, gen);
            break;
        case AgentState::ZOMBIE:
            updateZombie(params, gen);
            break;
        case AgentState::RECOVERED:
            updateRecovered(params, gen);
            break;
        }
    }

    void updateHealthy(const SimulationParams& params, mt19937& gen) {
        // Скорость нормальная или повышенная при бегстве
        if (getTarget()) {
            speed = params.v_h * 1.25;
        }
        else {
            speed = params.v_h;
        }

        // Параметры видимости стандартные
        view_angle = base_view_angle;
        view_radius = params.r_h;
    }

    void updateInfected(const SimulationParams& params, mt19937& gen) {
        // Скорость снижена на 10%
        speed = params.v_h * 0.9;

        // Уменьшение инкубационного таймера
        if (incubation_timer > 0) {
            incubation_timer--;
            if (incubation_timer == 0) {
                state = AgentState::ZOMBIE;
                clearTarget();
            }
        }
    }

    void updateZombie(const SimulationParams& params, mt19937& gen) {
        // Скорость снижена на 15%
        speed = params.v_h * 0.85;

        // Уменьшенный угол видимости
        view_angle = base_view_angle * 0.65;
        // Увеличенный радиус видимости
        view_radius = params.r_h * 1.1;

        // Шанс выздоровления
        uniform_real_distribution<> recovery_dist(0.0, 1.0);
        if (recovery_dist(gen) < params.recovery_chance) {
            state = AgentState::RECOVERED;
            clearTarget();
        }
    }

    void updateRecovered(const SimulationParams& params, mt19937& gen) {
        // Стандартные характеристики
        speed = params.v_h;
        view_angle = base_view_angle;
        view_radius = params.r_h;
    }

    // Движение агента
    void move(double dt = 1.0) {
        // Обновление таймера движения
        move_timer++;

        // Если время движения истекло, генерируем новое направление
        if (move_timer >= move_duration) {
            generateNewDirection();
            move_timer = 0;
        }

        // Обновление скорости
        velocity = direction * speed;

        // Перемещение
        Vector2D new_position = position + velocity * dt;

        // Проверка границ с отражением
        if (new_position.x < 0) {
            direction.x = abs(direction.x);
            new_position.x = 0;
        }
        else if (new_position.x > AREA_SIZE) {
            direction.x = -abs(direction.x);
            new_position.x = AREA_SIZE;
        }

        if (new_position.y < 0) {
            direction.y = abs(direction.y);
            new_position.y = 0;
        }
        else if (new_position.y > AREA_SIZE) {
            direction.y = -abs(direction.y);
            new_position.y = AREA_SIZE;
        }

        position = new_position;
    }

    // Генерация нового направления движения
    void generateNewDirection(mt19937* gen_ptr = nullptr) {
        static random_device rd;
        static mt19937 default_gen(rd());
        mt19937& gen = gen_ptr ? *gen_ptr : default_gen;

        uniform_real_distribution<> dir_dist(0, 2 * PI);
        double angle = dir_dist(gen);
        direction = Vector2D(cos(angle), sin(angle));

        uniform_int_distribution<> duration_dist(10, 30);
        move_duration = duration_dist(gen);
        move_timer = 0;
    }

    // Избегание зомби (для здоровых агентов)
    void avoidZombie(const shared_ptr<Agent>& zombie) {
        if (!zombie) return;

        Vector2D to_zombie = zombie->position - position;
        double angle_to_zombie = to_zombie.angle();
        double agent_angle = direction.angle();

        // Нормализация углов
        double angle_diff = fmod(angle_to_zombie - agent_angle + 3 * PI, 2 * PI) - PI;

        // Половина угла видимости в радианах
        double half_view = (view_angle * PI / 180.0) / 2.0;

        if (angle_diff > 0) {
            // Зомби справа - поворот влево
            direction = direction.rotated(-half_view * 0.5);
        }
        else {
            // Зомби слева - поворот вправо
            direction = direction.rotated(half_view * 0.5);
        }

        // Нормализация направления
        direction = direction.normalized();
        move_timer = 0; // Сброс таймера для немедленной реакции
    }

    // Разворот на 180 градусов
    void turnAround() {
        direction = direction * -1;
        move_timer = 0;
    }

    // Преследование цели
    void pursueTarget() {
        auto target = getTarget();
        if (!target) return;

        Vector2D to_target = target->position - position;
        if (to_target.length() > 1e-10) {
            direction = to_target.normalized();
        }
        move_timer = 0; // Непрерывное преследование
    }

    // Проверка, находится ли цель в радиусе действия
    bool isTargetInActionRange() const {
        auto target = getTarget();
        if (!target) return false;
        double dist = position.distanceTo(target->position);
        return dist <= getActionRadius();
    }
};

int Agent::next_id = 0;

class Simulation {
private:
    SimulationParams params;
    vector<shared_ptr<Agent>> agents;

    int current_time;
    bool simulation_finished;

    random_device rd;
    mt19937 gen;

public:
    Simulation(const SimulationParams& p) : params(p), current_time(0),
        simulation_finished(false),
        gen(rd()) {}

    // Инициализация симуляции
    void initialize() {
        agents.clear();
        current_time = 0;
        simulation_finished = false;

        // Создание агентов
        uniform_real_distribution<> pos_dist(0, AREA_SIZE);
        uniform_real_distribution<> angle_dist(params.alpha_min, params.alpha_max);

        for (int i = 0; i < params.n; i++) {
            double x = pos_dist(gen);
            double y = pos_dist(gen);
            double view_angle = angle_dist(gen);

            auto agent = make_shared<Agent>(x, y, view_angle, params.r_h);

            // Установка случайной длительности движения
            uniform_int_distribution<> move_dur_dist(params.t_move_min, params.t_move_max);
            agent->move_duration = move_dur_dist(gen);
            agent->generateNewDirection(&gen);

            agents.push_back(agent);
        }
    }

    // Получение списков агентов по состояниям
    vector<shared_ptr<Agent>> getHealthyAgents() const {
        vector<shared_ptr<Agent>> healthy;
        for (const auto& agent : agents) {
            if (agent->state == AgentState::HEALTHY) {
                healthy.push_back(agent);
            }
        }
        return healthy;
    }

    vector<shared_ptr<Agent>> getInfectedAgents() const {
        vector<shared_ptr<Agent>> infected;
        for (const auto& agent : agents) {
            if (agent->state == AgentState::INFECTED) {
                infected.push_back(agent);
            }
        }
        return infected;
    }

    vector<shared_ptr<Agent>> getZombieAgents() const {
        vector<shared_ptr<Agent>> zombies;
        for (const auto& agent : agents) {
            if (agent->state == AgentState::ZOMBIE) {
                zombies.push_back(agent);
            }
        }
        return zombies;
    }

    vector<shared_ptr<Agent>> getRecoveredAgents() const {
        vector<shared_ptr<Agent>> recovered;
        for (const auto& agent : agents) {
            if (agent->state == AgentState::RECOVERED) {
                recovered.push_back(agent);
            }
        }
        return recovered;
    }

    // Выполнение одного шага симуляции
    void step() {
        if (simulation_finished) return;

        current_time++;

        // Проверка условия окончания
        if (current_time >= params.T) {
            simulation_finished = true;
            return;
        }

        auto healthy_agents = getHealthyAgents();
        if (healthy_agents.empty()) {
            simulation_finished = true;
            return;
        }

        // Заражение первых m агентов через время t_init
        if (current_time == params.t_init) {
            auto all_agents = agents;
            shuffle(all_agents.begin(), all_agents.end(), gen);

            int infected_count = 0;
            for (auto& agent : all_agents) {
                if (infected_count >= params.m) break;

                if (agent->state == AgentState::HEALTHY) {
                    agent->state = AgentState::INFECTED;

                    // Установка инкубационного периода
                    uniform_int_distribution<> inc_dist(params.t_inc_min, params.t_inc_max);
                    agent->incubation_duration = inc_dist(gen);
                    agent->incubation_timer = agent->incubation_duration;

                    infected_count++;
                }
            }
        }

        // Обновление состояний всех агентов
        for (auto& agent : agents) {
            agent->updateState(params, gen);
        }

        // Формирование целей
        formTargets();

        // Движение агентов
        for (auto& agent : agents) {
            agent->move();
        }

        // Исполнение целей
        executeTargets();
    }

    // Формирование целей для агентов
    void formTargets() {
        auto zombies = getZombieAgents();
        auto healthy = getHealthyAgents();

        // Зомби ищут здоровых агентов
        for (auto& zombie : zombies) {
            zombie->clearTarget();

            shared_ptr<Agent> nearest_healthy = nullptr;
            double min_dist = numeric_limits<double>::max();

            for (auto& healthy_agent : healthy) {
                if (zombie->canSee(healthy_agent)) {
                    double dist = zombie->position.distanceTo(healthy_agent->position);
                    if (dist < min_dist) {
                        min_dist = dist;
                        nearest_healthy = healthy_agent;
                    }
                }
            }

            if (nearest_healthy) {
                zombie->setTarget(nearest_healthy);
            }
        }

        // Здоровые ищут зомби для избегания
        for (auto& healthy_agent : healthy) {
            healthy_agent->clearTarget();

            vector<shared_ptr<Agent>> seen_zombies;

            for (auto& zombie : zombies) {
                if (healthy_agent->canSee(zombie)) {
                    seen_zombies.push_back(zombie);
                }
            }

            if (!seen_zombies.empty()) {
                // Проверка с какой стороны зомби
                bool left = false, right = false;

                for (auto& zombie : seen_zombies) {
                    Vector2D to_zombie = zombie->position - healthy_agent->position;
                    double angle_to_zombie = to_zombie.angle();
                    double agent_angle = healthy_agent->direction.angle();

                    double angle_diff = fmod(angle_to_zombie - agent_angle + 3 * PI, 2 * PI) - PI;

                    if (angle_diff > 0) {
                        right = true;
                    }
                    else {
                        left = true;
                    }
                }

                if (left && right) {
                    // Зомби с обеих сторон - разворот
                    healthy_agent->turnAround();
                }
                else if (right && !seen_zombies.empty()) {
                    // Зомби справа - избегание влево
                    healthy_agent->avoidZombie(seen_zombies[0]);
                }
                else if (left && !seen_zombies.empty()) {
                    // Зомби слева - избегание вправо
                    healthy_agent->avoidZombie(seen_zombies[0]);
                }

                // Устанавливаем ближайшего зомби как цель для информации
                if (!seen_zombies.empty()) {
                    healthy_agent->setTarget(seen_zombies[0]);
                }
            }
        }
    }

    // Исполнение целей
    void executeTargets() {
        auto zombies = getZombieAgents();
        auto healthy = getHealthyAgents();
        auto recovered = getRecoveredAgents();

        // Зомби заражают здоровых
        for (auto& zombie : zombies) {
            if (zombie->isTargetInActionRange()) {
                auto target = zombie->getTarget();

                // Проверка, что цель все еще здорова и существует
                if (target && target->state == AgentState::HEALTHY) {
                    target->state = AgentState::INFECTED;

                    // Установка инкубационного периода
                    uniform_int_distribution<> inc_dist(params.t_inc_min, params.t_inc_max);
                    target->incubation_duration = inc_dist(gen);
                    target->incubation_timer = target->incubation_duration;
                }
            }

            // Преследование цели
            zombie->pursueTarget();
        }

        // Выздоровевшие могут снова стать зомби
        for (auto& recovered_agent : recovered) {
            for (auto& zombie : zombies) {
                double dist = recovered_agent->position.distanceTo(zombie->position);
                if (dist <= zombie->getActionRadius()) {
                    uniform_real_distribution<> chance_dist(0.0, 1.0);
                    if (chance_dist(gen) < params.re_zombie_chance) {
                        recovered_agent->state = AgentState::ZOMBIE;
                        break;
                    }
                }
            }
        }
    }

    // Запуск полной симуляции
    int run() {
        initialize();
        while (!simulation_finished) {
            step();
        }

        return getHealthyAgents().empty() ? current_time : -1;
    }

    // Получение статистики
    void printStats() const {
        auto healthy = getHealthyAgents();
        auto infected = getInfectedAgents();
        auto zombies = getZombieAgents();
        auto recovered = getRecoveredAgents();

        cout << "Время: " << current_time << "\n";
        cout << "Здоровые: " << healthy.size() << "\n";
        cout << "Зараженные: " << infected.size() << "\n";
        cout << "Зомби: " << zombies.size() << "\n";
        cout << "Выздоровевшие: " << recovered.size() << "\n";
        cout << "Всего агентов: " << agents.size() << "\n";
    }

    // Проверка, завершена ли симуляция
    bool isFinished() const { return simulation_finished; }

    // Геттеры для статистики
    int getCurrentTime() const { return current_time; }
    int getHealthyCount() const { return getHealthyAgents().size(); }
    int getZombieCount() const { return getZombieAgents().size(); }
    int getInfectedCount() const { return getInfectedAgents().size(); }
    int getRecoveredCount() const { return getRecoveredAgents().size(); }

    const vector<shared_ptr<Agent>>& getAgents() const { return agents; }
};

void runSingleExperiment(const SimulationParams& params) {
    cout << "\nЗапуск одиночной симуляции...\n";
    cout << "Параметры:\n";
    cout << "  Количество агентов: " << params.n << "\n";
    cout << "  Начальных зараженных: " << params.m << "\n";
    cout << "  Базовая скорость: " << params.v_h << "\n";
    cout << "  Радиус видимости: " << params.r_h << "\n";

    auto start = chrono::high_resolution_clock::now();

    Simulation sim(params);
    int time = sim.run();

    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);

    cout << "\nРезультаты симуляции:\n";
    cout << "=====================\n";

    if (time > 0) {
        cout << "Все здоровые агенты превратились в зомби за " << time << " итераций\n";
    }
    else {
        cout << "Симуляция завершена по времени (" << params.T << " итераций)\n";
        cout << "Осталось здоровых: " << sim.getHealthyCount() << "\n";
    }

    sim.printStats();
    cout << "\nВремя выполнения: " << duration.count() << " мс\n";
}

void runMultipleExperiments() {
    ofstream output("experiment_results.txt");
    output << "n m v_h r_h avg_zombification_time success_rate\n";

    // Базовые параметры
    SimulationParams base_params;
    base_params.v_h = 1.0;
    base_params.r_h = 10.0;
    base_params.t_init = 10;
    base_params.t_inc_min = 50;
    base_params.t_inc_max = 100;
    base_params.T = 2000;

    // Эксперимент 1: Разное количество агентов
    for (int n : {50, 100, 150, 200}) {
        for (int m : {1, 2, 5, 10}) {
            base_params.m = m;
            base_params.n = n;
            SimulationParams p = base_params;

            cout << "\nТестирование n=" << n << " m=" << m <<"\n";

            int successful = 0;
            long long total_time = 0;

            for (int i = 0; i < 1000; i++) {
                Simulation sim(p);
                int time = sim.run();

                if (time > 0) {
                    successful++;
                    total_time += time;
                }
            }

            double avg_time = successful > 0 ? static_cast<double>(total_time) / successful : 0;
            double success_rate = successful / 1000.0 * 100.0;

            output << p.n << " " << p.m << " " << p.v_h << " " << p.r_h << " " << avg_time << " " << success_rate << "%\n";

            cout << "  Среднее время: " << avg_time << ", Успешных: " << success_rate << "%\n";
        }
    }

    output.close();
    cout << "\nРезультаты сохранены в experiment_results.txt\n";
}

int main() {
    setlocale(LC_ALL, "Russian");

    ofstream output("experiment_results.txt");
    output << "n m v_h r_h avg_zombification_time success_rate\n";
    auto start = chrono::high_resolution_clock::now();

    // Базовые параметры
    SimulationParams base_params;
    base_params.v_h = 1.0;
    base_params.r_h = 10.0;
    base_params.t_init = 10;
    base_params.t_inc_min = 50;
    base_params.t_inc_max = 100;
    base_params.T = 2000;

    // Эксперимент 1: Разное количество агентов
    for (int n : {50, 100, 150, 200}) {
        for (int m : {1, 2, 5, 10}) {
            base_params.m = m;
            base_params.n = n;
            SimulationParams p = base_params;

            cout << "\nТестирование n=" << n << " m=" << m << "\n";

            int successful = 0;
            long long total_time = 0;

            for (int i = 0; i < 1000; i++) {
                Simulation sim(p);
                int time = sim.run();

                if (time > 0) {
                    successful++;
                    total_time += time;
                }
            }

            double avg_time = successful > 0 ? static_cast<double>(total_time) / successful : 0;
            double success_rate = successful / 1000.0 * 100.0;

            output << p.n << " " << p.m << " " << p.v_h << " " << p.r_h << " " << avg_time << " " << success_rate << "%\n";

            cout << "  Среднее время: " << avg_time << ", Успешных: " << success_rate << "%\n";
        }
    }


    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::seconds>(end - start);

    output.close();
    cout << "\nРезультаты сохранены в experiment_results.txt\n";

    cout << "\nВсе эксперименты завершены!\n";
    cout << "Общее время выполнения: " << duration.count() << " секунд\n";
}