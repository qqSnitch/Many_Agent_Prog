#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <random>
#include <algorithm>
#include <fstream>
#include <memory>
#include <cmath>

struct Colony {
    int id;
    int level;
    double balance;
    double income;
    double expense;
    double experience;
    double prev_balance;
    bool alive;
    bool won;

    // Активные эффекты
    std::map<std::string, std::vector<std::pair<double, int>>> active_effects;
};

struct ArtifactEffect {
    std::string type; // "income_%", "expense_abs", "level", "balance_%", "experience", "mythic"
    double value;
    int duration_type; // 0: циклы, 1: итерации, 2: до события, 3: единоразово
    int duration;
    int extra_param;
};

struct Artifact {
    int id;
    std::vector<ArtifactEffect> effects;
    double base_price;
};

struct AuctionResult {
    int winner_id;
    double price;
    Artifact artifact;
};

struct SimulationParams {
    int T; // Максимальное время
    int t; // Итераций в цикле
    double B; // Начальный баланс
    double L; // Константа опыта
    int t_a; // Период аукционов
    int t_e; // Период событий среды
    double g; // Параметр бури
    double j; // Параметр бури
    int num_colonies;
    int num_artifacts;
    int experiments;
};

std::vector<Artifact> generate_artifacts(int count) {
    std::vector<Artifact> artifacts;
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<> val_dist(0.1, 0.5);
    std::uniform_int_distribution<> dur_dist(1, 5);

    for (int i = 0; i < count; ++i) {
        Artifact art;
        art.id = i;
        art.base_price = 50 + i * 10;

        ArtifactEffect eff;
        eff.type = "income_%";
        eff.value = val_dist(rng) * 100;
        eff.duration_type = 0;
        eff.duration = dur_dist(rng);
        art.effects.push_back(eff);

        artifacts.push_back(art);
    }
    return artifacts;
}

class Agent {
private:
    std::mt19937 rng;

public:
    Agent() : rng(std::random_device{}()) {}

    double calculate_bid(const Colony& colony, const Artifact& artifact) {
        // Оценка полезности артефакта
        double utility = 0;

        for (const auto& eff : artifact.effects) {
            double effect_value = 0;

            if (eff.type.find("income") != std::string::npos) {
                effect_value = colony.income * eff.value / 100.0 * eff.duration;
            }
            else if (eff.type.find("expense") != std::string::npos) {
                effect_value = colony.expense * eff.value / 100.0 * eff.duration;
            }
            else if (eff.type.find("balance") != std::string::npos) {
                effect_value = colony.balance * eff.value / 100.0;
            }
            else if (eff.type == "level") {
                effect_value = 1000 * eff.value; // Высокая ценность уровня
            }

            utility += effect_value;
        }

        // Учет риска
        double risk_factor = 1.0;
        double future_balance = colony.balance - artifact.base_price;

        // Если покупка может привести к поражению
        double min_safe_balance = colony.expense * 3; // Минимальный безопасный баланс
        if (future_balance < min_safe_balance) {
            risk_factor = std::max(0.1, future_balance / min_safe_balance);
        }

        // Расчет ставки
        double max_bid = colony.balance * 0.7; // Не более 70% баланса
        double bid = std::min(utility * risk_factor * 0.01, max_bid);

        // Гарантированная минимальная ставка
        bid = std::max(bid, artifact.base_price * 0.5);

        return bid;
    }
};

class Simulator {
private:
    SimulationParams params;
    std::vector<Artifact> artifacts;
    std::mt19937 rng;

public:
    Simulator(const SimulationParams& p) : params(p), rng(std::random_device{}()) {
        artifacts = generate_artifacts(params.num_artifacts);
    }

    void initialize_colonies(std::vector<Colony>& colonies) {
        std::uniform_real_distribution<> inc_dist(100, 200);
        std::uniform_real_distribution<> exp_dist(50, 150);

        for (int i = 0; i < params.num_colonies; ++i) {
            Colony col;
            col.id = i;
            col.level = 1;
            col.balance = params.B;
            col.income = inc_dist(rng);
            col.expense = exp_dist(rng);
            // Гарантируем доход > расход
            if (col.income <= col.expense) {
                col.income = col.expense * 1.2;
            }
            col.experience = 0;
            col.prev_balance = params.B;
            col.alive = true;
            col.won = false;
            colonies.push_back(col);
        }
    }

    void apply_artifact(Colony& colony, const Artifact& artifact) {
        for (size_t i = 0; i < artifact.effects.size(); ++i) {
            const ArtifactEffect& eff = artifact.effects[i];

            if (eff.type == "income_%_from_income") {
                // Текущий доход + {n}% от дохода
                double bonus = colony.income * eff.value / 100.0;
                colony.income += bonus;

                if (eff.duration_type == 0) { // циклы
                    colony.active_effects["income"].push_back(std::make_pair(-bonus, eff.duration));
                }
                else if (eff.duration_type == 3) { // единоразово
                    // ничего не делаем, эффект постоянный
                }
            }
            else if (eff.type == "expense_%_from_expense") {
                // Текущий расход - {n}% от расходов
                double reduction = colony.expense * eff.value / 100.0;
                colony.expense -= reduction;

                if (eff.duration_type == 0) { // циклы
                    colony.active_effects["expense"].push_back(std::make_pair(reduction, eff.duration));
                }
                else if (eff.duration_type == 3) { // единоразово
                    // ничего не делаем, эффект постоянный
                }
            }
            else if (eff.type == "income_abs") {
                // Текущий доход + {n} единиц условной валюты
                colony.income += eff.value;

                if (eff.duration_type == 0 || eff.duration_type == 1) { // циклы или итерации
                    colony.active_effects["income"].push_back(std::make_pair(-eff.value, eff.duration));
                }
            }
            else if (eff.type == "expense_abs") {
                // Текущий расход - {n} единиц условной валюты
                colony.expense = std::max(0.0, colony.expense - eff.value);

                if (eff.duration_type == 0 || eff.duration_type == 1) { // циклы или итерации
                    colony.active_effects["expense"].push_back(std::make_pair(eff.value, eff.duration));
                }
            }
            else if (eff.type == "level_up") {
                // + {n} уровней (без учета длительности, единоразово)
                colony.level = std::min(10, colony.level + static_cast<int>(eff.value));
            }
            else if (eff.type == "balance_%_from_income") {
                // Текущий баланс + {n}% от дохода
                double bonus = colony.income * eff.value / 100.0;
                colony.balance += bonus;

                if (eff.duration_type == 2) { // до следующего события среды
                    colony.active_effects["balance_event"].push_back(std::make_pair(0, 1)); // маркер
                }
            }
            else if (eff.type == "balance_%_from_balance") {
                // Текущий баланс + {n}% от баланса
                double bonus = colony.balance * eff.value / 100.0;
                colony.balance += bonus;

                if (eff.duration_type == 0 || eff.duration_type == 1) { // циклы или итерации
                    // для баланса обычно единоразово, но если указана длительность, 
                    // нужно реализовать периодические выплаты
                    if (eff.duration > 1) {
                        colony.active_effects["balance_periodic"].push_back(
                            std::make_pair(bonus / eff.duration, eff.duration));
                    }
                }
            }
            else if (eff.type == "experience_abs") {
                // + {n} единиц опыта
                colony.experience += eff.value;

                if (eff.duration_type == 0 || eff.duration_type == 1) { // циклы или итерации
                    colony.active_effects["experience"].push_back(
                        std::make_pair(eff.value, eff.duration));
                }
            }
            else if (eff.type == "experience_%_from_current") {
                // + {n}% к опыту от текущего опыта
                double bonus = colony.experience * eff.value / 100.0;
                colony.experience += bonus;

                if (eff.duration_type == 2) { // до события среды
                    colony.active_effects["exp_event"].push_back(std::make_pair(0, 1));
                }
                else if (eff.duration_type == 3) { // единоразово
                    // уже применено
                }
            }
            else if (eff.type == "experience_%_from_max") {
                // + {n}% к опыту от максимального опыта уровня
                double max_exp_for_level = params.L; // L - опыт для нового уровня
                double bonus = max_exp_for_level * eff.value / 100.0;
                colony.experience += bonus;
            }
            else if (eff.type == "income_%_from_balance") {
                // Текущий доход + {n}% от баланса
                double bonus = colony.balance * eff.value / 100.0;
                colony.income += bonus;

                if (eff.duration_type == 0 || eff.duration_type == 1) { // циклы или итерации
                    colony.active_effects["income"].push_back(std::make_pair(-bonus, eff.duration));
                }
            }
            else if (eff.type == "expense_%_from_balance") {
                // Текущий расход - {n}% от баланса
                double reduction = colony.balance * eff.value / 100.0;
                colony.expense = std::max(0.0, colony.expense - reduction);

                if (eff.duration_type == 0 || eff.duration_type == 1) { // циклы или итерации
                    colony.active_effects["expense"].push_back(std::make_pair(reduction, eff.duration));
                }
            }
            else if (eff.type == "income_%_from_expense") {
                // Текущий доход + {n}% от расходов
                double bonus = colony.expense * eff.value / 100.0;
                colony.income += bonus;

                if (eff.duration_type == 0 || eff.duration_type == 1) { // циклы или итерации
                    colony.active_effects["income"].push_back(std::make_pair(-bonus, eff.duration));
                }
                else if (eff.duration_type == 2) { // до аукциона
                    colony.active_effects["income_auction"].push_back(std::make_pair(0, 1));
                }
            }
            else if (eff.type == "balance_%_from_expense") {
                // Текущий баланс + {n}% от расходов
                double bonus = colony.expense * eff.value / 100.0;
                colony.balance += bonus;

                if (eff.duration_type == 2) { // до аукциона
                    colony.active_effects["balance_auction"].push_back(std::make_pair(0, 1));
                }
                else if (eff.duration_type == 3) { // единоразово
                    // уже применено
                }
            }
            else if (eff.type == "expense_zero") {
                // Обнуление расхода
                double old_expense = colony.expense;
                colony.expense = 0;

                if (eff.duration_type == 0 || eff.duration_type == 1) { // циклы или итерации
                    colony.active_effects["expense"].push_back(std::make_pair(old_expense, eff.duration));
                }
                else if (eff.duration_type == 3) { // единоразово -> до события среды/аукциона
                    colony.active_effects["expense_event"].push_back(std::make_pair(old_expense, 1));
                }
            }
            else if (eff.type == "income_double") {
                // Текущий доход x2
                double old_income = colony.income;
                colony.income *= 2.0;

                if (eff.duration_type == 0 || eff.duration_type == 1) { // циклы или итерации
                    colony.active_effects["income"].push_back(std::make_pair(-old_income, eff.duration));
                }
                else if (eff.duration_type == 3) { // единоразово -> до события среды/аукциона
                    colony.active_effects["income_event"].push_back(std::make_pair(-old_income, 1));
                }
            }
            else if (eff.type == "balance_double") {
                // Текущий баланс x2
                colony.balance *= 2.0;

                if (eff.duration_type == 0 || eff.duration_type == 1) { // циклы или итерации
                    // для баланса периодическое удвоение не имеет смысла
                    // обрабатываем как единоразовое
                }
                else if (eff.duration_type == 2) { // до события среды
                    colony.active_effects["balance_double_event"].push_back(std::make_pair(0, 1));
                }
                else if (eff.duration_type == 3) { // единоразово
                    // уже применено
                }
            }
      
        }
    }

    void update_colony(Colony& colony, int cycle, bool event_occurred, bool is_storm) {
        if (!colony.alive || colony.won) return;

        // Обновление баланса
        colony.prev_balance = colony.balance;
        colony.balance += colony.income - colony.expense;

        // Обновление опыта
        double exp_gain = colony.balance - colony.prev_balance;
        colony.experience = std::max(0.0, colony.experience + exp_gain);

        // Проверка уровня
        if (colony.experience >= params.L) {
            colony.level++;
            colony.experience = 0;
            if (colony.level >= 10) {
                colony.won = true;
                return;
            }
        }

        // Проверка поражения
        if (colony.balance < 0) {
            colony.alive = false;
            return;
        }

        // Обработка событий среды
        if (event_occurred) {
            if (is_storm) {
                colony.income -= params.g;
                colony.expense += params.j;
            }
            else {
                colony.income += params.g;
                colony.expense -= params.j;
            }
        }

        // Обновление длительности эффектов
        for (auto& [type, effects] : colony.active_effects) {
            for (size_t i = 0; i < effects.size(); ) {
                effects[i].second--;
                if (effects[i].second <= 0) {
                    // Откат эффекта
                    if (type == "income") {
                        colony.income += effects[i].first;
                    }
                    effects.erase(effects.begin() + i);
                }
                else {
                    ++i;
                }
            }
        }
    }

    AuctionResult conduct_auction(const std::vector<Colony>& colonies,
        const Artifact& artifact,
        int cycle) {
        Agent agent;
        std::vector<std::pair<int, double>> bids;

        for (const auto& col : colonies) {
            if (col.alive && !col.won) {
                double bid = agent.calculate_bid(col, artifact);
                if (bid > 0) {
                    bids.push_back({ col.id, bid });
                }
            }
        }

        AuctionResult result;
        result.artifact = artifact;

        if (bids.empty()) {
            result.winner_id = -1;
            result.price = 0;
        }
        else {
            // Аукцион второй цены
            std::sort(bids.begin(), bids.end(),
                [](auto& a, auto& b) { return a.second > b.second; });

            result.winner_id = bids[0].first;
            result.price = bids.size() > 1 ? bids[1].second : bids[0].second * 0.8;
        }

        return result;
    }

    void run_experiment(int exp_id, std::vector<double>& lifetimes,
        std::vector<int>& results) {
        std::vector<Colony> colonies;
        initialize_colonies(colonies);

        std::uniform_int_distribution<> event_dist(0, 1);

        for (int cycle = 0; cycle < params.T; ++cycle) {
            // Событие среды
            bool event_occurred = (cycle % params.t_e == 0 && cycle > 0);
            bool is_storm = event_dist(rng) == 0;

            // Аукцион
            if (cycle % params.t_a == 0 && cycle > 0) {
                std::uniform_int_distribution<> art_dist(0, artifacts.size() - 1);
                Artifact art = artifacts[art_dist(rng)];
                AuctionResult auction = conduct_auction(colonies, art, cycle);

                if (auction.winner_id != -1) {
                    for (auto& col : colonies) {
                        if (col.id == auction.winner_id && col.balance >= auction.price) {
                            col.balance -= auction.price;
                            apply_artifact(col, auction.artifact);
                            break;
                        }
                    }
                }
            }

            // Обновление колоний
            for (auto& col : colonies) {
                update_colony(col, cycle, event_occurred, is_storm);

                // Запись времени жизни для уничтоженных колоний
                if (!col.alive && col.balance < 0) {
                    lifetimes.push_back(cycle);
                }
            }

            // Проверка окончания эксперимента
            int alive_count = 0;
            for (const auto& col : colonies) {
                if (col.alive && !col.won) alive_count++;
            }
            if (alive_count == 0) break;
        }

        // Сбор результатов
        int wins = 0, losses = 0;
        for (const auto& col : colonies) {
            if (col.won) wins++;
            if (!col.alive) losses++;
        }
        results.push_back(wins);
        results.push_back(losses);
    }

    void run_multiple_experiments() {
        std::vector<double> all_lifetimes;
        std::vector<int> experiment_results;

        for (int exp = 0; exp < params.experiments; ++exp) {
            std::vector<double> exp_lifetimes;
            std::vector<int> exp_results;
            run_experiment(exp, exp_lifetimes, exp_results);

            all_lifetimes.insert(all_lifetimes.end(),
                exp_lifetimes.begin(), exp_lifetimes.end());
            experiment_results.insert(experiment_results.end(),
                exp_results.begin(), exp_results.end());
        }

        // Вывод статистики
        std::cout << "Экспериментов: " << params.experiments << std::endl;

        // Статистика по времени жизни
        if (!all_lifetimes.empty()) {
            double avg_lifetime = 0;
            for (double lt : all_lifetimes) avg_lifetime += lt;
            avg_lifetime /= all_lifetimes.size();

            std::cout << "Среднее время жизни: " << avg_lifetime << std::endl;

            // Распределение времени жизни
            std::map<int, int> lifetime_dist;
            for (double lt : all_lifetimes) {
                int bucket = static_cast<int>(lt / 10) * 10;
                lifetime_dist[bucket]++;
            }

            // Запись в файл для построения графика
            std::ofstream file("lifetime_distribution.csv");
            file << "Время,Частота\n";
            for (const auto& [time, count] : lifetime_dist) {
                file << time << "," << count << "\n";
            }
            file.close();
        }

        // Статистика побед/поражений
        int total_wins = 0, total_losses = 0;
        for (size_t i = 0; i < experiment_results.size(); i += 2) {
            total_wins += experiment_results[i];
            total_losses += experiment_results[i + 1];
        }

        std::cout << "Всего побед: " << total_wins << std::endl;
        std::cout << "Всего поражений: " << total_losses << std::endl;

        // Запись зависимости от параметров
        std::ofstream param_file("parameter_dependence.txt", std::ios::app);
        param_file << params.j << " " << total_wins << " " << total_losses<<"\n";
        param_file.close();
    }
};

int main() {
    setlocale(LC_ALL, "Russian");
    SimulationParams params;
    params.T = 100; // циклов
    params.t = 10; // итераций в цикле
    params.B = 1000; // начальный баланс
    params.L = 500; // опыт для уровня
    params.t_a = 5; // аукцион каждые 5 циклов
    params.t_e = 8; // событие среды каждые 8 циклов
    params.g = 50; // изменение дохода при событиях
    params.j = 30; // изменение расхода при событиях
    params.num_colonies = 10;
    params.num_artifacts = 100;
    params.experiments = 100;
    for (int g = 10; g <= 100; g += 10) {
        std::cout << "Результаты для g = " << g<<"\n";
        params.j = g; // изменение дохода при событиях
        Simulator simulator(params);
        simulator.run_multiple_experiments();
        std::cout << "\n";
    }
}