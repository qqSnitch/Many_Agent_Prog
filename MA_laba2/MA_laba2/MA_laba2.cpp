#include <iostream>
#include <vector>
#include <ctime>

using namespace std;

struct Agent {
	int id;
	vector<int> patents;
	int count_iter;
	bool complete = false;
};
int n, m;
vector<Agent> agents;
vector<vector<int>> all_patents;

void createPatents() {
	srand(time(NULL));
	all_patents.resize(n);
	for (int i = 0; i < n; i++) {
		all_patents[i].resize(m);
		for (int j = 0; j < m; j++) {
			all_patents[i][j] = i;
		}
	}
	for (int i = 0; i < 10; i++) {
		for (int i = n - 1; i >= 0; i--) {
			for (int j = m - 1; j >= 0; j--) {
				int r_row = rand() % n;
				int r_col = rand() % m;

				int temp = all_patents[i][j];
				all_patents[i][j] = all_patents[r_row][r_col];
				all_patents[r_row][r_col] = temp;
			}
		}
	}

}
bool allDone() {
	for (int i = 0; i < n; i++) {
		if (agents[i].complete != true) {
			return false;
		}
	}
	return true;
}
bool hasPatent(vector<int> patents, int id) {
	for (int j = 0; j < m; j++) {
		if (patents[j] == id) return true;
	}
	return false;
}
void trade(Agent a, Agent b) {
	int t_id1=0, t_id2=0;
	if (hasPatent(b.patents, a.id) && hasPatent(a.patents, b.id)) {
		for (int j = 0; j < m; j++) {
			if (b.patents[j] == a.id) {
				t_id1 = j;
			}
			if (a.patents[j] == b.id) {
				t_id2 = j;
			}
		}
		int temp = b.patents[t_id1];
		b.patents[t_id1] = a.patents[t_id2];
		a.patents[t_id2] = temp;
	}
	else if (hasPatent(b.patents, a.id)) {
		for (int j = 0; j < m; j++) {
			if (b.patents[j] == a.id) {
				t_id1 = j;
			}
			if (a.patents[j] != a.id) {
				t_id2 = j;
			}
		}
		int temp = b.patents[t_id1];
		b.patents[t_id1] = a.patents[t_id2];
		a.patents[t_id2] = temp;
	}
	else if (hasPatent(a.patents, b.id)) {
		for (int j = 0; j < m; j++) {
			if (a.patents[j] == b.id) {
				t_id1 = j;
			}
			if (b.patents[j] != b.id) {
				t_id2 = j;
			}
		}
		int temp = a.patents[t_id1];
		a.patents[t_id1] = b.patents[t_id2];
		b.patents[t_id2] = temp;
	}
	agents[a.id] = a;
	agents[b.id] = b;
	agents[a.id].count_iter++;
	agents[b.id].count_iter++;
}
void complete(int i) {
	agents[i].complete = true;
}
bool canTrade(Agent a,int id) {
	for (int j = 0; j < m; j++) {
		if (agents[id].patents[j] != id) {
			return true;
		}
	}
	complete(a.id);
	return false;
}
int main() {
	cout << "Enter count of agents: "; cin >> n;
	cout << "Enter count of patents for one agent: "; cin >> m;
	agents.resize(n);
	createPatents();
	for (int i = 0; i < n; i++) {
		agents[i].id = i;
		agents[i].patents.resize(m);
		agents[i].patents = all_patents[i];
	}

	for (int i = 0; i < n; i++) {
		cout << "\nID: " << agents[i].id << "\npatents : ";
		for (int j = 0; j < m; j++) {
			cout << agents[i].patents[j] << " ";
		}
		cout << '\n';
	}

	while (!allDone()) {
		for (int i = 0; i < n; i++) {
			for (int j = 0; j < n; j++) {
				if (canTrade(agents[i], agents[i].id) && i!=j) {
					trade(agents[i], agents[j]);
				}
			}
		}
	}
	cout << "\n________________________\n";
	for (int i = 0; i < n; i++) {
		cout << "\nID: " << agents[i].id << "\npatents : ";
		for (int j = 0; j < m; j++) {
			cout << agents[i].patents[j] << " ";
		}
		cout <<" count of iterations: " << agents[i].count_iter << '\n';
	}

}
