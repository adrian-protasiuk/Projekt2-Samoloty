// Projekt wykonali:
// Adrian Protasiuk, s203374, acir 3B
// Kajetan Rajczyk, s203627 , acir 3B

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
#include <atomic>
#include <exception>
#include <mutex>

using namespace std;

const int SZEROKOSC = 60;
const int WYSOKOSC = 10;
const int RAMKA_X = 12;
const int RAMKA_Y = 10;
const int PRZEWIDYWANIE_TUR = 12;
const int BLOKADA_STRONY_TURY_POCZATEK = 7;
const int BLOKADA_STRONY_TURY_1 = 10;
const int BLOKADA_STRONY_TURY_2 = 13;
const int TURA_ZWIEKSZENIA_BLOKADY_1 = 13;
const int TURA_ZWIEKSZENIA_BLOKADY_2 = 21;
const int TURA_PIERWSZEGO_SAMOLOTU = 6;
const int MINIMALNY_ODSTEP_POCZATKOWE = 2;

atomic<bool> zatrzymajMonitorowanie(false);
atomic<bool> wykrytaKolizja(false);
mutex mutexSamolotow;

class Samolot {
public:
    int x, y;
    char oznaczenie;
    bool czyLeci;
    bool kierunek;
    int liczbaPolKomendy;
    char znakKomendy;
    bool aktywowanaKomenda;
    bool czyPoczatkowy = false;

    Samolot(int startY, bool startKierunek, char litera)
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

        if (znakKomendy == 'c') {
            znakKomendy = '=';
            liczbaPolKomendy = 0;
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

bool sprawdzKolizje(const vector<Samolot>& samoloty, bool czyWypisywac = false) {

    bool czyKolizja = false;
    int wspolrzednaKolizji1;
    int wspolrzednaKolizji2;

    for (size_t i = 0; i < samoloty.size(); ++i) {
        wspolrzednaKolizji1 = i;

        if (czyKolizja)
            break;

        for (size_t j = i + 1; j < samoloty.size(); ++j) {
            wspolrzednaKolizji2 = j;

            int dx = abs(samoloty[i].x - samoloty[j].x);
            int dy = abs(samoloty[i].y - samoloty[j].y);

            if (samoloty[i].kierunek != samoloty[j].kierunek) {
                if (dx <= 2 && dy <= 2) {
                    czyKolizja = true;
                }

                if (dx == 0 && dy <= 3) {
                    czyKolizja = true;
                }
            }

            else {
                if (dx <= 2 && dy < 3) {
                    czyKolizja = true;
                }
            }
        }
    }

    if (czyKolizja && czyWypisywac) {
        cout << "Kolizja: Samolot " << samoloty[wspolrzednaKolizji1].oznaczenie << " ("
             << samoloty[wspolrzednaKolizji1].x << "," << samoloty[wspolrzednaKolizji1].y << ", "
             << (samoloty[wspolrzednaKolizji1].kierunek ? "kierunek w prawo" : "kierunek w lewo") << ") zderzyl sie z samolotem "
             << samoloty[wspolrzednaKolizji2].oznaczenie << " ("
             << samoloty[wspolrzednaKolizji2].x << "," << samoloty[wspolrzednaKolizji2].y << ", "
             << (samoloty[wspolrzednaKolizji2].kierunek ? "kierunek w prawo" : "kierunek w lewo") << ")" << endl;
    }
    return czyKolizja;

}

void monitorujStan(vector<Samolot>* samoloty, atomic<bool>* zatrzymaj, atomic<bool>* kolizja) {
    while (!zatrzymaj->load()) {
        this_thread::sleep_for(chrono::milliseconds(100));

        lock_guard<mutex> lock(mutexSamolotow);

        bool wszystkieZakonczyly = all_of(samoloty->begin(), samoloty->end(), [](const Samolot& p) {
            return p.pozaPlansza();
        });

        if (wszystkieZakonczyly) {
            throw runtime_error("Wszystkie samoloty zakonczyly lot!");
        }

        if (sprawdzKolizje(*samoloty, true)) {
            kolizja->store(true);
            throw runtime_error("Symulacja zakonczona: kolizja w powietrzu!");
        }
    }
}

string wydajKomende(vector<Samolot>& samoloty) {

    for (auto& samolot : samoloty) {
        if (samolot.znakKomendy != '=') {
            for (const auto& inny : samoloty) {
                if (samolot.oznaczenie != inny.oznaczenie &&
                    abs(samolot.x - inny.x) <= 6 &&
                    abs(samolot.y - inny.y) <= 3) {

                    auto kopia = samoloty;
                    auto it = find_if(kopia.begin(), kopia.end(),
                        [&](const Samolot& s) { return s.oznaczenie == samolot.oznaczenie; });

                    for (int t = 0; t < 3; ++t) {
                        it->przesun();
                        if (sprawdzKolizje(kopia)) {

                            samolot.znakKomendy = '=';
                            samolot.liczbaPolKomendy = 0;
                            return string(1, samolot.oznaczenie) + " c";
                        }
                    }
                    }
            }
        }
    }

     for (auto& s1 : samoloty) {
        for (auto& s2 : samoloty) {
            if (s1.kierunek != s2.kierunek &&
                abs(s1.x - s2.x) <= 2 &&
                abs(s1.y - s2.y) <= 2) {

                vector<Samolot*> kandydaci = {&s1, &s2};

                for (auto s : kandydaci) {
                    if (s->znakKomendy != '=') continue;

                    if (s->y < RAMKA_Y-1) {
                        int noweY = s->y + 1;
                        bool bezpieczne = true;

                        for (auto& other : samoloty) {
                            if (&other == s) continue;
                            if (abs(other.x - s->x) <= 2 && abs(other.y - noweY) <= 2) {
                                bezpieczne = false;
                                break;
                            }
                        }

                        if (bezpieczne) {
                            s->znakKomendy = '\\';
                            s->liczbaPolKomendy = 1;
                            return string(1, s->oznaczenie) + " \\ 1";
                        }
                    }

                    if (s->y > 0) {
                        int noweY = s->y - 1;
                        bool bezpieczne = true;

                        for (auto& other : samoloty) {
                            if (&other == s) continue;
                            if (abs(other.x - s->x) <= 2 && abs(other.y - noweY) <= 2) {
                                bezpieczne = false;
                                break;
                            }
                        }

                        if (bezpieczne) {
                            s->znakKomendy = '/';
                            s->liczbaPolKomendy = 1;
                            return string(1, s->oznaczenie) + " / 1";
                        }
                    }
                }
            }
        }
    }

    vector<pair<char, char>> przeciwnePary;
    for (size_t i = 0; i < samoloty.size(); ++i) {
        for (size_t j = i + 1; j < samoloty.size(); ++j) {
            if (samoloty[i].kierunek != samoloty[j].kierunek) {
                int dx = abs(samoloty[i].x - samoloty[j].x);
                int dy = abs(samoloty[i].y - samoloty[j].y);

                if (dx <= 6 && dy <= 2) {
                    przeciwnePary.emplace_back(samoloty[i].oznaczenie, samoloty[j].oznaczenie);
                }
            }
        }
    }

    if (!przeciwnePary.empty()) {
        for (auto& para : przeciwnePary) {
            for (char oznaczenie : {para.first, para.second}) {
                auto it = find_if(samoloty.begin(), samoloty.end(),
                    [oznaczenie](const Samolot& p) { return p.oznaczenie == oznaczenie; });

                if (it != samoloty.end() && it->znakKomendy == '=') {
                    vector<pair<char, int>> mozliweManewry;

                    if (it->y > 0) mozliweManewry.emplace_back('/', min(2, it->y));
                    if (it->y < RAMKA_Y - 1) mozliweManewry.emplace_back('\\', min(2, RAMKA_Y - 1 - it->y));

                    for (auto& manewr : mozliweManewry) {
                        auto kop = samoloty;
                        auto& s = kop[it - samoloty.begin()];
                        s.znakKomendy = manewr.first;
                        s.liczbaPolKomendy = manewr.second;

                        bool bezpieczny = true;
                        for (int t = 0; t < 4; ++t) {
                            for (auto& p : kop) p.przesun();
                            if (sprawdzKolizje(kop)) {
                                bezpieczny = false;
                                break;
                            }
                        }

                        if (bezpieczny) {
                            it->znakKomendy = manewr.first;
                            it->liczbaPolKomendy = manewr.second;
                            return string(1, it->oznaczenie) + " " + manewr.first + " " + to_string(manewr.second);
                        }
                    }
                }
            }
        }
    }

    map<int, bool> krytyczneKolumny;
    for (const auto& s1 : samoloty) {
        for (const auto& s2 : samoloty) {
            if (s1.kierunek != s2.kierunek && abs(s1.x - s2.x) <= 4) {
                krytyczneKolumny[s1.x] = true;
                break;
            }
        }
    }

    map<int, vector<char>> samolotyNaY;
    for (const auto& s : samoloty) {
        samolotyNaY[s.y].push_back(s.oznaczenie);
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
        bool naTymSamymY = (samolotyNaY[samolot.y].size() > 1);

        for (const auto& s : samoloty) {
            if (s.kierunek != samolot.kierunek &&
                abs(s.x - samolot.x) <= 4 &&
                abs(s.y - samolot.y) <= 2) {
                bezposrednieZagrozenie = true;
                break;
            }
        }

        auto symuluj = [&](char znak, int ile, int priorytet) {
            if (wKrytycznejKolumnie) priorytet += 2;
            if (bezposrednieZagrozenie) priorytet += 3;
            if (naTymSamymY) priorytet += 1;

            auto kop = samoloty;
            int idx = &samolot - &samoloty[0];
            kop[idx].znakKomendy = znak;
            kop[idx].liczbaPolKomendy = ile;
            kop[idx].aktywowanaKomenda = false;

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
                string(1, samolot.oznaczenie) + " " + znak + " " + to_string(ile),
                !kolizja,
                minDist,
                ile,
                priorytet
            });
        };

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
                (wKrytycznejKolumnie || bezposrednieZagrozenie || naTymSamymY) ? 1 : 0
            });
        }

        if (samolot.y > 0) {
            symuluj('/', min(2, samolot.y), 2);
        }
        if (samolot.y < RAMKA_Y - 1) {
            symuluj('\\', min(2, RAMKA_Y - 1 - samolot.y), 2);
        }

        if (bezposrednieZagrozenie) {
            if (samolot.y > 1) {
                symuluj('/', min(4, samolot.y), 5);
            }
            if (samolot.y < RAMKA_Y - 2) {
                symuluj('\\', min(4, RAMKA_Y - 1 - samolot.y), 5);
            }
        }
    }

    sort(mozliweScenariusze.begin(), mozliweScenariusze.end(), [](const Scenariusz& a, const Scenariusz& b) {
        if (a.bezpieczny != b.bezpieczny) return a.bezpieczny > b.bezpieczny;
        if (a.priorytet != b.priorytet) return a.priorytet > b.priorytet;
        if (a.minOdleglosc != b.minOdleglosc) return a.minOdleglosc > b.minOdleglosc;
        return a.zmianaWysokosci < b.zmianaWysokosci;
    });

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

    for (auto& s : samoloty) {
        if (s.znakKomendy != '=' && (s.znakKomendy == '/' || s.znakKomendy == '\\')) {
            return string(1, s.oznaczenie) + " c";
        }
    }

    return "Spacja";
}

pair<int, int> znajdzNajbezpieczniejszeY(const vector<Samolot>& samoloty, bool kierunekNowego) {
    vector<int> przeciwneY;
    for (const auto& s : samoloty) {
        if (s.kierunek != kierunekNowego) {
            przeciwneY.push_back(s.y);
        }
    }

    if (przeciwneY.empty()) {
        return {rand() % 2 == 0 ? 0 : RAMKA_Y - 1, RAMKA_Y};
    }

    int najlepszeY = 0;
    int maxOdleglosc = -1;

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

bool generujNowySamolot(vector<Samolot>& samoloty, int& literaIndex, int tura) {
    if (samoloty.size() >= 4) return false;
    if (tura <= TURA_PIERWSZEGO_SAMOLOTU) return false;

    int prawo = count_if(samoloty.begin(), samoloty.end(),
        [](const Samolot& p) { return p.kierunek; });
    int lewo = samoloty.size() - prawo;

    if (prawo >= 2 && lewo >= 2) return false;

    bool kierunek = (prawo < lewo) ? true : false;
    if (prawo >= 2) kierunek = false;
    if (lewo >= 2) kierunek = true;

    int blokadaTur = BLOKADA_STRONY_TURY_POCZATEK;
    if (tura >= TURA_ZWIEKSZENIA_BLOKADY_2) blokadaTur = 13;
    else if (tura >= TURA_ZWIEKSZENIA_BLOKADY_1) blokadaTur = 10;

    vector<bool> bezpieczneY(RAMKA_Y, true);
    for (const auto& s : samoloty) {
        if (s.kierunek == kierunek) {
            for (int y = max(0, s.y-2); y <= min(RAMKA_Y-1, s.y+2); ++y) {
                bezpieczneY[y] = false;
            }
        }
    }

    vector<int> dostepneY;
    for (int y = 0; y < RAMKA_Y; ++y) {
        if (bezpieczneY[y]) dostepneY.push_back(y);
    }

    if (dostepneY.empty()) return false;

    int y = dostepneY[rand() % dostepneY.size()];
    char litera = 'A' + literaIndex;

    samoloty.emplace_back(y, kierunek, litera);
    cout << "Pojawil sie samolot " << litera
     << " po stronie " << (kierunek ? "lewej" : "prawej")
     << " na wysokosci " << y+1 << endl;
    literaIndex++;
    return true;
}

bool dodajSamolotPodczasWybranejTury(vector<Samolot>& samoloty, int& literaIndex) {
    if (samoloty.size() >= 4) return false;

    int prawo = count_if(samoloty.begin(), samoloty.end(), [](const Samolot& p) { return p.kierunek && !p.czyPoczatkowy; });
    int lewo = count_if(samoloty.begin(), samoloty.end(), [](const Samolot& p) { return !p.kierunek && !p.czyPoczatkowy; });
    bool kierunek = (prawo < lewo);

    auto [y, bezpieczenstwo] = znajdzNajbezpieczniejszeY(samoloty, kierunek);

    bool moznaDodac = true;
    for (const auto& s : samoloty) {
        int dx = abs((kierunek ? -1 : RAMKA_X) - s.x);
        int dy = abs(y - s.y);

        if (s.kierunek == kierunek) {
            if (dx == 0 && dy < 3) {
                moznaDodac = false;
                cout << "Nie udalo sie dodac samolotu D w turze " << TURA_PIERWSZEGO_SAMOLOTU
                << " z powodu zbyt bliskiego samolotu lecacego w te sama strone" << endl;
                break;
            }
        } else {
            if ((dy == 0 && dx < 8) || (dy <= 2 && dx < 6)) {
                moznaDodac = false;
                cout << "Nie udalo sie dodac samolotu D w turze " << TURA_PIERWSZEGO_SAMOLOTU
               << " z powodu zbyt bliskich samolotow lecacych w przeciwna strone" << endl;
                break;
            }
        }
    }

    if (moznaDodac) {
        samoloty.emplace_back(y, kierunek, 'D');
        cout << "Pojawil sie samolot " << 'D'
             << " po stronie " << (kierunek ? "lewej" : "prawej")
             << " na wysokosci " << y+1 << endl;
        literaIndex++;
        return true;
    }

    return generujNowySamolot(samoloty, literaIndex, TURA_PIERWSZEGO_SAMOLOTU);
}

void pauza() {
    this_thread::sleep_for(chrono::milliseconds(1500));
}

void rysujPlansze(const vector<Samolot>& samoloty) {
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
                           (samolot.znakKomendy == 'c' ? "=" : string(1, samolot.znakKomendy));
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
    vector<Samolot> samoloty;
    int literaIndex = 0;
    int tura = 0;
    atomic<bool> zatrzymajMonitorowanie(false);
    atomic<bool> wykrytaKolizja(false);

    vector<char> dostepneLitery = {'A', 'B', 'C'};
    random_shuffle(dostepneLitery.begin(), dostepneLitery.end());

    bool stronaA = rand() % 2 == 0;
    int yA = rand() % RAMKA_Y;
    samoloty.emplace_back(yA, stronaA, 'A');
    samoloty.back().czyPoczatkowy = true;
    cout << "Pojawil sie samolot " << 'A'
     << " po stronie " << (stronaA ? "lewej" : "prawej")
     << " na wysokosci " << yA+1 << endl;

    bool stronaB = rand() % 2 == 0;
    int yB;
    do {
        yB = rand() % RAMKA_Y;

        if (stronaA == stronaB) {
            for (const auto& s : samoloty) {
                if (s.kierunek == stronaB && abs(s.y - yB) <= 2) {
                    yB = -1;
                    break;
                }
            }
        }
    } while (yB == -1);

    samoloty.emplace_back(yB, stronaB, 'B');
    samoloty.back().czyPoczatkowy = true;
    cout << "Pojawil sie samolot " << 'B'
      << " po stronie " << (stronaB ? "lewej" : "prawej")
      << " na wysokosci " << yB+1 << endl;

    bool stronaC;
    if (stronaA == stronaB) {
        stronaC = !stronaA;
    } else {
        stronaC = rand() % 2 == 0;
    }

    int yC;
    do {
        yC = rand() % RAMKA_Y;

        for (const auto& s : samoloty) {
            if (s.kierunek == stronaC && abs(s.y - yC) <= 2) {
                yC = -1;
                break;
            }
        }
    } while (yC == -1);

    samoloty.emplace_back(yC, stronaC, 'C');
    samoloty.back().czyPoczatkowy = true;
    cout << "Pojawil sie samolot " << 'C'
     << " po stronie " << (stronaC ? "lewej" : "prawej")
     << " na wysokosci " << yC+1 << endl;

    literaIndex = 3;

    thread watekMonitorujacy([&]() {
        monitorujStan(&samoloty, &zatrzymajMonitorowanie, &wykrytaKolizja);
    });

    try {
        while (true) {
            tura++;

            {
                lock_guard<mutex> lock(mutexSamolotow);
                for (auto& s : samoloty) {
                    s.przesun();
                }

                samoloty.erase(remove_if(samoloty.begin(), samoloty.end(), [](Samolot& p) {
                    return p.pozaPlansza();
                }), samoloty.end());

                if (samoloty.empty()) {
                    cout << "Wszystkie samoloty zakonczyly lot." << endl;
                    break;
                }
            }

            string komenda;
            string poprzedniaKomenda = "Brak";

            {
                lock_guard<mutex> lock(mutexSamolotow);

                vector<Samolot> kopia = samoloty;
                komenda = wydajKomende(samoloty);

                if (komenda != "Spacja") {
                    char litera = komenda[0];
                    auto it = find_if(kopia.begin(), kopia.end(), [litera](const Samolot& s) {
                        return s.oznaczenie == litera;
                    });

                    if (it != kopia.end()) {
                        if (it->znakKomendy == '=')
                            poprzedniaKomenda = string(1, it->oznaczenie) + " =";
                        else if (it->znakKomendy == 'c')
                            poprzedniaKomenda = string(1, it->oznaczenie) + " c";
                        else
                            poprzedniaKomenda = string(1, it->oznaczenie) + " " + it->znakKomendy + " " + to_string(it->liczbaPolKomendy);
                    }
                }
            }


            string opis;
            if (komenda == "Spacja") opis = "(utrzymanie kursu)";
            else if (komenda.find("/") != string::npos) opis = "(wznoszenie)";
            else if (komenda.find("\\") != string::npos) opis = "(obnizanie)";
            else if (komenda.find("c") != string::npos) opis = "(anulowanie manewru)";

            cout << "Tura " << tura << ": " << komenda << " " << opis;
            if (poprzedniaKomenda != "Brak") {
                cout << " | Bez komendy wykonwalby: " << poprzedniaKomenda;
            }
            cout << endl;




            if (tura == TURA_PIERWSZEGO_SAMOLOTU) {
                lock_guard<mutex> lock(mutexSamolotow);
                dodajSamolotPodczasWybranejTury(samoloty, literaIndex);
            }
            else if (tura > TURA_PIERWSZEGO_SAMOLOTU && rand() % 10 == 0 && samoloty.size() < 4) {
                lock_guard<mutex> lock(mutexSamolotow);
                generujNowySamolot(samoloty, literaIndex, tura);
            }

            {
                lock_guard<mutex> lock(mutexSamolotow);
                rysujPlansze(samoloty);
            }

            if (wykrytaKolizja.load()) {
                break;
            }

            pauza();
        }
    }
    catch (const exception& e) {
        cout << e.what() << endl;
    }

    zatrzymajMonitorowanie.store(true);
    if (watekMonitorujacy.joinable()) {
        watekMonitorujacy.join();
    }

    return 0;
}
