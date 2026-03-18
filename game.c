/* ============================================================
   game.c - Logica de joc Pac-Man
   Contine: initHarta, deseneazaHarta (cu culori),
            miscaFantome, salveazaScor, ruleazaJoc.
   Compilare (impreuna cu main.c):
       gcc main.c game.c -o pacman.exe
   ============================================================ */

#include "ui.h"
#include <stdarg.h>

/* ============================================================
   UTILITARA CULORI
   ============================================================ */

void seteazaCuloare(int cod) {
    const char *ansi = "\033[0m";
    switch (cod) {
        case CULOARE_ALB:        ansi = "\033[97m"; break;
        case CULOARE_GALBEN:     ansi = "\033[93m"; break;
        case CULOARE_ALBASTRU:   ansi = "\033[38;5;39m"; break;
        case CULOARE_ROSU:       ansi = "\033[91m"; break;
        case CULOARE_CYAN:       ansi = "\033[96m"; break;
        case CULOARE_VERDE:      ansi = "\033[92m"; break;
        case CULOARE_MAGENTA:    ansi = "\033[95m"; break;
        case CULOARE_PORTOCALIU: ansi = "\033[38;5;214m"; break;
        case CULOARE_NORMAL:
        default:                 ansi = "\033[0m"; break;
    }
    printf("%s", ansi);
}

static void appendf(char *buffer, size_t capacitate, size_t *lungime, const char *fmt, ...) {
    va_list args;
    int scris;
    if (*lungime >= capacitate) return;
    va_start(args, fmt);
    scris = vsnprintf(buffer + *lungime, capacitate - *lungime, fmt, args);
    va_end(args);
    if (scris <= 0) return;
    *lungime += (size_t)scris;
    if (*lungime >= capacitate) {
        *lungime = capacitate - 1;
    }
}

static const char *simbolPacman(const Pacman *p, int cadru) {
    if (cadru % 2 != 0) return "● ";
    if (p->directie == DIR_SUS) return "ᗨ ";
    if (p->directie == DIR_JOS) return "ᗪ ";
    if (p->directie == DIR_STANGA) return "ᗤ ";
    return "ᗧ ";
}

static const char *simbolCelulaUtf8(char celula) {
    if (celula == CELULA_PERETE) return UTF8_PERETE;
    if (celula == CELULA_PUNCT)  return UTF8_PUNCT;
    return UTF8_GOL;
}

static void efectIntrareCortina(void) {
    int y, x, pas;
    const int latime = MAP_COLS * 2 + 6;
    curatEcran();
    for (pas = 0; pas <= MAP_ROWS + 3; pas++) {
        mutaCursor(0, 0);
        for (y = 0; y < pas; y++) {
            int culoareFundal = 17 + (y % 6) * 3;
            printf("\033[48;5;%dm", culoareFundal);
            for (x = 0; x < latime; x++) printf(" ");
            printf("\033[0m\n");
        }
        fflush(stdout);
        sleepMs(35);
    }
    sleepMs(80);
    curatEcran();
}

/* ============================================================
   HARTA
   ============================================================ */

/* Layout initial: CELULA_PERETE perete, CELULA_PUNCT punct, CELULA_GOL liber.
   Zona centrala (randul 5) contine "casa" fantomelor.       */
static const char harta_initiala[MAP_ROWS][MAP_COLS + 1] = {
    "####################",
    "#........##........#",
    "#.##.###.##.###.##.#",
    "#..................#",
    "#.##.#.######.#.##.#",
    "#....#..    ..#....#",
    "#.##.#.######.#.##.#",
    "#..................#",
    "#.##.###.##.###.##.#",
    "#........##........#",
    "####################"
};

void initHarta(Harta *h) {
    int i, j;
    h->nrPuncte = 0;
    for (i = 0; i < MAP_ROWS; i++) {
        strcpy(h->celule[i], harta_initiala[i]);
        for (j = 0; j < MAP_COLS; j++) {
            if (h->celule[i][j] == CELULA_PUNCT) h->nrPuncte++;
        }
    }
}

/* ============================================================
   DESENARE HARTA CU CULORI
   ============================================================ */

void deseneazaHarta(const Harta *h, const Pacman *p,
                    const Fantoma fantome[], int nrFantome,
                    const char *username, int cadru) {
    int i, j, f;
    char buffer[32768];
    size_t lungime = 0;
    static const char *fantomaSimbol[4] = {"◕ ", "◔ ", "Ω ", "Ѫ "};
    static const char *fantomaCuloare[4] = {
        "\033[1;38;5;196m", "\033[1;38;5;201m", "\033[1;38;5;51m", "\033[1;38;5;214m"
    };
    const char *pac = simbolPacman(p, cadru);

    mutaCursor(0, 0);
    appendf(buffer, sizeof(buffer), &lungime,
            "\033[1;38;5;45m  Jucator:\033[0m \033[1;38;5;226m%-15s\033[0m  "
            "\033[1;38;5;51mScor:\033[0m \033[1;38;5;118m%5d\033[0m\n\n",
            username, p->scor);

    for (i = 0; i < MAP_ROWS; i++) {
        appendf(buffer, sizeof(buffer), &lungime, "  ");
        for (j = 0; j < MAP_COLS; j++) {
            if (j == p->x && i == p->y) {
                appendf(buffer, sizeof(buffer), &lungime, "\033[1;38;5;226m%s\033[0m", pac);
            } else {
                int eFantoma = 0;
                for (f = 0; f < nrFantome; f++) {
                    if (j == fantome[f].x && i == fantome[f].y) {
                        appendf(buffer, sizeof(buffer), &lungime, "%s%s\033[0m",
                                fantomaCuloare[f % 4], fantomaSimbol[f % 4]);
                        eFantoma = 1;
                        break;
                    }
                }
                if (!eFantoma) {
                    char c = h->celule[i][j];
                    if (c == CELULA_PERETE) {
                        appendf(buffer, sizeof(buffer), &lungime, "\033[38;5;33m%s\033[0m", simbolCelulaUtf8(c));
                    } else if (c == CELULA_PUNCT) {
                        appendf(buffer, sizeof(buffer), &lungime, "\033[38;5;229m%s\033[0m", simbolCelulaUtf8(c));
                    } else {
                        appendf(buffer, sizeof(buffer), &lungime, "%s", simbolCelulaUtf8(c));
                    }
                }
            }
        }
        appendf(buffer, sizeof(buffer), &lungime, "\n");
    }
    appendf(buffer, sizeof(buffer), &lungime,
            "\n  \033[38;5;117mMiscare:\033[0m WASD / Sageti  |  \033[38;5;203mIesire:\033[0m Q\n");
    printf("%s", buffer);
    fflush(stdout);
}

/* ============================================================
   MISCARE FANTOME
   ============================================================ */

/* Misca fiecare fantoma intr-o directie aleatorie valida.
   Se incearca pana la 10 directii; daca toate sunt blocate
   de pereti sau margini, fantoma ramane pe loc.             */
void miscaFantome(Fantoma fantome[], int nrFantome, const Harta *harta) {
    /* Deplasarile posibile: sus, jos, stanga, dreapta */
    static const int dx[4] = { 0,  0, -1, 1};
    static const int dy[4] = {-1,  1,  0, 0};

    int f, dir, nx, ny, incercari;
    for (f = 0; f < nrFantome; f++) {
        incercari = 0;
        do {
            dir = rand() % 4;
            nx  = fantome[f].x + dx[dir];
            ny  = fantome[f].y + dy[dir];
            incercari++;
        } while ((nx < 0 || nx >= MAP_COLS ||
                  ny < 0 || ny >= MAP_ROWS ||
                  harta->celule[ny][nx] == CELULA_PERETE) && incercari < 10);

        if (incercari < 10) {
            fantome[f].x = nx;
            fantome[f].y = ny;
        }
    }
}

/* ============================================================
   SALVARE SCOR
   ============================================================ */

/* Adauga o linie noua in clasament.txt cu formatul:
   Jucator: <username> | Varsta: <varsta> ani | Scor: <scor>  */
void salveazaScor(DateUtilizator jucator, int scor) {
    FILE *fp = fopen("clasament.txt", "a");
    if (fp == NULL) return; /* nu blocam jocul daca fisierul nu poate fi creat */
    fprintf(fp, "Jucator: %-20s | Varsta: %2d ani | Scor: %5d\n",
            jucator.username, jucator.varsta, scor);
    fclose(fp);
}

/* ============================================================
   BUCLA PRINCIPALA DE JOC
   ============================================================ */

void ruleazaJoc(DateUtilizator jucator) {
    Harta   harta;
    Pacman  pacman;
    Fantoma fantome[NR_FANTOME];
    int     tasta, nx, ny, f;
    int     gameOver   = 0;
    int     cadru      = 0; /* contor cadre pentru viteza fantomelor */

    /* Initializeaza generatorul de numere aleatoare */
    srand((unsigned int)time(NULL));

    /* -- Harta -- */
    initHarta(&harta);

    /* -- Pac-Man porneste in coltul stanga-sus -- */
    pacman.x        = 1;
    pacman.y        = 1;
    pacman.directie = DIR_STOP;
    pacman.scor     = 0;

    /* Mananca primul punct de pe pozitia de start */
    if (harta.celule[pacman.y][pacman.x] == CELULA_PUNCT) {
        harta.celule[pacman.y][pacman.x] = CELULA_GOL;
        pacman.scor += 10;
        harta.nrPuncte--;
    }

    /* -- Fantomele pornesc din centrul hartii --
       Culorile: prima = rosie, a doua = magenta.           */
    {
        int culori[4] = {CULOARE_ROSU, CULOARE_MAGENTA, CULOARE_CYAN, CULOARE_PORTOCALIU};
        for (f = 0; f < NR_FANTOME; f++) {
            fantome[f].x      = MAP_COLS / 2 + f;   /* col 10, 11 */
            fantome[f].y      = MAP_ROWS / 2;        /* rand 5     */
            fantome[f].culoare = culori[f % 4];
            fantome[f].simbol  = 'M';
            /* Fallback: daca celula e perete, muta o coloana la stanga */
            if (harta.celule[fantome[f].y][fantome[f].x] == CELULA_PERETE) {
                fantome[f].x = MAP_COLS / 2 - f;
            }
        }
    }

    /* -- Ascunde cursorul pentru o afisare curata -- */
    setCursorVisible(0);

    efectIntrareCortina();
    deseneazaHarta(&harta, &pacman, fantome, NR_FANTOME, jucator.username, cadru);

    /* ---- Bucla principala ---- */
    while (1) {

        /* -- Citire tasta (non-blocant) -- */
        if (_kbhit()) {
            tasta = _getch();
            if (tasta == 0 || tasta == 224) {
                tasta = _getch();
                if      (tasta == 72) pacman.directie = DIR_SUS;
                else if (tasta == 80) pacman.directie = DIR_JOS;
                else if (tasta == 75) pacman.directie = DIR_STANGA;
                else if (tasta == 77) pacman.directie = DIR_DREAPTA;
            } else {
                if      (tasta == 'w' || tasta == 'W') pacman.directie = DIR_SUS;
                else if (tasta == 's' || tasta == 'S') pacman.directie = DIR_JOS;
                else if (tasta == 'a' || tasta == 'A') pacman.directie = DIR_STANGA;
                else if (tasta == 'd' || tasta == 'D') pacman.directie = DIR_DREAPTA;
                else if (tasta == 'q' || tasta == 'Q') break;
            }
        }

        /* -- Misca Pac-Man -- */
        nx = pacman.x;
        ny = pacman.y;
        if      (pacman.directie == DIR_SUS)     ny--;
        else if (pacman.directie == DIR_JOS)     ny++;
        else if (pacman.directie == DIR_STANGA)  nx--;
        else if (pacman.directie == DIR_DREAPTA) nx++;

        if (nx >= 0 && nx < MAP_COLS && ny >= 0 && ny < MAP_ROWS
                && harta.celule[ny][nx] != CELULA_PERETE) {
            pacman.x = nx;
            pacman.y = ny;
            if (harta.celule[ny][nx] == CELULA_PUNCT) {
                harta.celule[ny][nx] = CELULA_GOL;
                pacman.scor += 10;
                harta.nrPuncte--;
            }
        }

        /* -- Misca fantomele din 2 in 2 cadre (viteza moderata) -- */
        cadru++;
        if (cadru % 2 == 0) {
            miscaFantome(fantome, NR_FANTOME, &harta);
        }

        /* -- Verifica coliziune Pac-Man cu fantome -- */
        for (f = 0; f < NR_FANTOME; f++) {
            if (pacman.x == fantome[f].x && pacman.y == fantome[f].y) {
                gameOver = 1;
                break;
            }
        }

        /* -- Redeseneaza scena -- */
        deseneazaHarta(&harta, &pacman, fantome, NR_FANTOME, jucator.username, cadru);

        /* -- Game Over -- */
        if (gameOver) {
            mutaCursor(0, MAP_ROWS + 4);
            seteazaCuloare(CULOARE_ROSU);
            printf("  *** GAME OVER! O fantoma te-a prins, %s! ***\n",
                   jucator.username);
            seteazaCuloare(CULOARE_GALBEN);
            printf("  Scor final: %d\n\n", pacman.scor);
            seteazaCuloare(CULOARE_NORMAL);
            sleepMs(3000);
            break;
        }

        /* -- Victorie: toate punctele mancate -- */
        if (harta.nrPuncte == 0) {
            mutaCursor(0, MAP_ROWS + 4);
            seteazaCuloare(CULOARE_VERDE);
            printf("  *** Felicitari, %s! Ai mancat toate punctele! ***\n",
                   jucator.username);
            seteazaCuloare(CULOARE_GALBEN);
            printf("  Scor final: %d\n\n", pacman.scor);
            seteazaCuloare(CULOARE_NORMAL);
            sleepMs(3000);
            break;
        }

        sleepMs(120); /* ~8 cadre/secunda */
    }

    /* -- Salveaza scorul in fisier -- */
    salveazaScor(jucator, pacman.scor);

    /* -- Reafiseaza cursorul -- */
    setCursorVisible(1);
}
