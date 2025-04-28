#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <climits>
#include <map>

using namespace std;

const int SZEROKOSC = 60;
const int WYSOKOSC = 10;
const int RAMKA_X = 12;
const int RAMKA_Y = 10;
const int PRZEWIDYWANIE_TUR = 12;  // Zwiększony horyzont przewidywania

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

            // Dla samolotów w tym samym kierunku
            if (samoloty[i].kierunek == samoloty[j].kierunek) {
                if (dx <= 2 && dy < 3) {  // Kolizja pionowa
                    return true;
                }
            }
            // Dla samolotów w przeciwnych kierunkach
            else {
                if (dx < 3 && dy < 3) {  // Pełna kolizja
                    return true;
                }
            }
        }
    }
    return false;
}

string wydajKomende(vector<Plane>& samoloty) {
    // Znajdź wszystkie krytyczne kolumny (gdzie są samoloty w obu kierunkach w odległości <=4)
    map<int, bool> krytyczneKolumny;
    for (const auto& s1 : samoloty) {
        for (const auto& s2 : samoloty) {
            if (s1.kierunek != s2.kierunek && abs(s1.x - s2.x) <= 4) {
                krytyczneKolumny[s1.x] = true;
                break;
            }
        }
    }

    struct Scenariusz {
        string komenda;
        bool bezpieczny;
        int minOdleglosc;
        int zmianaWysokosci;
        int priorytet;
    };

    vector<Scenariusz> mozliweScenariusze;

    for (auto& samolot : samoloty) {
        if (!samolot.czyLeci || samolot.znakKomendy != '=') continue;

        bool wKrytycznejKolumnie = krytyczneKolumny[samolot.x];
        bool bezposrednieZagrozenie = false;

        // Sprawdź bezpośrednie zagrożenie kolizją
        for (const auto& s : samoloty) {
            if (s.kierunek != samolot.kierunek &&
                abs(s.x - samolot.x) <= 4 &&
                abs(s.y - samolot.y) <= 2) {
                bezposrednieZagrozenie = true;
                break;
            }
        }

        auto symuluj = [&](char znak, int ile, int priorytet) {
            // Zwiększ priorytet w zależności od zagrożenia
            if (wKrytycznejKolumnie) priorytet += 2;
            if (bezposrednieZagrozenie) priorytet += 3;

            auto kop = samoloty;
            int idx = &samolot - &samoloty[0];
            kop[idx].znakKomendy = znak;
            kop[idx].liczbaPolKomendy = ile;
            kop[idx].aktywowanaKomenda = false;

            bool kolizja = false;
            int minDist = INT_MAX;

            for (int t = 0; t < PRZEWIDYWANIE_TUR; ++t) {
                for (auto& p : kop) p.przesun();

                // Sprawdź kolizje
                for (size_t i = 0; i < kop.size(); ++i) {
                    for (size_t j = i + 1; j < kop.size(); ++j) {
                        int dx = abs(kop[i].x - kop[j].x);
                        int dy = abs(kop[i].y - kop[j].y);

                        if (kop[i].kierunek == kop[j].kierunek) {
                            if (dx <= 2 && dy < 3) kolizja = true;
                        } else {
                            if (dx < 3 && dy < 3) kolizja = true;
                        }
                        minDist = min(minDist, max(dx, dy));
                    }
                }

                if (kolizja) break;
            }

            mozliweScenariusze.push_back({
                string(1, samolot.oznaczenie) + " " + znak + " " + to_string(ile),
                !kolizja,
                minDist,
                ile,
                priorytet
            });
        };

        // Scenariusz: brak komendy
        {
            auto kop = samoloty;
            bool kolizja = false;
            int minDist = INT_MAX;

            for (int t = 0; t < PRZEWIDYWANIE_TUR; ++t) {
                for (auto& p : kop) p.przesun();

                for (size_t i = 0; i < kop.size(); ++i) {
                    for (size_t j = i + 1; j < kop.size(); ++j) {
                        int dx = abs(kop[i].x - kop[j].x);
                        int dy = abs(kop[i].y - kop[j].y);

                        if (kop[i].kierunek == kop[j].kierunek) {
                            if (dx <= 2 && dy < 3) kolizja = true;
                        } else {
                            if (dx < 3 && dy < 3) kolizja = true;
                        }
                        minDist = min(minDist, max(dx, dy));
                    }
                }

                if (kolizja) break;
            }

            mozliweScenariusze.push_back({
                "Spacja",
                !kolizja,
                minDist,
                0,
                (wKrytycznejKolumnie || bezposrednieZagrozenie) ? 1 : 0
            });
        }

        // Standardowe komendy
        if (samolot.y > 0) {
            symuluj('/', min(2, samolot.y), 2);
        }
        if (samolot.y < RAMKA_Y - 1) {
            symuluj('\\', min(2, RAMKA_Y - 1 - samolot.y), 2);
        }

        // Komendy awaryjne dla zagrożonych samolotów
        if (bezposrednieZagrozenie) {
            if (samolot.y > 1) {
                symuluj('/', min(4, samolot.y), 5);
            }
            if (samolot.y < RAMKA_Y - 2) {
                symuluj('\\', min(4, RAMKA_Y - 1 - samolot.y), 5);
            }
        }
    }

    // Sortowanie scenariuszy
    sort(mozliweScenariusze.begin(), mozliweScenariusze.end(), [](const Scenariusz& a, const Scenariusz& b) {
        if (a.bezpieczny != b.bezpieczny) return a.bezpieczny > b.bezpieczny;
        if (a.priorytet != b.priorytet) return a.priorytet > b.priorytet;
        if (a.minOdleglosc != b.minOdleglosc) return a.minOdleglosc > b.minOdleglosc;
        return a.zmianaWysokosci < b.zmianaWysokosci;
    });

    // Wybierz i zastosuj pierwszą bezpieczną komendę
    for (auto& scenariusz : mozliweScenariusze) {
        if (scenariusz.bezpieczny) {
            if (scenariusz.komenda == "Spacja") {
                return "Spacja";
            }

            for (auto& s : samoloty) {
                if (s.oznaczenie == scenariusz.komenda[0]) {
                    size_t space1 = scenariusz.komenda.find(' ');
                    size_t space2 = scenariusz.komenda.rfind(' ');
                    char znak = scenariusz.komenda[space1 + 1];
                    int ile = stoi(scenariusz.komenda.substr(space2 + 1));

                    s.znakKomendy = znak;
                    s.liczbaPolKomendy = ile;
                    s.aktywowanaKomenda = false;
                    break;
                }
            }
            return scenariusz.komenda;
        }
    }

    return "Spacja";
}

pair<int, int> znajdzNajbezpieczniejszeY(const vector<Plane>& samoloty, bool kierunekNowego) {
    vector<int> przeciwneY;
    for (const auto& s : samoloty) {
        if (s.kierunek != kierunekNowego) {
            przeciwneY.push_back(s.y);
        }
    }

    // Jeśli nie ma przeciwników, wybierz losowo górę lub dół
    if (przeciwneY.empty()) {
        return {rand() % 2 == 0 ? 0 : RAMKA_Y - 1, RAMKA_Y};
    }

    // Oceń bezpieczeństwo skrajnych pozycji
    int najlepszeY = 0;
    int maxOdleglosc = -1;

    // Rozważ tylko skrajne pozycje (0 i RAMKA_Y-1)
    for (int y : {0, RAMKA_Y - 1}) {
        int minOdleglosc = INT_MAX;
        for (int py : przeciwneY) {
            minOdleglosc = min(minOdleglosc, abs(y - py));
        }

        if (minOdleglosc > maxOdleglosc ||
            (minOdleglosc == maxOdleglosc && rand() % 2 == 0)) {
            maxOdleglosc = minOdleglosc;
            najlepszeY = y;
        }
    }

    // Jeśli obie skrajne pozycje są niebezpieczne, wybierz środek
    if (maxOdleglosc < 3) {
        int srodek = RAMKA_Y / 2;
        int odleglosc = INT_MAX;
        for (int py : przeciwneY) {
            odleglosc = min(odleglosc, abs(srodek - py));
        }
        return {srodek, odleglosc};
    }

    return {najlepszeY, maxOdleglosc};
}

bool dodajSamolotW6Turze(vector<Plane>& samoloty, int& literaIndex) {
    if (samoloty.size() >= 4) return false;

    // Policz samoloty w każdym kierunku
    int prawo = count_if(samoloty.begin(), samoloty.end(), [](const Plane& p) { return p.kierunek; });
    int lewo = samoloty.size() - prawo;
    bool kierunek = (prawo < lewo);

    // Znajdź najlepsze Y (tylko góra lub dół)
    auto [y, bezpieczenstwo] = znajdzNajbezpieczniejszeY(samoloty, kierunek);

    // Sprawdź bezpieczeństwo pozycji
    bool moznaDodac = true;
    for (const auto& s : samoloty) {
        int dx = abs((kierunek ? -1 : RAMKA_X) - s.x);
        int dy = abs(y - s.y);

        if (s.kierunek == kierunek) {
            if (dx == 0 && dy < 3) {
                moznaDodac = false;
                break;
            }
        } else {
            if ((dy == 0 && dx < 8) || (dy <= 2 && dx < 6)) {
                moznaDodac = false;
                break;
            }
        }
    }

    if (moznaDodac) {
        samoloty.emplace_back(y, kierunek, 'A' + literaIndex);
        literaIndex++;
        return true;
    }

    return false;
}

bool generujNowySamolot(vector<Plane>& samoloty, int& literaIndex) {
    int liczbaPrawo = 0;
    int liczbaLewo = 0;
    for (auto& s : samoloty) {
        if (s.kierunek) liczbaPrawo++;
        else liczbaLewo++;
    }

    if (samoloty.size() >= 4) return false;
    if ((liczbaPrawo >= 2 && rand() % 2 == 1) || (liczbaLewo >= 2 && rand() % 2 == 0)) {
        return false;
    }

    if (samoloty.size() >= 3) {
        for (auto& s : samoloty) {
            if ((s.kierunek && s.x < 4) || (!s.kierunek && (RAMKA_X - s.x) < 4)) {
                return false;
            }
        }
    }

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

    // Inicjalizacja pierwszych 3 samolotów z bezpiecznymi odległościami
    for (int i = 0; i < 3; ++i) {
        bool kierunek = (i % 2 == 0);
        int attempts = 0;
        const int MAX_ATTEMPTS = 50;
        bool dodany = false;

        while (attempts++ < MAX_ATTEMPTS && !dodany) {
            int y = rand() % RAMKA_Y;
            bool moznaDodac = true;

            for (const auto& s : samoloty) {
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
                dodany = true;
            }
        }

        if (!dodany) {
            samoloty.emplace_back(rand() % RAMKA_Y, kierunek, 'A' + literaIndex);
            literaIndex++;
        }
    }

    while (true) {
        tura++;

        // Przesunięcie wszystkich samolotów
        for (auto& s : samoloty) {
            s.przesun();
        }

        // Usuwanie samolotów poza planszą
        samoloty.erase(remove_if(samoloty.begin(), samoloty.end(), [](Plane& p) {
            return p.pozaPlansza();
        }), samoloty.end());

        if (samoloty.empty()) {
            cout << "Wszystkie samoloty zakończyły lot." << endl;
            break;
        }

        // Wydanie nowej komendy
        string komenda = wydajKomende(samoloty);
        cout << "Tura " << tura << ": " << komenda << endl;

        // Dodanie nowego samolotu w 6. turze
        if (tura == 6 && samoloty.size() < 4) {
            cout << "Dodanie samolotu w 6. turze!" << endl;
            dodajSamolotW6Turze(samoloty, literaIndex);
        }
        // Losowe dodanie samolotu
        else if (samoloty.size() < 4 && rand() % 10 == 0) {
            generujNowySamolot(samoloty, literaIndex);
        }

        if (sprawdzKolizje(samoloty)) {
            cout << "Symulacja zakończona: kolizja w powietrzu!" << endl;
            break;
        }

        rysujPlansze(samoloty);
        pauza();
    }

    return 0;
}