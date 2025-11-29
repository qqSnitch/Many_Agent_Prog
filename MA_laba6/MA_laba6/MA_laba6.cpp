#include<iostream>
#include<vector>
#include <random>
#include <algorithm>
using namespace std;

struct Card {
	int value; //Величина карты от 0 до 12
	int color; // Масть
	bool adm = false; // Флаг козыря
};

struct Agent {
	int id; //id игрока
	vector<Card> hand; //Карты в "руке"
	bool leader = false;
	bool done = false;
};

struct Table {
	vector<Card> beaten; //Бита
	vector<Card> atk; // Карты которые надо побить
	vector<Card> temp; //Побитые карты в этом раунде
};

struct Bank
{
	vector<Card>arr;
	int adm; //Козырь
};
int n;
vector<vector<Agent>> agents;
vector<vector<Card>> cards;
Table table;
Bank bank;

void init() {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 13; j++) {
			cards[i][j].value = j;
			cards[i][j].color = i;
		}
	}
	int iter = 0;
	for (int i = 0; i < 2; i++) { //Создаем агентов
		for (int j=0; j < 2; j++) {
			agents[i][j].id = iter++;
			if (j == 0) {
				agents[i][j].leader = true;
			}
		}
	}

	random_device rd;
	mt19937 gen(rd());
	vector<Card> arrCards; //Мешаем и раздаем карты
	for (const auto& row : cards) {
		arrCards.insert(arrCards.end(), row.begin(), row.end());
	}
	shuffle(arrCards.begin(), arrCards.end(), gen);
	bank.adm = arrCards.back().color; // Создаем козырь
	for (int i = 0; i < arrCards.size(); i++) {
		if (arrCards[i].color == bank.adm) {
			arrCards[i].adm = true;
		}

	}
	for (int j = 0; j < 6; j++) {
		agents[0][0].hand.push_back(arrCards.front());
		arrCards.erase(arrCards.begin());
		agents[0][1].hand.push_back(arrCards.front());
		arrCards.erase(arrCards.begin());
		agents[1][0].hand.push_back(arrCards.front());
		arrCards.erase(arrCards.begin());
		agents[1][1].hand.push_back(arrCards.front());
		arrCards.erase(arrCards.begin());
	}
	for (int i = 0; i < arrCards.size(); i++) {
		bank.arr.push_back(arrCards[i]);
	}
}
void deleteCard(int party, int id, int i) {
	Card temp = agents[party][id].hand[0];
	agents[party][id].hand[0] = agents[party][id].hand[i];
	agents[party][id].hand[i] = temp;
	agents[party][id].hand.erase(agents[party][id].hand.begin()); // Очень странная логика удаления карты из руки
}
bool hasWinner() {
	for (int i = 0; i < 2; i++) {
		if (agents[i][0].done == true && agents[i][1].done == true) return false;
	}
	return true;
}
void atack(int party, int atk_id) {
	if (party == 0) { // тактика первой команды - класть минимальную карту
		Card min_card = agents[party][atk_id].hand[0];
		int min_index = 0;
		bool flag = true;
		for (int i = 0; i < agents[party][atk_id].hand.size(); i++) {
			if (agents[party][atk_id].hand[i].value < min_card.value && !agents[party][atk_id].hand[i].adm) { //Сначала смотрим по не козырным картам
				min_card = agents[party][atk_id].hand[i];
				min_index = i;
				flag = false; // флаг выбора карты для атаки
			}
		}
		if (flag) {
			for (int i = 0; i < agents[party][atk_id].hand.size(); i++) { // Потом среди всех
				if (agents[party][atk_id].hand[i].value < min_card.value) {
					min_card = agents[party][atk_id].hand[i];
					min_index = i;
					
				}
			}
		}
		table.atk.push_back(min_card);
		deleteCard(party, atk_id, min_index);
	}
	else { // тактика второй команды - класть рандомную карту (тоже тактика)
		random_device rd;
		mt19937 gen(rd());
		uniform_int_distribution<> dis(0, agents[party][atk_id].hand.size()-1);
		int card = dis(gen);
		table.atk.push_back(agents[party][atk_id].hand[card]);
		deleteCard(party, atk_id, card);
	}
}
bool canBeat(int atk, bool adm_atk, int def,bool adm_def,int col_atk,int col_def) {
	if (def > atk && col_def == col_atk )
		return true;
	if (adm_def && !adm_atk)
		return true;
	return false;
}
bool deffend(int party, int def_id) {
	for (int i = 0; i < agents[party][def_id].hand.size(); i++) {
		for (int j = 0; j < table.atk.size(); j++) {
			if (canBeat(
					table.atk[j].value, 
					table.atk[j].adm, 
					agents[party][def_id].hand[i].value, 
					agents[party][def_id].hand[i].adm, 
					table.atk[j].color, agents[party][def_id].hand[i].color)) 
			{
				table.temp.push_back(table.atk[j]);
				table.temp.push_back(agents[party][def_id].hand[i]);
				Card temp = table.atk[0];
				table.atk[0] = table.atk[j];
				table.atk[j] = temp;
				table.atk.erase(table.atk.begin());
				deleteCard(party, def_id, i);
			}
		}
	}
	if (table.atk.size() > 0) {
		for (int j = 0; j < table.atk.size(); j++) {
			agents[party][def_id].hand.push_back(table.atk[j]);
		}
		table.atk.clear();
		for (int j = 0; j < table.temp.size(); j++) {
			agents[party][def_id].hand.push_back(table.temp[j]);
		}
		table.temp.clear();
		return false; //Не смог отбить
	}
	for (int j = 0; j < table.temp.size(); j++) {
		table.beaten.push_back(table.temp[j]);
	}
	table.temp.clear();
	return true;
	
}
bool addToTable(int party, int atk_id) {
	bool flag = false;
	for (int i = 0; i < agents[party][atk_id].hand.size(); i++) {
		for (int j = 0; j < table.temp.size(); j++) {
			if (agents[party][atk_id].hand[i].value == table.temp[j].value) {
				table.atk.push_back(agents[party][atk_id].hand[i]);
				deleteCard(party, atk_id, i);
				flag = true;
				break;
			}
		}
		if (flag == false) {
			atk_id = 1 - atk_id;
			for (int j = 0; j < table.temp.size(); j++) {
				if (agents[party][atk_id].hand[i].value == table.temp[j].value) {
					table.atk.push_back(agents[party][atk_id].hand[i]);
					deleteCard(party, atk_id, i);
					flag = true;
					break;
				}
			}
		}
		if (flag) break;
	}
	return flag;
}
bool play(int party_atk,int party_def,int atk_id,int def_id) {
	if (agents[party_atk][atk_id].done) atk_id = 1 - atk_id;
	if (agents[party_def][def_id].done) def_id = 1 - def_id;
	atack(party_atk, atk_id);
	bool def = true;
	int iter = 0;
	while(iter < 6) {
		def = deffend(party_def, def_id);
		if (!def) {
			break;
		}
		if (!addToTable(party_atk, atk_id)) break;
		iter++;
	}
	return def;
}
void addToHand() {
	if (bank.arr.size() > 0) {
		for (int i = 0; i < agents.size(); i++) {
			for (int j = 0; j < agents[i].size(); j++) {
				while (agents[i][j].hand.size() < 6)
				{
					agents[i][j].hand.push_back(bank.arr[0]);
					bank.arr.erase(bank.arr.begin());
				}
			}
		}
	}
}
void check() {
	for (int i = 0; i < agents.size(); i++) {
		for (int j = 0; j < agents[i].size(); j++) {
			if (agents[i][j].hand.size() == 0) {
				//cout << "Agent " << i << " " << j << '\n';
				agents[i][j].done = true;
			}
			
		}
	}
}
int main() {
	int win1 = 0, win2=0;
	for(int i=0;i<1000;i++)
	{
		agents.resize(2);
		cards.resize(4);
		for (int i = 0; i < 4; i++) {
			cards[i].resize(13);
		}
		for (int i = 0; i < 2; i++) {
			agents[i].resize(2);
		}
		init();
		int atk_party = 0;
		int def_party = 1;
		int atk_id = 0;
		int def_id = 0;
		int iter = 0;
		while (hasWinner()) {
			//cout << iter << " " << agents[atk_party][atk_id].done << '\n';
			if (play(atk_party, def_party, atk_id, def_id)) {
				atk_party = 1 - atk_party;
				def_party = 1 - def_party;
				swap(atk_id, def_id);
			}
			addToHand();
			check();
			if (agents[atk_party][atk_id].done) {
				atk_id = 1 - atk_id;
			}
			if (agents[def_party][atk_id].done) {
				atk_id = 1 - atk_id;
			}
			iter++;
		}
		if (agents[0][0].done && agents[0][1].done) {
			cout << "Team 1 win\n";
			win1++;
		}
		else {
			cout << "Team 2 win\n";
			win2++;
		}
		for (int i = 0; i < agents.size(); i++) {
			agents[i].clear();
		}
		agents.clear();
		cards.clear();
		bank.arr.clear();
		table.beaten.clear();
		table.atk.clear();
		table.temp.clear();
	}
	cout <<'\n' << win1 << " : " << win2;
}