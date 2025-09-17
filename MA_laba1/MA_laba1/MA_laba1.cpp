#include <iostream>
#include <queue>
#include <vector>
#include <ctime>
using namespace std;
struct Agent
{
    int id;
    queue<int> clients; // Первый человек в очереди обрабатывается, остальные ждут
    int count_clients = 0;
    int all_time = 0; //время клиентов за все время работы (только увеличивается)
    int count_time = 0; //время всех в текущей очереди
};
int n,m, all_clients = 0;
vector <Agent> a (n);
vector<Agent> addNewClientToAgent(int diff, vector<Agent> agents) {
    vector<int> time(n);
    for (int i = 0; i < n; i++) {
        time[i] = agents[i].count_time;// Считываем текущее время
    }
    int min = time[0], min_index = 0;
    for (int i = 0; i < n; i++) {
        if (min > time[i]) {
            min = time[i];
            min_index = i;
        }
    }
    agents[min_index].clients.push(diff);
    agents[min_index].all_time += diff;
    agents[min_index].count_time += diff;
    agents[min_index].count_clients ++;
    all_clients++;
    return agents;
}
vector<Agent> hasNewClient(vector<Agent> agents) {
    int a = rand() % 10 + 1;
    if (a >= 6) { // Определяем есть ли новый клиент с помощью псевдорандома
        int difficult = rand() % 10 + 1; //Сложность клиента
        agents = addNewClientToAgent(difficult, agents);
    }
    return agents;
}
bool allDone(vector<Agent> agents) {
    for (int i = 0; i < n; i++) {// Если есть хотя бы 1 агент, который не закончил работу возращаем true
        if (agents[i].count_time != 0) {
            return true;
        }
    }
    return false;
}
vector<Agent> sortAgents(vector <Agent> a) {
    for (int i = 0; i < n - 1; i++) { //По убыванию клиентов
        for (int j = 0; j < n - i - 1; j++) {
            if (a[j].count_clients < a[j + 1].count_clients) {
                Agent temp = a[j];
                a[j] = a[j + 1];
                a[j + 1] = temp;
            }
        }
    }
    bool flag = true;
    while (flag) {
        flag = false;
        for (int i = 0; i < n-1; i++) { //По возрастанию времени если кол-во клиентов ==
            if (a[i].count_clients == a[i + 1].count_clients) {
                if (a[i].all_time > a[i + 1].all_time) {
                    Agent temp = a[i];
                    a[i] = a[i + 1];
                    a[i + 1] = temp;
                    flag = true;
                }
            }
        }
    }
    return a;
}
void print(vector <Agent> a) {
    a = sortAgents(a);
    for (int i = 0; i < n; i++) {
        cout << "\nID: " << a[i].id << "; count of clients: " << a[i].count_clients << "; time: " << a[i].all_time;
    }
}
int main()
{
    srand(time(NULL));
    cout << "Enter count of agents: ";
    cin >> n;
    a.resize(n);
    for (int i = 0; i < n; i++) {
        a[i].id = i;
    }
    cout << "Enter max count of clients: ";
    cin >> m;
    int counts = 0 ,t = 0;

    while (all_clients < m) { //Каждый проход while - 1 еденица времени
        a = hasNewClient(a);
        for (int i = 0; i < n; i++) {
            if (a[i].count_time == 0) continue;
            a[i].clients.front()--;
            a[i].count_time--;
        }
            //print(a);
            //cout << "\n______________________________";
    }
    while (allDone(a)) {
        for (int i = 0; i < n; i++) {
            if (a[i].count_time == 0) continue;
            if (a[i].clients.front() == 0) a[i].clients.pop();
            a[i].clients.front()--;
            a[i].count_time--;
        }
            //print(a);
            //cout << "\n------------------------------";
    }
    print(a);
}
