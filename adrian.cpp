#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <climits>

using namespace std;

const int SZEROKOSC = 60;
const int WYSOKOSC = 10;
const int RAMKA_X = 12;
const int RAMKA_Y = 10;
const int PRZEWIDYWANIE_TUR = 10;  // Zwiększona liczba tur do przewidywania kolizji

class Plane {
public:
	int x, y;
	char oznaczenie;
	bool czyLeci;
	bool kierunek;
	int liczbaPolKomendy;
	char znakKomendy;
	bool aktywowanaKomenda;

	Plane(int startY, bool startKierunek, char litera)
		: y(startY), kierunek(startKierunek), oznaczenie(litera), czyLeci(true),
		  znakKomendy('='), liczbaPolKomendy(0), aktywowanaKomenda(true) {
		x = (kierunek ? -1 : RAMKA_X);
	}

	void przesun() {
		x += (kierunek ? 1 : -1);

		if (!aktywowanaKomenda) {
			aktywowanaKomenda = true;
			return;
		}

		if (znakKomendy == '/' && liczbaPolKomendy > 0) {
			y = max(0, y - 1);
			liczbaPolKomendy--;
		}
		else if (znakKomendy == '\\' && liczbaPolKomendy > 0) {
			y = min(RAMKA_Y - 1, y + 1);
			liczbaPolKomendy--;
		}

		if (liczbaPolKomendy == 0 && (znakKomendy == '/' || znakKomendy == '\\')) {
			znakKomendy = '=';
		}
	}

	bool pozaPlansza() const {
		return (x < -1 || x > RAMKA_X || y < 0 || y >= RAMKA_Y);
	}
};

bool sprawdzKolizje(const vector<Plane>& samoloty) {
    for (size_t i = 0; i < samoloty.size(); ++i) {
        for (size_t j = i + 1; j < samoloty.size(); ++j) {
            int dx = abs(samoloty[i].x - samoloty[j].x);
            int dy = abs(samoloty[i].y - samoloty[j].y);
            if (dx < 3 && dy <= 2) {  // Zmienione z "dy < 3" na "dy <= 2" dla szybszego wykrywania kolizji
                return true;
            }
        }
    }
    return false;
}

string wydajKomende(vector<Plane>& samoloty) {
    struct Scenariusz {
        string komenda;
        bool bezpieczny;
        int minOdleglosc;
        int zmianaWysokosci;
    };

    vector<Scenariusz> mozliweScenariusze;

    for (auto& samolot : samoloty) {
        if (!samolot.czyLeci || samolot.znakKomendy != '=') continue;

        // Scenariusz: brak komendy
        int minDist = INT_MAX;
        vector<Plane> tmp = samoloty;
        for (int t = 0; t < PRZEWIDYWANIE_TUR; ++t) {
            for (auto& p : tmp) p.przesun();
            for (size_t i = 0; i < tmp.size(); ++i) {
                for (size_t j = i + 1; j < tmp.size(); ++j) {
                    int dx = abs(tmp[i].x - tmp[j].x);
                    int dy = abs(tmp[i].y - tmp[j].y);
                    minDist = min(minDist, max(dx, dy));
                }
            }
        }
        mozliweScenariusze.push_back({"Spacja", minDist >= 3, minDist, 0});

        // Scenariusz: wznoszenie
        if (samolot.y > 0) {
            int maxWznoszenie = min(2, samolot.y);
            if (maxWznoszenie > 0) {
                auto kop = samoloty;
                int idx = &samolot - &samoloty[0];
                kop[idx].znakKomendy = '/';
                kop[idx].liczbaPolKomendy = maxWznoszenie;
                kop[idx].aktywowanaKomenda = false;

                int minDist = INT_MAX;
                vector<Plane> tmp = kop;
                for (int t = 0; t < PRZEWIDYWANIE_TUR; ++t) {
                    for (auto& p : tmp) p.przesun();
                    for (size_t i = 0; i < tmp.size(); ++i) {
                        for (size_t j = i + 1; j < tmp.size(); ++j) {
                            int dx = abs(tmp[i].x - tmp[j].x);
                            int dy = abs(tmp[i].y - tmp[j].y);
                            minDist = min(minDist, max(dx, dy));
                        }
                    }
                }
                mozliweScenariusze.push_back({
                    string(1, samolot.oznaczenie) + " / " + to_string(maxWznoszenie),
                    minDist >= 3,
                    minDist,
                    maxWznoszenie
                });
            }
        }

        // Scenariusz: opadanie
        if (samolot.y < RAMKA_Y - 1) {
            int maxOpadanie = min(2, RAMKA_Y - 1 - samolot.y);
            if (maxOpadanie > 0) {
                auto kop = samoloty;
                int idx = &samolot - &samoloty[0];
                kop[idx].znakKomendy = '\\';
                kop[idx].liczbaPolKomendy = maxOpadanie;
                kop[idx].aktywowanaKomenda = false;

                int minDist = INT_MAX;
                vector<Plane> tmp = kop;
                for (int t = 0; t < PRZEWIDYWANIE_TUR; ++t) {
                    for (auto& p : tmp) p.przesun();
                    for (size_t i = 0; i < tmp.size(); ++i) {
                        for (size_t j = i + 1; j < tmp.size(); ++j) {
                            int dx = abs(tmp[i].x - tmp[j].x);
                            int dy = abs(tmp[i].y - tmp[j].y);
                            minDist = min(minDist, max(dx, dy));
                        }
                    }
                }
                mozliweScenariusze.push_back({
                    string(1, samolot.oznaczenie) + " \\ " + to_string(maxOpadanie),
                    minDist >= 3,
                    minDist,
                    maxOpadanie
                });
            }
        }
    }

    sort(mozliweScenariusze.begin(), mozliweScenariusze.end(), [](const Scenariusz& a, const Scenariusz& b) {
        if (a.bezpieczny != b.bezpieczny) return a.bezpieczny > b.bezpieczny;
        if (a.minOdleglosc != b.minOdleglosc) return a.minOdleglosc > b.minOdleglosc;
        return a.zmianaWysokosci < b.zmianaWysokosci;
    });

    for (auto& scenariusz : mozliweScenariusze) {
        if (scenariusz.bezpieczny) {
            for (auto& samolot : samoloty) {
                if (samolot.oznaczenie == scenariusz.komenda[0]) {
                    if (scenariusz.komenda.find("/") != string::npos) {
                        samolot.znakKomendy = '/';
                    } else if (scenariusz.komenda.find("\\") != string::npos) {
                        samolot.znakKomendy = '\\';
                    }
                    size_t spacePos = scenariusz.komenda.rfind(" ");
                    samolot.liczbaPolKomendy = stoi(scenariusz.komenda.substr(spacePos + 1));
                    samolot.aktywowanaKomenda = false;
                    break;
                }
            }
            return scenariusz.komenda;
        }
    }

    return "Spacja";
}

bool generujNowySamolot(vector<Plane>& samoloty, int& literaIndex) {
    int liczbaPrawo = 0;
    int liczbaLewo = 0;
    for (auto& s : samoloty) {
        if (s.kierunek) liczbaPrawo++;
        else liczbaLewo++;
    }

    // Maksymalnie 4 samoloty, max 2 w jednym kierunku
    if (samoloty.size() >= 4) return false;
    if ((liczbaPrawo >= 2 && rand() % 2 == 1) || (liczbaLewo >= 2 && rand() % 2 == 0)) {
        return false;
    }

    // Jeśli jest już 3 samoloty, nowy pojawia się dopiero gdy istniejące są daleko od krawędzi
    if (samoloty.size() >= 3) {
        for (auto& s : samoloty) {
            if ((s.kierunek && s.x < 4) || (!s.kierunek && (RAMKA_X - s.x) < 4)) {
                return false;
            }
        }
    }

    // Wybór bezpiecznej pozycji dla nowego samolotu
    bool kierunek = rand() % 2;
    int attempts = 0;
    const int MAX_ATTEMPTS = 20;

    while (attempts++ < MAX_ATTEMPTS) {
        int y = rand() % RAMKA_Y;
        bool moznaDodac = true;

        for (auto& s : samoloty) {
            int dx = abs((kierunek ? -1 : RAMKA_X) - s.x);
            int dy = abs(y - s.y);

            if ((dy == 0 && dx < 8) || (dy <= 2 && dx < 6)) {
                moznaDodac = false;
                break;
            }
        }

        if (moznaDodac) {
            samoloty.emplace_back(y, kierunek, 'A' + literaIndex);
            literaIndex++;
            return true;
        }
    }

    return false;
}

void pauza() {
    this_thread::sleep_for(chrono::milliseconds(1500));
}

void rysujPlansze(const vector<Plane>& samoloty) {
    for (int y = -1; y <= WYSOKOSC; y++) {
        for (int x = -1; x <= SZEROKOSC; x++) {
            if (y == -1 || y == WYSOKOSC) {
                cout << '=';
            } else if (x == -1 || x == SZEROKOSC) {
                cout << '|';
            } else {
                bool czySamolot = false;
                for (const auto& samolot : samoloty) {
                    if (samolot.y == y && x / 5 == samolot.x) {
                        string pokaz = "(" + string(1, samolot.oznaczenie) +
                                       to_string(samolot.liczbaPolKomendy) + ")" +
                                       samolot.znakKomendy;
                        cout << pokaz[x % 5];
                        czySamolot = true;
                        break;
                    }
                }
                if (!czySamolot) cout << ' ';
            }
        }
        cout << endl;
    }
}

int main() {
    srand(time(0));
    vector<Plane> samoloty;
    int literaIndex = 0;
    int tura = 0;

    // Inicjalizacja pierwszych 3 samolotów
    for (int i = 0; i < 3; ++i) {
        bool kierunek = (i % 2 == 0);
        samoloty.emplace_back(rand() % RAMKA_Y, kierunek, 'A' + literaIndex);
        literaIndex++;
    }

    while (true) {
        tura++;  // Licznik tur

        // Przesunięcie wszystkich samolotów
        for (auto& s : samoloty) {
            s.przesun();
        }

        // Usuwanie samolotów poza planszą
        samoloty.erase(remove_if(samoloty.begin(), samoloty.end(), [](Plane& p) {
            return p.pozaPlansza();
        }), samoloty.end());

        // Koniec, jeśli nie ma samolotów na planszy
        if (samoloty.empty()) {
            cout << "Wszystkie samoloty zakończyły lot." << endl;
            break;
        }

        // Wydanie nowej komendy
        string komenda = wydajKomende(samoloty);
        cout << "Podana komenda: " << komenda << endl;

        // 🔹 **Gwarantowane dodanie nowego samolotu w 5. turze**
        if (tura == 5 && samoloty.size() < 4) {
            cout << "Automatyczne dodanie samolotu w turze 5!" << endl;
            generujNowySamolot(samoloty, literaIndex);
        }
        // 🔹 **Losowe dodanie samolotu (1/13 zamiast 1/16)**
        else if (samoloty.size() < 4 && rand() % 13 == 0) {
            generujNowySamolot(samoloty, literaIndex);
        }

        // Sprawdzenie kolizji
        if (sprawdzKolizje(samoloty)) {
            cout << "Symulacja zakończona: kolizja w powietrzu!" << endl;
            break;
        }

        // Wyświetlenie planszy
        rysujPlansze(samoloty);

        // Pauza na 1,5 sekundy
        pauza();
    }

    return 0;
}
