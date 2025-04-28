//
// Created by adria on 28.04.2025.
//

// WPROWADZAM ZMIANY NA BIEŻĄCO

#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <string>

using namespace std;

const int SZEROKOSC = 60;
const int WYSOKOSC = 10;
const int RAMKA_X = 12;
const int RAMKA_Y = 10;
const int PRZEWIDYWANIE_TUR = 4;

class Plane {
public:
    int x, y;
    char oznaczenie;
    bool czyLeci;
    bool kierunek;
    int liczbaPolKomendy;
    char znakKomendy;
    bool aktywowanaKomenda;

    Plane(int startY, bool startKierunek, char litera) {
        y = startY;
        kierunek = startKierunek;
        oznaczenie = litera;
        czyLeci = true;
        znakKomendy = '=';
        liczbaPolKomendy = 0;
        aktywowanaKomenda = true;
        x = (kierunek ? -1 : RAMKA_X);
    }

    void przesun() {
        if (kierunek) x++;
        else x--;

        if (!aktywowanaKomenda) {
            aktywowanaKomenda = true;
        } else {
            if (znakKomendy == '/' && liczbaPolKomendy > 0) {
                y--;
                liczbaPolKomendy--;
            } else if (znakKomendy == '\\' && liczbaPolKomendy > 0) {
                y++;
                liczbaPolKomendy--;
            }
            if (liczbaPolKomendy == 0 && (znakKomendy == '/' || znakKomendy == '\\')) {
                znakKomendy = '=';
            }
        }
    }

    bool pozaPlansza() const {
        return (x < -1 || x > RAMKA_X || y < 0 || y >= RAMKA_Y);
    }
};

bool sprawdzKolizje(const vector<Plane>& samoloty) {
    for (size_t i = 0; i < samoloty.size(); ++i) {
        for (size_t j = i + 1; j < samoloty.size(); ++j) {
            if (abs(samoloty[i].x - samoloty[j].x) < 3 && abs(samoloty[i].y - samoloty[j].y) < 3) {
                return true;
            }
        }
    }
    return false;
}

vector<Plane> symulujRuch(vector<Plane> samoloty, int tury) {
    for (int t = 0; t < tury; ++t) {
        for (auto& p : samoloty) p.przesun();
    }
    return samoloty;
}

string wydajKomende(vector<Plane>& samoloty) {
    struct Scenariusz {
        string komenda;
        vector<Plane> stan;
        bool bezpieczny;
        int stabilnosc;
    };

    vector<Scenariusz> mozliweScenariusze;

    for (auto& samolot : samoloty) {
        if (!samolot.czyLeci || samolot.znakKomendy != '=') continue;

        auto baza = samoloty;
        auto stanSpacja = symulujRuch(baza, PRZEWIDYWANIE_TUR);
        mozliweScenariusze.push_back({"Spacja", stanSpacja, !sprawdzKolizje(stanSpacja), 0});

        if (samolot.y > 0) {
            auto wzno = samoloty;
            int ileMoznaWznieść = min(2, samolot.y);
            if (ileMoznaWznieść > 0) {
                wzno[&samolot - &samoloty[0]].znakKomendy = '/';
                wzno[&samolot - &samoloty[0]].liczbaPolKomendy = ileMoznaWznieść;
                wzno[&samolot - &samoloty[0]].aktywowanaKomenda = false;
                auto stanWzno = symulujRuch(wzno, PRZEWIDYWANIE_TUR);
                mozliweScenariusze.push_back({string(1, samolot.oznaczenie) + " / " + to_string(ileMoznaWznieść), stanWzno, !sprawdzKolizje(stanWzno), ileMoznaWznieść});
            }
        }

        if (samolot.y < RAMKA_Y - 1) {
            auto opad = samoloty;
            int ileMoznaOpasc = min(2, RAMKA_Y - 1 - samolot.y);
            if (ileMoznaOpasc > 0) {
                opad[&samolot - &samoloty[0]].znakKomendy = '\\';
                opad[&samolot - &samoloty[0]].liczbaPolKomendy = ileMoznaOpasc;
                opad[&samolot - &samoloty[0]].aktywowanaKomenda = false;
                auto stanOpad = symulujRuch(opad, PRZEWIDYWANIE_TUR);
                mozliweScenariusze.push_back({string(1, samolot.oznaczenie) + " \\ " + to_string(ileMoznaOpasc), stanOpad, !sprawdzKolizje(stanOpad), ileMoznaOpasc});
            }
        }
    }

    // Najpierw szukamy bezpiecznych, potem najbardziej stabilnych
    sort(mozliweScenariusze.begin(), mozliweScenariusze.end(), [](const Scenariusz& a, const Scenariusz& b) {
        if (a.bezpieczny != b.bezpieczny) return a.bezpieczny > b.bezpieczny;
        return a.stabilnosc > b.stabilnosc;
    });

    for (auto& s : mozliweScenariusze) {
        if (s.bezpieczny) {
            if (s.komenda == "Spacja") return "Spacja";
            for (auto& samolot : samoloty) {
                if (s.komenda[0] == samolot.oznaczenie) {
                    if (s.komenda.find("/") != string::npos) samolot.znakKomendy = '/';
                    else if (s.komenda.find("\\") != string::npos) samolot.znakKomendy = '\\';
                    size_t pos = s.komenda.find(" ");
                    samolot.liczbaPolKomendy = stoi(s.komenda.substr(pos + 3));
                    samolot.aktywowanaKomenda = false;
                }
            }
            return s.komenda;
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

    if (samoloty.size() >= 4) return false;
    bool kierunek = rand() % 2;
    if ((kierunek && liczbaPrawo >= 2) || (!kierunek && liczbaLewo >= 2)) return false;

    if (samoloty.size() >= 3) {
        for (auto& s : samoloty) {
            if (s.kierunek && s.x < 4) return false;
            if (!s.kierunek && (RAMKA_X - s.x) < 4) return false;
        }
    }

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
        samoloty.push_back(Plane(y, kierunek, 'A' + literaIndex));
        literaIndex++;
        return true;
    }

    return false;
}

void pauza() {
    this_thread::sleep_for(chrono::milliseconds(1500));
}

void rysujPlansze(const vector<Plane>& samoloty) {
    for (int y = -1; y <= WYSOKOSC; y++) {
        for (int x = -1; x <= SZEROKOSC; x++) {
            if (y == -1 || y == WYSOKOSC) cout << '=';
            else if (x == -1 || x == SZEROKOSC) cout << '|';
            else {
                bool czySamolot = false;
                for (const auto& samolot : samoloty) {
                    if (samolot.y == y && x / 5 == samolot.x) {
                        string pokaz = "(" + string(1, samolot.oznaczenie) + to_string(samolot.liczbaPolKomendy) + ")" + samolot.znakKomendy;
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

    for (int i = 0; i < 3; ++i) {
        bool kierunek = (i % 2 == 0);
        int y = rand() % RAMKA_Y;
        samoloty.push_back(Plane(y, kierunek, 'A' + literaIndex));
        literaIndex++;
    }

    while (true) {
        for (auto& s : samoloty) {
            s.przesun();
        }

        samoloty.erase(remove_if(samoloty.begin(), samoloty.end(), [](Plane& p) {
            return p.pozaPlansza();
        }), samoloty.end());

        if (samoloty.empty()) {
            cout << "Wszystkie samoloty ukonczily lot." << endl;
            break;
        }

        string komenda = wydajKomende(samoloty);
        cout << "Podana komenda: " << komenda << endl;

        if (samoloty.size() < 4 && rand() % 16 == 0) {
            generujNowySamolot(samoloty, literaIndex);
        }

        if (sprawdzKolizje(samoloty)) {
            cout << "Symulacja zakonczona: kolizja w powietrzu!" << endl;
            break;
        }

        rysujPlansze(samoloty);
        pauza();
    }

    return 0;
}
