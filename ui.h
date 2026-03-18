#ifndef UI_H
#define UI_H

/* ============================================================
   ui.h - Header global Pac-Man
   Contine toate structurile, constantele si prototipurile.
   ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#endif

/* ---- Dimensiuni si limite ---- */
#define MAX_USERNAME  32
#define VARSTA_MIN     1
#define VARSTA_MAX    99
#define MAP_ROWS      11
#define MAP_COLS      20
#define NR_FANTOME     2

/* ---- Celule logice in matricea hartii ---- */
#define CELULA_PERETE '#'
#define CELULA_PUNCT  '.'
#define CELULA_GOL    ' '

/* ---- Simboluri UTF-8 pentru randare ---- */
#define UTF8_PERETE "\xE2\x96\x88 "
#define UTF8_PUNCT  "\xC2\xB7 "
#define UTF8_GOL    "  "

/* ---- Directii de miscare ---- */
#define DIR_STOP    0
#define DIR_SUS     1
#define DIR_JOS     2
#define DIR_STANGA  3
#define DIR_DREAPTA 4

/* ---- Coduri culori consola Windows ----
   Valoarea = combinatie de biti FOREGROUND_* din windows.h  */
#define CULOARE_NORMAL      7   /* gri implicit                  */
#define CULOARE_ALB        15   /* alb intens                    */
#define CULOARE_GALBEN     14   /* rosu + verde intens           */
#define CULOARE_ALBASTRU    9   /* albastru intens               */
#define CULOARE_ROSU       12   /* rosu intens                   */
#define CULOARE_CYAN       11   /* albastru + verde intens       */
#define CULOARE_VERDE      10   /* verde intens                  */
#define CULOARE_MAGENTA    13   /* rosu + albastru intens        */
#define CULOARE_PORTOCALIU 16   /* portocaliu ANSI (fantoma)     */

/* ============================================================
   STRUCTURI DE DATE
   ============================================================ */

/* Pac-Man: pozitie curenta, directie si scor acumulat */
typedef struct {
    int x;          /* coloana curenta (0 .. MAP_COLS-1) */
    int y;          /* linia curenta   (0 .. MAP_ROWS-1) */
    int directie;   /* una dintre constantele DIR_*      */
    int scor;       /* puncte acumulate                  */
} Pacman;

/* Harta: matricea de celule si numarul de puncte ramase */
typedef struct {
    char celule[MAP_ROWS][MAP_COLS + 1]; /* CELULA_* in matricea logica */
    int  nrPuncte;                       /* puncte ramase de mancat           */
} Harta;

/* Fantoma: pozitie, culoare de afisare si simbol */
typedef struct {
    int  x;       /* coloana curenta               */
    int  y;       /* linia curenta                 */
    int  culoare; /* cod culoare (CULOARE_*)        */
    char simbol;  /* caracterul afisat (implicit G) */
} Fantoma;

/* Date utilizator: username si varsta */
typedef struct {
    char username[MAX_USERNAME];
    int  varsta;
} DateUtilizator;

/* ============================================================
   PROTOTIPURI — main.c (UI / meniu)
   ============================================================ */
void           afiseazaLogo(void);
DateUtilizator citesteDateUtilizator(void);
void           curatEcran(void);
void           mutaCursor(int x, int y);
void           setCursorVisible(int vizibil);
void           sleepMs(int ms);
void           setConsoleTitleCompat(const char *title);
void           initConsoleUtf8Ansi(void);

#ifndef _WIN32
void initLinuxTerminal(void);
void restoreLinuxTerminal(void);
int  _kbhit(void);
int  _getch(void);
#endif

/* ============================================================
   PROTOTIPURI — game.c (logica de joc)
   ============================================================ */

/* Seteaza culoarea textului in consola */
void seteazaCuloare(int cod);

/* Initializeaza harta cu layout-ul implicit */
void initHarta(Harta *h);

/* Deseneaza harta, Pac-Man si fantomele pe ecran */
void deseneazaHarta(const Harta *h, const Pacman *p,
                    const Fantoma fantome[], int nrFantome,
                    const char *username, int cadru);

/* Misca fiecare fantoma aleatoriu (evita peretii) */
void miscaFantome(Fantoma fantome[], int nrFantome, const Harta *harta);

/* Scrie scorul in fisierul clasament.txt */
void salveazaScor(DateUtilizator jucator, int scor);

/* Bucla principala a jocului */
void ruleazaJoc(DateUtilizator jucator);

#endif /* UI_H */
