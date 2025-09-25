#include <iostream>
#include <vector>
#include <ctime>

using namespace std;

struct Agent {
	int id;
	vector<int> patents;
	int count_iter;
};
int n, m;
vector<Agent> agents;
vector<vector<int>> all_patents;

void createPatents(int n, int m) {
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

int main() {
	cout << "Enter count of agents: "; cin >> n;
	cout << "Enter count of patents for one agent: "; cin >> m;
	agents.resize(n);
	createPatents(n,m);
	for (int i = 0; i < n; i++) {
		agents[i].id = i;
		agents[i].patents.resize(m);
	}
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < m; j++) {
			cout << all_patents[i][j] << " ";
		}
		cout << '\n';
	}

}