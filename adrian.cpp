#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <string>
#include <climits>

using namespace std;

const int SZEROKOSC = 60;
const int WYSOKOSC = 10;
const int RAMKA_X = 12;
const int RAMKA_Y = 10;
const int PRZEWIDYWANIE_TUR = 6;  // Increased from 4 to 6 for better collision prediction

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
        // Horizontal movement
        x += (kierunek ? 1 : -1);

        // Handle command delay
        if (!aktywowanaKomenda) {
            aktywowanaKomenda = true;
            return;
        }

        // Vertical movement
        if (znakKomendy == '/' && liczbaPolKomendy > 0) {
            y = max(0, y - 1);
            liczbaPolKomendy--;
        } 
        else if (znakKomendy == '\\' && liczbaPolKomendy > 0) {
            y = min(RAMKA_Y - 1, y + 1);
            liczbaPolKomendy--;
        }

        // Reset to stable flight after command completion
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
            if (dx < 3 && dy < 3) {
                return true;
            }
        }
    }
    return false;
}

vector<Plane> symulujRuch(vector<Plane> samoloty, int tury) {
    for (int t = 0; t < tury; ++t) {
        for (auto& p : samoloty) {
            p.przesun();
        }
    }
    return samoloty;
}

string wydajKomende(vector<Plane>& samoloty) {
    struct Scenariusz {
        string komenda;
        bool bezpieczny;
        int minOdleglosc;
        int zmianaWysokosci;
    };

    vector<Scenariusz> mozliweScenariusze;

    // Generate possible commands for each plane
    for (auto& samolot : samoloty) {
        if (!samolot.czyLeci || samolot.znakKomendy != '=') continue;

        // Scenario 1: No command (space)
        {
            int minDist = INT_MAX;
            vector<Plane> tmp = samoloty;
            for (int t = 0; t < PRZEWIDYWANIE_TUR; ++t) {
                for (auto& p : tmp) p.przesun();
                
                // Check distances between all planes
                for (size_t i = 0; i < tmp.size(); ++i) {
                    for (size_t j = i + 1; j < tmp.size(); ++j) {
                        int dx = abs(tmp[i].x - tmp[j].x);
                        int dy = abs(tmp[i].y - tmp[j].y);
                        minDist = min(minDist, max(dx, dy));
                    }
                }
            }
            mozliweScenariusze.push_back({"Spacja", minDist >= 3, minDist, 0});
        }

        // Scenario 2: Ascend command
        if (samolot.y > 0) {
            int maxWznoszenie = min(2, samolot.y);
            if (maxWznoszenie > 0) {
                auto kop = samoloty;
                int idx = &samolot - &samoloty[0];
                kop[idx].znakKomendy = '/';
                kop[idx].liczbaPolKomendy = maxWznoszenie;
                kop[idx].aktywowanaKomenda = false;

                int minDist = INT_MAX;
                for (auto& p : kop) p.przesun(); // Initial movement
                
                vector<Plane> tmp = kop;
                for (int t = 1; t < PRZEWIDYWANIE_TUR; ++t) {
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

        // Scenario 3: Descend command
        if (samolot.y < RAMKA_Y - 1) {
            int maxOpadanie = min(2, RAMKA_Y - 1 - samolot.y);
            if (maxOpadanie > 0) {
                auto kop = samoloty;
                int idx = &samolot - &samoloty[0];
                kop[idx].znakKomendy = '\\';
                kop[idx].liczbaPolKomendy = maxOpadanie;
                kop[idx].aktywowanaKomenda = false;

                int minDist = INT_MAX;
                for (auto& p : kop) p.przesun(); // Initial movement
                
                vector<Plane> tmp = kop;
                for (int t = 1; t < PRZEWIDYWANIE_TUR; ++t) {
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

    // Sort scenarios: safe first, then by max distance, then by minimal altitude change
    sort(mozliweScenariusze.begin(), mozliweScenariusze.end(), [](const Scenariusz& a, const Scenariusz& b) {
        if (a.bezpieczny != b.bezpieczny) return a.bezpieczny > b.bezpieczny;
        if (a.minOdleglosc != b.minOdleglosc) return a.minOdleglosc > b.minOdleglosc;
        return a.zmianaWysokosci < b.zmianaWysokosci;
    });

    // Apply the first safe command found
    for (auto& scenariusz : mozliweScenariusze) {
        if (scenariusz.bezpieczny) {
            if (scenariusz.komenda == "Spacja") {
                return "Spacja";
            }
            
            // Apply the command to the actual plane
            for (auto& samolot : samoloty) {
                if (samolot.oznaczenie == scenariusz.komenda[0]) {
                    if (scenariusz.komenda.find("/") != string::npos) {
                        samolot.znakKomendy = '/';
                    } 
                    else if (scenariusz.komenda.find("\\") != string::npos) {
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

    // If no safe command found, return space
    return "Spacja";
}

bool generujNowySamolot(vector<Plane>& samoloty, int& literaIndex) {
    // Count planes in each direction
    int liczbaPrawo = 0;
    int liczbaLewo = 0;
    for (auto& s : samoloty) {
        if (s.kierunek) liczbaPrawo++;
        else liczbaLewo++;
    }

    // Max 4 planes total, max 2 in each direction
    if (samoloty.size() >= 4) return false;
    if ((liczbaPrawo >= 2 && rand() % 2 == 1) || (liczbaLewo >= 2 && rand() % 2 == 0)) {
        return false;
    }

    // If 3 planes, each must be at least 4 fields from X edge
    if (samoloty.size() >= 3) {
        for (auto& s : samoloty) {
            if ((s.kierunek && s.x < 4) || (!s.kierunek && (RAMKA_X - s.x) < 4)) {
                return false;
            }
        }
    }

    // Try to find a safe position
    bool kierunek = rand() % 2;
    int attempts = 0;
    const int MAX_ATTEMPTS = 20;
    
    while (attempts++ < MAX_ATTEMPTS) {
        int y = rand() % RAMKA_Y;
        bool moznaDodac = true;
        
        for (auto& s : samoloty) {
            int dx = abs((kierunek ? -1 : RAMKA_X) - s.x);
            int dy = abs(y - s.y);
            
            // Check safe distances
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
            }
            else if (x == -1 || x == SZEROKOSC) {
                cout << '|';
            }
            else {
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

    // Generate initial 3 planes with safe distances
    for (int i = 0; i < 3; ++i) {
        bool kierunek = (i % 2 == 0);
        int attempts = 0;
        const int MAX_ATTEMPTS = 20;
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
            // If couldn't find safe position after attempts, add anyway
            samoloty.emplace_back(rand() % RAMKA_Y, kierunek, 'A' + literaIndex);
            literaIndex++;
        }
    }

    while (true) {
        // Move all planes
        for (auto& s : samoloty) {
            s.przesun();
        }

        // Remove planes that left the board
        samoloty.erase(remove_if(samoloty.begin(), samoloty.end(), [](Plane& p) {
            return p.pozaPlansza();
        }), samoloty.end());

        // End if no planes left
        if (samoloty.empty()) {
            cout << "Wszystkie samoloty ukonczily lot." << endl;
            break;
        }

        // Issue command
        string komenda = wydajKomende(samoloty);
        cout << "Podana komenda: " << komenda << endl;

        // Randomly add new plane (1/16 chance)
        if (samoloty.size() < 4 && rand() % 16 == 0) {
            generujNowySamolot(samoloty, literaIndex);
        }

        // Check for collisions
        if (sprawdzKolizje(samoloty)) {
            cout << "Symulacja zakonczona: kolizja w powietrzu!" << endl;
            break;
        }

        // Draw board
        rysujPlansze(samoloty);
        
        // Pause
        pauza();
    }

    return 0;
}