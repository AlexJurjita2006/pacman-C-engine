# Documentație Pac-Man în C

## Cuprins
1. [Structuri de date](#1-structuri-de-date)
2. [Funcții Windows pentru grafică](#2-funcții-windows-pentru-grafică)
3. [Mișcarea asincronă](#3-mișcarea-asincronă)
4. [Modulele proiectului](#4-modulele-proiectului)
5. [Compilare și rulare](#5-compilare-și-rulare)
6. [Ghid laborator Debian 12](#6-ghid-laborator-debian-12)

---

## 1. Structuri de date

### `Pacman`
Reprezintă starea curentă a jucătorului.

```c
typedef struct {
    int x;        // coloana curentă pe hartă (0 .. MAP_COLS-1)
    int y;        // linia curentă pe hartă   (0 .. MAP_ROWS-1)
    int directie; // direcția activă: DIR_STOP / DIR_SUS / DIR_JOS /
                  //                  DIR_STANGA / DIR_DREAPTA
    int scor;     // punctele acumulate (fiecare '.' valorează 10 puncte)
} Pacman;
```

**Câmpuri cheie:**
- `x` și `y` — indexează matricea `Harta.celule[y][x]`.
- `directie` — setată de jucător prin WASD sau taste săgeți; actualizată la fiecare cadru înainte de calculul noii poziții.
- `scor` — crește cu 10 la fiecare celulă `'.'` consumată; afișat în header cu culoare verde.

---

### `Harta`
Conține layout-ul nivelului curent.

```c
typedef struct {
    char celule[MAP_ROWS][MAP_COLS + 1]; // matrice de caractere (11 × 20)
    int  nrPuncte;                       // puncte rămase de mâncat
} Harta;
```

**Semnificația caracterelor:**
| Caracter | Semnificație      | Culoare afișare |
|----------|-------------------|-----------------|
| `#`      | Perete            | Albastru intens |
| `.`      | Punct de mâncare  | Alb intens      |
| ` `      | Celulă liberă     | Gri (implicit)  |

- `nrPuncte` scade cu 1 la fiecare punct consumat; când ajunge la 0 se declanșează condiția de victorie.
- `initHarta()` copiază layout-ul static `harta_initiala` și numără punctele inițiale.

---

### `Fantoma`
Reprezintă un inamic care se mișcă aleatoriu pe hartă.

```c
typedef struct {
    int  x;       // coloana curentă
    int  y;       // linia curentă
    int  culoare; // cod culoare Windows (ex. CULOARE_ROSU = 12)
    char simbol;  // caracterul afișat, implicit 'G'
} Fantoma;
```

**Comportament:**
- Două fantome pornesc din centrul hărții (`MAP_COLS/2`, `MAP_ROWS/2`).
- La fiecare două cadre, `miscaFantome()` alege o direcție aleatorie (max. 10 încercări) care să nu fie perete sau margine.
- Dacă Pac-Man ajunge pe aceeași celulă cu o fantomă → **Game Over**.

---

### `DateUtilizator`
Date introduse de jucător înainte de pornirea jocului.

```c
typedef struct {
    char username[MAX_USERNAME]; // șir de maxim 32 caractere
    int  varsta;                 // ales prin slider interactiv (1–99)
} DateUtilizator;
```

Această structură este transmisă la `ruleazaJoc()` și folosită la afișarea numelui și la salvarea scorului în `clasament.txt`.

---

## 2. Funcții Windows pentru grafică

Proiectul folosește exclusiv API-ul Win32 din `<windows.h>` pentru operații grafice în consolă.

### `SetConsoleTextAttribute` — culori text

```c
void seteazaCuloare(int cod) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (WORD)cod);
}
```

- `GetStdHandle(STD_OUTPUT_HANDLE)` returnează un `HANDLE` spre bufferul de ieșire al consolei.
- `WORD cod` este o combinație de biți `FOREGROUND_*` și `BACKGROUND_*`.
- Toate caracterele tipărite **după** acest apel vor apărea în culoarea setată.

**Tabel culori folosite:**

| Constantă          | Valoare | Culoare vizuală | Folosit pentru     |
|--------------------|---------|-----------------|---------------------|
| `CULOARE_NORMAL`   | 7       | Gri             | Text implicit       |
| `CULOARE_ALB`      | 15      | Alb intens      | Puncte `'.'`        |
| `CULOARE_GALBEN`   | 14      | Galben intens   | Pac-Man `'C'`, scor |
| `CULOARE_ALBASTRU` | 9       | Albastru intens | Pereți `'#'`        |
| `CULOARE_ROSU`     | 12      | Roșu intens     | Fantomă 1, Game Over|
| `CULOARE_MAGENTA`  | 13      | Magenta intens  | Fantomă 2           |
| `CULOARE_VERDE`    | 10      | Verde intens    | Scor, Victorie      |
| `CULOARE_CYAN`     | 11      | Cyan intens     | Header jucător      |

---

### `SetConsoleCursorPosition` — poziționare cursor

```c
void mutaCursor(int x, int y) {
    COORD pos = {(SHORT)x, (SHORT)y};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}
```

- Structura `COORD` specifică coloana (`X`) și linia (`Y`).
- `deseneazaHarta()` apelează `mutaCursor(0, 0)` la fiecare cadru pentru a **suprascrie** ecranul în loc să deruleze terminalul — efect de „double buffering" primitiv.

---

### `FillConsoleOutputCharacter` / `FillConsoleOutputAttribute` — ștergere ecran

```c
void curatEcran(void) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD count;
    COORD homeCoords = {0, 0};
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    DWORD cellCount = csbi.dwSize.X * csbi.dwSize.Y;
    FillConsoleOutputCharacter(hConsole, ' ', cellCount, homeCoords, &count);
    FillConsoleOutputAttribute(hConsole, csbi.wAttributes, cellCount, homeCoords, &count);
    SetConsoleCursorPosition(hConsole, homeCoords);
}
```

- Umple **întregul buffer** cu spații, fără a apela `system("cls")` (mai rapid, fără flickering).

---

### `GetConsoleCursorInfo` / `SetConsoleCursorInfo` — vizibilitate cursor

```c
CONSOLE_CURSOR_INFO cci;
GetConsoleCursorInfo(hCon, &cci);
cci.bVisible = 0;  // 0 = ascuns, 1 = vizibil
SetConsoleCursorInfo(hCon, &cci);
```

- Cursorul este ascuns pe durata jocului pentru a elimina clipirea vizuală.
- Este restaurat la terminarea jocului (Game Over sau victorie).

---

## 3. Mișcarea asincronă

Jocul folosește un model **non-blocant** pentru citirea tastaturii, combinat cu un sleep fix, simulând o buclă de joc cu tick constant.

### Tehnica `_kbhit()` + `_getch()`

```c
while (1) {                     // bucla principala
    if (_kbhit()) {             // verifica NON-BLOCANT daca s-a apasat o tasta
        tasta = _getch();       // citeste tasta FARA a bloca executia
        // ... actualizeaza directia Pac-Man
    }

    // ... misca Pac-Man
    // ... misca fantomele
    // ... redeseneaza scena

    Sleep(120);                 // asteapta 120ms → ~8 cadre/secunda
}
```

- `_kbhit()` (din `<conio.h>`) returnează non-zero dacă există o tastă în buffer, **fără a bloca** execuția.
- `_getch()` preia tasta din buffer **fără ecou** în consolă.
- Tastele speciale (săgeți) trimit **2 octeți**: primul este `0` sau `224`, al doilea identifică tasta (`72` = sus, `80` = jos, `75` = stânga, `77` = dreapta).

### De ce nu este blocat jocul?

Fără `_kbhit()`, apelul `_getch()` ar **bloca** firul de execuție până la o apăsare de tastă — Pac-Man și fantomele s-ar opri. Verificarea prealabilă cu `_kbhit()` permite ca logica de joc să avanseze la fiecare ciclu indiferent de input.

### Viteza fantomelor vs. Pac-Man

```c
cadru++;
if (cadru % 2 == 0) {          // fantomele se misca din 2 in 2 cadre
    miscaFantome(fantome, NR_FANTOME, &harta);
}
```

- Pac-Man se mișcă la fiecare cadru (120ms).
- Fantomele se mișcă o dată la 2 cadre (240ms) — oferind jucătorului un avantaj de reacție.

### Persistența direcției

Direcția Pac-Man **nu se resetează** după fiecare mișcare:
```c
pacman.directie = DIR_SUS; // ramane activa pana la o alta tasta
```
Astfel, jucătorul apasă o singură dată o tastă și Pac-Man continuă în acea direcție până la o nouă comandă sau până lovește un perete.

---

## 4. Modulele proiectului

| Fișier          | Responsabilitate                                             |
|-----------------|--------------------------------------------------------------|
| `ui.h`          | Header global: structuri, constante, prototipuri             |
| `main.c`        | Logo, loading screen, slider vârstă, meniu principal         |
| `game.c`        | Hartă, culori, fantome, salvare scor, buclă de joc           |
| `clasament.txt` | Fișier text generat automat cu scorurile jucătorilor         |

---

## 5. Compilare și rulare

```bash
# Windows (MinGW)
gcc main.c game.c -o pacman.exe
pacman.exe

# Linux (recomandat)
gcc -std=c11 -O2 -Wall -Wextra main.c game.c -o pacman
./pacman
```

> **Notă:** Codul este portabil pe Windows și Linux.
> Pe Windows folosește API Win32 (`windows.h`, `conio.h`), iar pe Linux folosește terminal raw mode (`termios`) + secvențe ANSI pentru culori/cursor.

---

## 6. Ghid laborator Debian 12

Pașii de mai jos sunt cei recomandați pentru afișare corectă (UTF-8 + culori ANSI) în laborator:

### 6.1 Cum deschizi proiectul

1. Deschide un terminal grafic (GNOME Terminal / Konsole / xfce4-terminal).
2. Intră în folderul proiectului.
3. Verifică rapid locale-ul activ:

```bash
locale
```

Ideal este să vezi valori de tip `C.UTF-8` sau `en_US.UTF-8`.

### 6.2 Compilare pe Debian 12

```bash
gcc -std=c11 -O2 -Wall -Wextra main.c game.c -o pacman
```

### 6.3 Rulare

```bash
./pacman
```

### 6.4 Dacă simbolurile UTF-8 nu se văd corect

Dacă pereții `█` sau punctele `·` apar stricat, rulează înainte:

```bash
export LANG=C.UTF-8
export LC_ALL=C.UTF-8
./pacman
```

### 6.5 Observații tehnice

- Inițializarea consolei pe Linux este tratată în cod în `initConsoleUtf8Ansi()`.
- În Linux se folosesc secvențe ANSI pentru culori/cursor și `termios` pentru input non-blocant.
- Dacă locale-ul curent nu poate fi setat, codul face fallback automat la `C.UTF-8`.
