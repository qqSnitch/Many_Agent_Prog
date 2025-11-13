#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <set>
#include <algorithm>
#include <random>
#include <unordered_map>
#include <limits>

using namespace std;

class Module {
public:
    int id;
    double load; // Нагрузка модуля
    vector<int> next_modules; // Следующие модули
    vector<int> prev_modules; // Предыдущие модули

    Module(int id, double load) : id(id), load(load) {}
};

class Agent {
public:
    int id;
    double current_load; // Текущая нагрузка
    int current_module; // Текущий выполняемый модуль (-1 если свободен)
    vector<int> neighbors; // Соседние агенты
    vector<int> assigned_modules; // Назначенные модули
    double completion_time; // Время завершения текущей задачи

    Agent(int id) : id(id), current_load(0), current_module(-1), completion_time(0) {}
};

// Класс для управления графом приложения
class ApplicationGraph {
public:
    vector<Module> modules;
    vector<int> start_modules; // Начальные модули
    vector<int> end_modules;   // Конечные модули

    void addModule(int id, double load) {
        modules.emplace_back(id, load);
    }

    void addDependency(int from, int to) {
        modules[from].next_modules.push_back(to);
        modules[to].prev_modules.push_back(from);
    }

    void findStartEndModules() {
        start_modules.clear();
        end_modules.clear();

        for (const auto& module : modules) {
            if (module.prev_modules.empty()) {
                start_modules.push_back(module.id);
            }
            if (module.next_modules.empty()) {
                end_modules.push_back(module.id);
            }
        }
    }

    const Module& getModule(int id) const {
        return modules[id];
    }
};

// Класс для управления графом агентов
class AgentGraph {
public:
    vector<Agent> agents;

    void addAgent(int id) {
        agents.emplace_back(id);
    }

    void addConnection(int agent1, int agent2) {
        agents[agent1].neighbors.push_back(agent2);
        agents[agent2].neighbors.push_back(agent1);
    }

    Agent& getAgent(int id) {
        return agents[id];
    }
};

// Основной класс для балансировки нагрузки
class LoadBalancer {
private:
    ApplicationGraph app_graph;
    AgentGraph agent_graph;
    mt19937 rng;

    // Текущее распределение: модуль -> агент
    unordered_map<int, int> module_to_agent;
    // Время завершения модулей
    unordered_map<int, double> module_completion_time;
    // 0-не начат, 1-выполняется, 2-завершен
    unordered_map<int, int> module_status;
    // Очередь готовых к выполнению модулей
    queue<int> ready_modules;

public:
    LoadBalancer() : rng(random_device{}()) {}

    // Инициализация графов
    void initializeGraphs(const ApplicationGraph& app, const AgentGraph& agents) {
        app_graph = app;
        agent_graph = agents;
        app_graph.findStartEndModules();

        // Инициализация статусов
        for (const auto& module : app_graph.modules) {
            module_status[module.id] = 0;
        }
    }

    // Начальное распределение нагрузки
    void initialDistribution() {
        module_to_agent.clear();
        module_completion_time.clear();

        // Распределяем все модули
        vector<int> all_modules;
        for (const auto& module : app_graph.modules) {
            all_modules.push_back(module.id);
        }

        // Сортируем модули по количеству зависимостей (сначала независимые)
        sort(all_modules.begin(), all_modules.end(), [this](int a, int b) {
            return app_graph.getModule(a).prev_modules.size() <
                app_graph.getModule(b).prev_modules.size();
            });

        for (int module_id : all_modules) {
            int best_agent = findLeastLoadedAgent();
            assignModuleToAgent(module_id, best_agent);
        }

        // Добавляем начальные модули в очередь готовых
        for (int start_id : app_graph.start_modules) {
            ready_modules.push(start_id);
        }

        cout << "Начальное распределение завершено" << endl;
        printDistribution();
    }

    // Поиск наименее загруженного агента
    int findLeastLoadedAgent() {
        int best_agent = 0;
        double min_load = agent_graph.agents[0].current_load;

        for (int i = 1; i < agent_graph.agents.size(); i++) {
            if (agent_graph.agents[i].current_load < min_load) {
                min_load = agent_graph.agents[i].current_load;
                best_agent = i;
            }
        }
        return best_agent;
    }

    // Назначение модуля агенту
    void assignModuleToAgent(int module_id, int agent_id) {
        module_to_agent[module_id] = agent_id;
        const Module& module = app_graph.getModule(module_id);
        agent_graph.agents[agent_id].assigned_modules.push_back(module_id);
        agent_graph.agents[agent_id].current_load += module.load;

        cout << "Модуль " << module_id << " назначен агенту " << agent_id<< " (нагрузка: " << module.load << ")" << endl;
    }

    // Выполнение приложения
    double executeApplication() {
        double current_time = 0;
        int completed_modules = 0;
        int total_modules = app_graph.modules.size();

        cout << "\nНачало выполнения приложения" << endl;

        while (completed_modules < total_modules) {
            // Если есть готовые модули, выполняем их
            if (!ready_modules.empty()) {
                int module_id = ready_modules.front();
                ready_modules.pop();

                if (module_status[module_id] != 0) continue;

                int agent_id = module_to_agent[module_id];
                const Module& module = app_graph.getModule(module_id);

                // Проверка на отказ оборудования
                if (uniform_real_distribution<double>(0, 1)(rng) < 0.05) {
                    cout << "Отказ на агенте " << agent_id<< " при выполнении модуля " << module_id << endl;
                    dynamicRebalance(module_id, agent_id);
                    agent_id = module_to_agent[module_id]; // Обновляем agent_id после перераспределения
                }

                // Выполнение модуля
                cout << "Агент " << agent_id << " выполняет модуль " << module_id<< " (нагрузка: " << module.load << ")" << endl;

                module_status[module_id] = 1;
                double completion_time = current_time + module.load;
                module_completion_time[module_id] = completion_time;

                // Завершение модуля
                module_status[module_id] = 2;
                completed_modules++;
                current_time = completion_time;

                cout << "Модуль " << module_id << " завершен" << endl;

                // Освобождение агента
                agent_graph.agents[agent_id].current_load -= module.load;
                agent_graph.agents[agent_id].current_module = -1;

                // Добавление следующих модулей в очередь, если они готовы
                for (int next_id : module.next_modules) {
                    if (isModuleReady(next_id)) {
                        ready_modules.push(next_id);
                        cout << "Модуль " << next_id << " готов к выполнению и добавлен в очередь" << endl;
                    }
                }

                // Проверка необходимости динамической балансировки
                if (needRebalancing()) {
                    cout << "Выполняется динамическая балансировка" << endl;
                    dynamicRebalance(-1, -1); // Общая балансировка
                }
            }
            else {
                // Если нет готовых модулей, увеличиваем время
                current_time += 0.1;

                bool found_ready = false;
                for (const auto& module : app_graph.modules) {
                    if (module_status[module.id] == 0 && isModuleReady(module.id)) {
                        ready_modules.push(module.id);
                        found_ready = true;
                        cout << "Модуль " << module.id << " обнаружен как готовый и добавлен в очередь" << endl;
                    }
                }

            }
        }
        cout << "Общее время выполнения: " << current_time << endl;

        return current_time;
    }

    // Проверка готовности модуля к выполнению
    bool isModuleReady(int module_id) {
        const Module& module = app_graph.getModule(module_id);
        for (int prev_id : module.prev_modules) {
            if (module_status[prev_id] != 2) {
                return false;
            }
        }
        return module_status[module_id] == 0;
    }

    // Проверка необходимости балансировки
    bool needRebalancing() {
        if (agent_graph.agents.size() <= 1) return false;

        double max_load = 0, min_load = numeric_limits<double>::max();

        for (const auto& agent : agent_graph.agents) {
            max_load = max(max_load, agent.current_load);
            min_load = min(min_load, agent.current_load);
        }

        // Балансировка нужна, если разница нагрузок превышает порог
        return (max_load - min_load) > 1.0;
    }

    // Динамическая перебалансировка
    void dynamicRebalance(int failed_module, int failed_agent) {
        if (failed_module != -1) {
            // Перераспределение отказавшего модуля
            int new_agent = findLeastLoadedAgent();
            if (new_agent != failed_agent) {
                cout << "Перемещение модуля " << failed_module << " с агента "<< failed_agent << " на агента " << new_agent << endl;

                // Удаление из старого агента
                auto& old_modules = agent_graph.agents[failed_agent].assigned_modules;
                old_modules.erase(remove(old_modules.begin(), old_modules.end(), failed_module), old_modules.end());
                agent_graph.agents[failed_agent].current_load -= app_graph.getModule(failed_module).load;

                // Добавление новому агенту
                assignModuleToAgent(failed_module, new_agent);
            }
        }
        else {
            // Общая балансировка нагрузки
            // Перемещаем модули от самых загруженных к наименее загруженным
            vector<pair<double, int>> agent_loads;
            for (int i = 0; i < agent_graph.agents.size(); i++) {
                agent_loads.emplace_back(agent_graph.agents[i].current_load, i);
            }

            sort(agent_loads.begin(), agent_loads.end());

            int left = 0, right = agent_loads.size() - 1;
            while (left < right) {
                int loaded_agent = agent_loads[right].second;
                int free_agent = agent_loads[left].second;

                if (agent_graph.agents[loaded_agent].current_load - agent_graph.agents[free_agent].current_load < 0.5) {
                    break;
                }

                // Находим модуль для перемещения
                int module_to_move = -1;
                for (int module_id : agent_graph.agents[loaded_agent].assigned_modules) {
                    if (module_status[module_id] == 0) { // Перемещаем только не начатые модули
                        module_to_move = module_id;
                        break;
                    }
                }

                if (module_to_move != -1) {
                    cout << "Балансировка: перемещение модуля " << module_to_move << " с агента " << loaded_agent << " на агента " << free_agent << endl;

                    // Перемещение модуля
                    auto& old_modules = agent_graph.agents[loaded_agent].assigned_modules;
                    old_modules.erase(remove(old_modules.begin(), old_modules.end(), module_to_move), old_modules.end());
                    agent_graph.agents[loaded_agent].current_load -= app_graph.getModule(module_to_move).load;

                    assignModuleToAgent(module_to_move, free_agent);
                }

                left++;
                right--;
            }
        }

        printDistribution();
    }

    // Вывод текущего распределения
    void printDistribution() {
        cout << "\nТекущее распределение нагрузки:" << endl;
        for (const auto& agent : agent_graph.agents) {
            cout << "Агент " << agent.id << ": нагрузка=" << agent.current_load<< ", модули=[";
            for (size_t i = 0; i < agent.assigned_modules.size(); i++) {
                cout << agent.assigned_modules[i];
                if (i < agent.assigned_modules.size() - 1) cout << ", ";
            }
            cout << "]" << endl;
        }
        cout << endl;
    }
};

// Тестовые примеры
void test1() {
    cout << "==========================================================";
    cout << "\nТЕСТ 1: Простая линейная цепочка" << endl;

    ApplicationGraph app;
    AgentGraph agents;

	random_device rd;
	mt19937 gen(rd());
	uniform_real_distribution<double> dist(0.0, 3.0);
    for (int i = 0; i < 4; i++) {
        app.addModule(i, dist(gen));
    }

    for (int i = 0; i < 3; i++) {
        app.addDependency(i, i + 1);
    }

    for (int i = 0; i < 2; i++) {
        agents.addAgent(i);
    }

    agents.addConnection(0, 1);

    LoadBalancer balancer;
    balancer.initializeGraphs(app, agents);
    balancer.initialDistribution();
    balancer.executeApplication();
}

void test2() {
    cout << "==========================================================";
    cout << "\nТЕСТ 2: Древовидная структура" << endl;

    ApplicationGraph app;
    AgentGraph agents;

    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<double> dist(0.0, 3.0);

    for (int i = 0; i < 6; i++) {
        app.addModule(i, dist(gen));
    }

    app.addDependency(0, 1);
    app.addDependency(0, 2);
    app.addDependency(1, 3);
    app.addDependency(1, 4);
    app.addDependency(2, 5);

    for (int i = 0; i < 3; i++) {
        agents.addAgent(i);
    }

    for (int i = 0; i < 3; i++) {
        for (int j = i + 1; j < 3; j++) {
            agents.addConnection(i, j);
        }
    }

    LoadBalancer balancer;
    balancer.initializeGraphs(app, agents);
    balancer.initialDistribution();
    balancer.executeApplication();
}

void test3() {
    cout << "\n==========================================================";
    cout << "\nТЕСТ 3: Сложный граф" << endl;

    ApplicationGraph app;
    AgentGraph agents;
    
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<double> dist(0.0, 3.0);
    for (int i = 0; i < 7; i++) {
        app.addModule(i, dist(gen));
    }

    app.addDependency(0, 1);
    app.addDependency(0, 2);
    app.addDependency(1, 3);
    app.addDependency(1, 4);
    app.addDependency(2, 4);
    app.addDependency(2, 5);
    app.addDependency(3, 6);
    app.addDependency(4, 6);
    app.addDependency(5, 6);

    for (int i = 0; i < 4; i++) {
        agents.addAgent(i);
    }

    for (int i = 1; i < 4; i++) {
        agents.addConnection(0, i);
    }

    LoadBalancer balancer;
    balancer.initializeGraphs(app, agents);
    balancer.initialDistribution();
    balancer.executeApplication();
}

int main() {
    setlocale(LC_ALL, "Russian");
    test1();
    test2();
    test3();

}
