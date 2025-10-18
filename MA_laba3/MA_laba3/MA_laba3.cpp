#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
using namespace std;

int n = 100;
struct Court
{
	double len = 23.77/2;
	double width = 8.23;
	double cell_x = len / n; // Размеры ячеек которые делят поле на nxn матрицу
	double cell_y = width / n;
} c;
struct Agent
{
	//Начальные значения
	double pos_x = 0; //Длина
	double pos_y = c.width / 2; //Ширина
	double r;
	double l;
	int ball = 0; // Если счет равный - играем до 2 мячей
	int score = 0;
	int set = 0;
	bool winner = false;
} a;
struct Bot
{
	double pos_x = c.len / 2;
	double pos_y = c.width / 2;
	double r;
	double l;
	int ball = 0;
	int score = 0;
	int set = 0;
	bool winner = false;
} b;

bool setIsDone() {
	if (a.score > 40 && b.score < 40) { //Agent
		a.set++;
		a.score = 0;
		a.ball = 0;
		b.score = 0;
		b.ball = 0;
		return false;
	}
	else if (a.score < 40 && b.score > 40) { //Bot
		b.set++;
		a.score = 0;
		a.ball = 0;
		b.score = 0;
		b.ball = 0;
		return false;
	}
	if (a.score == 40 && b.score == 40) {
		if (a.ball - b.ball >= 2) { //Agent
			a.set++;
			a.score = 0;
			a.ball = 0;
			b.score = 0;
			b.ball = 0;
			return false;
		}
		else if (b.ball - a.ball >= 2) { //Bot
			b.set++;
			a.score = 0;
			a.ball = 0;
			b.score = 0;
			b.ball = 0;
			return false;
		}
	}
	return true;
}

void movePlayer(double x, double y, string player) {
	if (player == "agent") {
		double dx = x - a.pos_x;
		double dy = y - a.pos_y;
		double dist = sqrt(dx * dx + dy * dy);
		if (dist < a.l) {
			a.pos_x = x;
			b.pos_y = y;
		}
		else {
			double scale = dist / a.l;
			a.pos_x += dx * scale;
			a.pos_y += dy * scale;
		}
	}
	else {
		double dx = x - b.pos_x;
		double dy = y - b.pos_y;
		double dist = sqrt(dx * dx + dy * dy);
		if (dist < b.l) {
			b.pos_x = x;
			b.pos_y = y;
		}
		else {
			double scale = dist / a.l;
			b.pos_x += dx * scale;
			b.pos_y += dy * scale;
		}
	}
}

bool hit(double x,double y,string player) {
	if (player == "agent") {//Бьет агент
		movePlayer(x, y,"bot"); // Во время удара противник двигается в точку падения мяча
		double dx = x - b.pos_x;
		double dy = y - b.pos_y;
		double dist = sqrt(dx * dx + dy * dy);
		if (dist <= b.r) {
			//cout << "Bot otbil, dist= "<<dist<<'\n';
			return false; //Отбил
		}
		//cout << "Agent get score, dist= " << dist << '\n';
		return true; // Не отбил
	}
	else { //Бьет бот
		movePlayer(x, y, "agent"); // Во время удара противник двигается в точку падения мяча
		double dx = x - a.pos_x;
		double dy = y - a.pos_y;
		double dist = sqrt(dx * dx + dy * dy);
		if (dist <= a.r && x >= a.pos_x) { //Проверка на то что мяч в полукруге
			//cout << "Agent otbil, dist = " << dist << '\n';
			return false; //Отбил
		}
		//cout << "Bot get score, dist = " << dist << '\n';
		return true; // Не отбил
	}
}

void addScore(string player) {
	if (a.score == 40 && b.score == 40) {
		if (player == "agent") {
			a.ball++;
		}
		else {
			b.ball++;
		}
	}
	else if (player == "agent") {
		if (a.score >= 30) {
			a.score += 10;
		}
		else {
			a.score += 15;
		}
	}
	else {
		if (b.score >= 30) {
			b.score += 10;
		}
		else {
			b.score += 15;
		}
	}
}
void hasWinner() {
	if (a.set == 2) a.winner = true;
	else if (b.set == 2) b.winner = true;
}
void clearPos() {
	a.pos_x = 0;
	a.pos_y = c.width / 2;
	b.pos_x = c.len / 2;
	b.pos_y = c.width / 2;
}
int main() {
	ofstream f("file.txt");
	srand(time(NULL));
	cout << "\nEnter l: "; cin >> a.l;
	b.l = a.l;

	for (double r = 1; r <= 10; r++) { // для разных r 
		b.r = a.r = r;
		a.r *= 2;
		int count = 0;
		for (int i = 0; i < 100; i++) { //100 раз тестируем
			a.winner = b.winner = false;
			a.set = b.set = 0;
			while (!a.winner && !b.winner) {
				while (setIsDone()) {
					bool game = true;
					int rand_x = rand() % 100 + 1;
					int rand_y = rand() % 100 + 1;
					double posx = rand_x * c.cell_x;
					double posy = rand_y * c.cell_y;
					if (hit(posx, posy, "agent")) {
						addScore("agent");
						//cout << a.score << " : " << b.score << '\n';
						//cout << a.set << " : " << b.set << '\n';
						game = false;
					}
					while (game) {
						int rand_x = rand() % 110 + 1; //Бот
						int rand_y = rand() % 110 + 1;
						int miss = rand() % 100;
						if (miss < 2) rand_x++;
						else if (miss < 3) rand_x--;
						else if (miss < 4) rand_y++;
						else if (miss < 5) rand_y--;
						if (rand_x > 100 || rand_y > 100) { //аут
							addScore("agent");
							//cout << "Bot hit at aut\n";
							//cout << a.score << " : " << b.score << '\n';
							//cout << a.set << " : " << b.set << '\n';
							break;
						}
						double posx = rand_x * c.cell_x;
						double posy = rand_y * c.cell_y;
						if (hit(posx, posy, "bot")) {
							addScore("bot");
							//cout << a.score << " : " << b.score << '\n';
							//cout << a.set << " : " << b.set << '\n';
							break;
						}

						rand_x = rand() % 110 + 1;//Агент
						rand_y = rand() % 110 + 1;
						miss = rand() % 100;
						if (miss < 2) rand_x++;
						else if (miss < 3) rand_x--;
						else if (miss < 4) rand_y++;
						else if (miss < 5) rand_y--;
						if (rand_x > 100 || rand_y > 100) { //аут
							addScore("bot");
							//cout << "Agent hit at aut\n";
							//cout << a.score << " : " << b.score << '\n';
							//cout << a.set << " : " << b.set << '\n';
							break;
						}
						posx = rand_x * c.cell_x;
						posy = rand_y * c.cell_y;
						if (hit(posx, posy, "agent")) {
							addScore("agent");
							//cout << a.score << " : " << b.score << '\n';
							//cout << a.set << " : " << b.set << '\n';
							break;
						}
					}
					clearPos();
				}
				//cout << "\n___________________________________\n";
				hasWinner();
			}
			if (a.winner) count++;
			string text = a.winner ? "Agent win" : "Bot win";
			//cout << text << '\n';
			//cout << a.set << " : " << b.set << '\n';
		}
		f << r << " " << count << " " << 100 - count <<'\n';
		cout << r << " " << count << " " << 100 - count << '\n';
	}
	
}