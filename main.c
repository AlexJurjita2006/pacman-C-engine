/* ============================================================
   main.c - Punct de intrare Pac-Man (UI + meniu)
   Compilare: gcc main.c game.c -o pacman.exe
    Sistem: Windows / Linux
   ============================================================ */

#include "ui.h"
#include <locale.h>

static int g_ansiEnabled = 1;

/* ============================================================
   UTILITARE CONSOLA
   ============================================================ */

void sleepMs(int ms) {
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    usleep((useconds_t)ms * 1000);
#endif
}

void setConsoleTitleCompat(const char *title) {
#ifdef _WIN32
    SetConsoleTitleA(title);
#else
    printf("\033]0;%s\007", title);
    fflush(stdout);
#endif
}

void initConsoleUtf8Ansi(void) {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hIn  = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    if (GetConsoleMode(hOut, &mode)) {
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        if (!SetConsoleMode(hOut, mode)) {
            g_ansiEnabled = 0;
        }
    } else {
        g_ansiEnabled = 0;
    }
    if (GetConsoleMode(hIn, &mode)) {
        mode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
        SetConsoleMode(hIn, mode);
    }
#endif
#ifdef _WIN32
    setlocale(LC_ALL, ".UTF-8");
#else
    if (setlocale(LC_ALL, "") == NULL) {
        setlocale(LC_ALL, "C.UTF-8");
    }
#endif
}

/* Curata ecranul folosind API-ul Windows */
void curatEcran(void) {
#ifdef _WIN32
    if (!g_ansiEnabled) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD count, cellCount;
    COORD homeCoords = {0, 0};

    if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) return;
    cellCount = csbi.dwSize.X * csbi.dwSize.Y;

    /* Umple tot ecranul cu spatii */
    FillConsoleOutputCharacter(hConsole, ' ', cellCount, homeCoords, &count);
    FillConsoleOutputAttribute(hConsole, csbi.wAttributes, cellCount, homeCoords, &count);

    /* Muta cursorul in coltul stanga-sus */
    SetConsoleCursorPosition(hConsole, homeCoords);
        return;
    }
#endif
    printf("\033[2J\033[H");
    fflush(stdout);
}

/* Muta cursorul la coloana x, linia y */
void mutaCursor(int x, int y) {
#ifdef _WIN32
    if (!g_ansiEnabled) {
    COORD pos = {(SHORT)x, (SHORT)y};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
        return;
    }
#endif
    printf("\033[%d;%dH", y + 1, x + 1);
    fflush(stdout);
}

/* Ascunde / arata cursorul consolei */
void setCursorVisible(int vizibil) {
#ifdef _WIN32
    if (!g_ansiEnabled) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;
    GetConsoleCursorInfo(hConsole, &info);
    info.bVisible = vizibil;
    SetConsoleCursorInfo(hConsole, &info);
        return;
    }
#endif
    printf(vizibil ? "\033[?25h" : "\033[?25l");
    fflush(stdout);
}

#ifndef _WIN32
static struct termios g_terminalInitial;
static int g_terminalConfigured = 0;
static int g_queuedKey = -1;

void initLinuxTerminal(void) {
    if (g_terminalConfigured) {
        return;
    }

    if (tcgetattr(STDIN_FILENO, &g_terminalInitial) != 0) {
        return;
    }

    struct termios raw = g_terminalInitial;
    raw.c_lflag &= (tcflag_t)~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    g_terminalConfigured = 1;
}

void restoreLinuxTerminal(void) {
    if (!g_terminalConfigured) {
        return;
    }
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_terminalInitial);
    g_terminalConfigured = 0;
}

static int mapEscapeToArrow(char code) {
    if (code == 'A') return 72;
    if (code == 'B') return 80;
    if (code == 'D') return 75;
    if (code == 'C') return 77;
    return -1;
}

int _kbhit(void) {
    if (g_queuedKey != -1) {
        return 1;
    }

    fd_set fds;
    struct timeval tv;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
}

int _getch(void) {
    if (g_queuedKey != -1) {
        int out = g_queuedKey;
        g_queuedKey = -1;
        return out;
    }

    unsigned char ch = 0;
    while (read(STDIN_FILENO, &ch, 1) != 1) {
        usleep(1000);
    }

    if (ch == 27) {
        unsigned char seq[2] = {0, 0};
        if (read(STDIN_FILENO, &seq[0], 1) == 1 && seq[0] == '[' &&
            read(STDIN_FILENO, &seq[1], 1) == 1) {
            int arrow = mapEscapeToArrow((char)seq[1]);
            if (arrow != -1) {
                g_queuedKey = arrow;
                return 224;
            }
        }
    }

    return (int)ch;
}
#endif

/* ============================================================
   LOGO SI LOADING SCREEN
   ============================================================ */

void afiseazaLogo(void) {
    static const char *titlu[] = {
        "██████╗  █████╗  ██████╗      ███╗   ███╗ █████╗ ███╗   ██╗",
        "██╔══██╗██╔══██╗██╔════╝      ████╗ ████║██╔══██╗████╗  ██║",
        "██████╔╝███████║██║           ██╔████╔██║███████║██╔██╗ ██║",
        "██╔═══╝ ██╔══██║██║           ██║╚██╔╝██║██╔══██║██║╚██╗██║",
        "██║     ██║  ██║╚██████╗      ██║ ╚═╝ ██║██║  ██║██║ ╚████║",
        "╚═╝     ╚═╝  ╚═╝ ╚═════╝      ╚═╝     ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝"
    };
    int line;

    setCursorVisible(0);
    curatEcran();
    printf("\n");
    for (line = 0; line < (int)(sizeof(titlu) / sizeof(titlu[0])); line++) {
        printf("  \033[1;38;5;%dm%s\033[0m\n", 45 + line * 5, titlu[line]);
    }
    printf("\n  \033[1;38;5;214mMeniu principal\033[0m\n");
    printf("  \033[38;5;118m• Introdu username\n");
    printf("  • Selecteaza varsta\n");
    printf("  • Incepe jocul\033[0m\n\n");
    fflush(stdout);
    sleepMs(450);
    setCursorVisible(1);
}

/* ============================================================
   SLIDER INTERACTIV PENTRU VARSTA
   ============================================================ */

/* Deseneaza bara slider-ului pe linia curenta.
   Formatul: [----<valoare>----]
   Sagetile stanga/dreapta modifica valoarea.             */
static void deseneazaSlider(int valoare) {
    /* Numarul de caractere din bara (fara paranteze) */
    #define LATIME_BARA 30

    printf("\r   [");

    /* Calculeaza pozitia capului (0 .. LATIME_BARA-1) */
    int pozitie = (int)(((float)(valoare - VARSTA_MIN) /
                         (float)(VARSTA_MAX - VARSTA_MIN)) * (LATIME_BARA - 1));

    int j;
    for (j = 0; j < LATIME_BARA; j++) {
        if (j == pozitie) {
            /* Afiseaza valoarea centrata in mijlocul slider-ului */
            if (valoare < 10) {
                /* 1 cifra */
                printf(" %d ", valoare);
                j += 2; /* am scris 3 caractere */
            } else {
                /* 2 cifre */
                printf("%d", valoare);
                j += 1; /* am scris 2 caractere */
            }
        } else {
            printf("-");
        }
    }

    printf("] ");
    /* Afiseaza si valoarea numerica explicit langa bara */
    printf("Varsta: %2d ani   ", valoare);
    fflush(stdout);
}

/* ============================================================
   CITIRE DATE UTILIZATOR
   ============================================================ */

DateUtilizator citesteDateUtilizator(void) {
    DateUtilizator date;
    memset(&date, 0, sizeof(date));

    curatEcran();

    printf("\n");
    printf("   +-----------------------------------------+\n");
    printf("   |        INREGISTRARE JUCATOR             |\n");
    printf("   +-----------------------------------------+\n\n");

    /* --- Username --- */
    printf("   Introdu username-ul tau: ");
    fflush(stdout);

    /* Citim cu fgets pentru a prinde si spatii */
    if (fgets(date.username, MAX_USERNAME, stdin) != NULL) {
        /* Eliminam newline-ul de la sfarsit */
        size_t len = strlen(date.username);
        if (len > 0 && date.username[len - 1] == '\n') {
            date.username[len - 1] = '\0';
        }
    }

    /* Daca username-ul e gol, punem un default */
    if (strlen(date.username) == 0) {
        strcpy(date.username, "Jucator");
    }

    printf("\n");
    printf("   Salut, %s!\n\n", date.username);

    /* --- Slider varsta --- */
    printf("   Selecteaza varsta ta cu sagetile <- -> :\n");
    printf("   (Apasa ENTER pentru confirmare)\n\n");

    /* Ascunde cursorul pentru un slider curat vizual */
    setCursorVisible(0);

    int varsta = 18; /* valoare implicita */
    deseneazaSlider(varsta);

    int tasta;
    while (1) {
        tasta = _getch(); /* citeste o tasta fara ecou */

        /* Tastele speciale (sageti) trimit doua coduri:
           primul e 0 sau 224, al doilea identifica tasta */
        if (tasta == 0 || tasta == 224) {
            tasta = _getch(); /* al doilea byte */

            if (tasta == 75) {
                /* Sageata STANGA - scade varsta */
                if (varsta > VARSTA_MIN) varsta--;
            } else if (tasta == 77) {
                /* Sageata DREAPTA - creste varsta */
                if (varsta < VARSTA_MAX) varsta++;
            } else if (tasta == 72) {
                /* Sageata SUS - creste cu 5 */
                varsta += 5;
                if (varsta > VARSTA_MAX) varsta = VARSTA_MAX;
            } else if (tasta == 80) {
                /* Sageata JOS - scade cu 5 */
                varsta -= 5;
                if (varsta < VARSTA_MIN) varsta = VARSTA_MIN;
            }

            deseneazaSlider(varsta);

        } else if (tasta == 13) {
            /* ENTER - confirma selectia */
            break;
        }
    }

    date.varsta = varsta;

    setCursorVisible(1); /* reafiseaza cursorul */
    printf("\n   Ai ales varsta: %d ani\n", date.varsta);
    sleepMs(800);

    return date;
}

/* ============================================================
   MAIN
   ============================================================ */

int main(void) {
    initConsoleUtf8Ansi();

    /* Setam titlul ferestrei consolei */
    setConsoleTitleCompat("PAC-MAN");

#ifndef _WIN32
    initLinuxTerminal();
    atexit(restoreLinuxTerminal);
#endif

    /* 1. Afiseaza logo + loading screen */
    afiseazaLogo();

    /* 2. Citeste datele utilizatorului */
    DateUtilizator jucator = citesteDateUtilizator();

    /* 3. Rezumat inainte de a intra in joc */
    curatEcran();
    printf("\n\n");
    printf("   +-----------------------------------------+\n");
    printf("   |              BINE AI VENIT!             |\n");
    printf("   +-----------------------------------------+\n\n");
    printf("   Jucator : %s\n", jucator.username);
    printf("   Varsta  : %d ani\n\n", jucator.varsta);
    printf("   Apasa orice tasta pentru a incepe jocul...\n\n");
    _getch();

    /* 3. Porneste jocul (salveaza scorul automat la final) */
    ruleazaJoc(jucator);

    return 0;
}
