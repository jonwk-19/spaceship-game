// Juego.cpp (Windows Console) - Refactor pro
#include <stdio.h>
#include <windows.h>
#include <conio.h>
#include <time.h>

#define MAX_ENEMIES 60

// -------------------- CONFIG --------------------
const int WIDTH  = 80;   // ancho del área
const int HEIGHT = 25;   // alto del área

const int HUD_Y = 0;     // fila del HUD (arriba)
const int PLAY_MIN_X = 1;
const int PLAY_MIN_Y = 2;              // dejamos fila 1 para borde superior
const int PLAY_MAX_X = WIDTH - 2;      // borde en WIDTH-1
const int PLAY_MAX_Y = HEIGHT - 2;     // borde en HEIGHT-1

// nave dibuja
const int SHIP_W = 6;
const int SHIP_H = 2;

// disparos
const int START_SHOTS = 20;

// power-up
const int POWERUP_BONUS = 5;
const int POWERUP_SPAWN_TICKS = 250;

// limits
const int BULLET_MIN_X = PLAY_MIN_X + 2;
const int BULLET_MAX_X = PLAY_MAX_X - 2;

// colors
const int C_RESET   = 7;   // gris claro (default)
const int C_BORDER  = 8;   // gris oscuro
const int C_SHIP    = 15;  // blanco brillante
const int C_SHIP_T  = 11;  // cian brillante
const int C_ENEMY_O = 13;  // amarillo (tipo naranja)
const int C_ENEMY_X = 12;  // rojo brillante
const int C_POWER_1 = 10;  // verde brillante
const int C_POWER_2 = 2;   // verde oscuro (para parpadeo)
const int C_BULLET  = 15;  // blanco brillante
const int C_HIT     = 13;  // magenta (feedback)

// -------------------- STRUCTS --------------------
struct Enemy {
    int x, y;
    int hp;
    bool alive;
    char ch;     // 'O' o 'X'
    int points;  // puntos al destruir
};

struct PowerUp {
    int x, y;
    bool active;
    char ch; // '+'
};

// -------------------- UTILIDADES CONSOLA --------------------
void pos_xy(int x, int y) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD coor; coor.X = (SHORT)x; coor.Y = (SHORT)y;
    SetConsoleCursorPosition(h, coor);
}

void hideCursor() {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cur;
    cur.dwSize = 1;
    cur.bVisible = FALSE;
    SetConsoleCursorInfo(h, &cur);
}

void clearScreen() {
    system("cls");
}

void setColor(int color) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(h, (WORD)color);
}

// -------------------- DIBUJOS --------------------
void drawBorders() {
    for (int x = 0; x < WIDTH; x++) {
        pos_xy(x, 1);           putchar('#');
        pos_xy(x, HEIGHT - 1);  putchar('#');
    }

    for (int y = 1; y < HEIGHT; y++) {
        pos_xy(0, y);           putchar('#');
        pos_xy(WIDTH - 1, y);   putchar('#');
    }
}

void drawShip(int x, int y) {
    // y = fila superior de la nave
    // línea 1: "  /^\ "
    pos_xy(x, y);
    setColor(C_SHIP);
    putchar(' ');
    putchar(' ');
    putchar('/');
    putchar('^');
    putchar('\\');
    putchar(' ');

    // línea 2: " <_T_>"
    pos_xy(x, y + 1);
    setColor(C_SHIP);
    putchar(' ');
    putchar('<');
    putchar('_');

    setColor(C_SHIP_T);
    putchar('T');

    setColor(C_SHIP);
    putchar('_');
    putchar('>');
    setColor(C_RESET);
}

void eraseShip(int x, int y) {
    for (int row = 0; row < SHIP_H; row++) {
        pos_xy(x, y + row);
        for (int i = 0; i < SHIP_W; i++) putchar(' ');
    }
}

void drawEnemy(const Enemy &e) {
    if (!e.alive) return;

    if (e.ch == 'X') setColor(C_ENEMY_X);
    else             setColor(C_ENEMY_O);

    pos_xy(e.x, e.y);
    putchar(e.ch);

    setColor(C_RESET);
}

void eraseEnemy(const Enemy &e) {
    pos_xy(e.x, e.y);
    putchar(' ');
}

void explosion(int x, int y) {
    // mini animación
    pos_xy(x, y); putchar('*');
    Sleep(40);
    pos_xy(x, y); putchar('@');
    Sleep(40);
    pos_xy(x, y); putchar(' ');
}

void drawHUD(int score, int shots, int level, int aliveEnemies) {
    // HUD arriba (y=0)
    pos_xy(0, HUD_Y);
    // limpiamos la fila completa
    for (int i = 0; i < WIDTH; i++) putchar(' ');
    pos_xy(0, HUD_Y);
    printf("Puntaje: %d   Disparos: %d   Nivel: %d   Enemigos: %d   (ESC = salir)",
           score, shots, level, aliveEnemies);
}

void drawPowerUp(const PowerUp &p, int color) {
    if (!p.active) return;
    setColor(color);
    pos_xy(p.x, p.y);
    putchar(p.ch);
    setColor(C_RESET);
}

void erasePowerUp(const PowerUp &p) {
    pos_xy(p.x, p.y);
    putchar(' ');
}

// -------------------- LOGICA ENEMIGOS --------------------
int enemiesAlive(Enemy enemies[], int count) {
    int alive = 0;
    for (int i = 0; i < count; i++) if (enemies[i].alive) alive++;
    return alive;
}

bool positionOccupiedByEnemy(Enemy enemies[], int count, int x, int y) {
    for (int i = 0; i < count; i++) {
        if (enemies[i].alive && enemies[i].x == x && enemies[i].y == y) return true;
    }
    return false;
}

void spawnEnemies(Enemy enemies[], int &count, int level) {
    // nivel sube: más enemigos
    count = 0;
    int target = 6 + (level * 3);
    if (target > MAX_ENEMIES) target = MAX_ENEMIES;

    for (int i = 0; i < target; i++) {
        Enemy e;
        // random en parte alta
        e.x = (rand() % (BULLET_MAX_X - BULLET_MIN_X + 1)) + BULLET_MIN_X;
        e.y = (rand() % 8) + PLAY_MIN_Y; // y entre 2 y 9 aprox

        // tipos: 70% 'O' (1 vida), 30% 'X' (2 vidas)
        int r = rand() % 100;
        if (r < 70) {
            e.ch = 'O'; e.hp = 1; e.points = 10;
        } else {
            e.ch = 'X'; e.hp = 2; e.points = 25;
        }

        // evitar spawnear encima de otro enemigo
        while (positionOccupiedByEnemy(enemies, count, e.x, e.y)) {
            e.x = (rand() % (PLAY_MAX_X - PLAY_MIN_X + 1)) + PLAY_MIN_X;
            e.y = (rand() % 8) + PLAY_MIN_Y;
        }

        e.alive = true;
        enemies[count++] = e;
    }
}

void drawAllEnemies(Enemy enemies[], int count) {
    for (int i = 0; i < count; i++) drawEnemy(enemies[i]);
}

// -------------------- POWER UP --------------------
bool positionOccupied(Enemy enemies[], int enemyCount, const PowerUp &p, int x, int y) {
    if (p.active && p.x == x && p.y == y) return true;
    if (positionOccupiedByEnemy(enemies, enemyCount, x, y)) return true;
    return false;
}

void trySpawnPowerUp(PowerUp &p, Enemy enemies[], int enemyCount) {
    if (p.active) return;

    // 30% de probabilidad cuando toca intentar
    if ((rand() % 100) >= 30) return;

    int x = (rand() % (BULLET_MAX_X - BULLET_MIN_X + 1)) + BULLET_MIN_X;
    int y = (rand() % (PLAY_MAX_Y - PLAY_MIN_Y + 1)) + PLAY_MIN_Y;

    // evitar spawnear encima de enemigos
    if (positionOccupied(enemies, enemyCount, p, x, y)) return;

    p.x = x;
    p.y = y;
    p.active = true;
    p.ch = '+';
    drawPowerUp(p, C_POWER_1);
}

bool shipTouchesPowerUp(int shipX, int shipY, const PowerUp &p) {
    if (!p.active) return false;

    // power-up puede tocar la fila superior o inferior de la nave
    bool yMatch = (p.y == shipY) || (p.y == shipY + 1);
    if (!yMatch) return false;

    // x dentro del ancho
    if (p.x >= shipX && p.x < shipX + SHIP_W) return true;

    return false;
}

// -------------------- DISPARO + COLISIONES --------------------
int handleBulletHit(Enemy enemies[], int enemyCount, int bx, int by, int &score) {
    for (int i = 0; i < enemyCount; i++) {
        if (!enemies[i].alive) continue;

        if (enemies[i].x == bx && enemies[i].y == by) {
            enemies[i].hp--;

            if (enemies[i].hp <= 0) {
                enemies[i].alive = false;
                score += enemies[i].points;
                explosion(bx, by);
                eraseEnemy(enemies[i]);
                return 2;
            } else {
                setColor(C_HIT);
                pos_xy(bx, by); putchar('@');
                setColor(C_RESET);
                Sleep(40);
                pos_xy(bx, by); putchar(enemies[i].ch);
                return 1;
            }
        }
    }
    return 0;
}

void shoot(int shipX, int shipY, Enemy enemies[], int enemyCount, int &score) {
    int bx = shipX + 3;          // el cañón
    int startY = shipY;          // shipY es la línea superior, el disparo sale arriba del cuerpo

    for (int y = startY; y >= PLAY_MIN_Y; y--) {
        setColor(C_BULLET);
        pos_xy(bx, y);
        putchar('*');
        setColor(C_RESET);
        Sleep(15);

        int hit = handleBulletHit(enemies, enemyCount, bx, y, score);
        if (hit != 0) {
            if (hit == 2) {
            } else {
            }
            break;
        }

        pos_xy(bx, y);
        putchar(' ');
    }

    drawShip(shipX, shipY);
}

// -------------------- INPUT / MOVIMIENTO --------------------
void clampShip(int &x, int &y) {
    if (x < PLAY_MIN_X) x = PLAY_MIN_X;
    if (y < PLAY_MIN_Y) y = PLAY_MIN_Y;

    int maxX = PLAY_MAX_X - (SHIP_W - 1);
    if (x > maxX) x = maxX;
    int maxY = PLAY_MAX_Y - (SHIP_H - 1);
    if (y > maxY) y = maxY;
}

void updateShipByKey(int &x, int &y, char key) {
    if (key == 'a') x--;
    if (key == 'd') x++;
    if (key == 'w') y--;
    if (key == 's') y++;
    clampShip(x, y);
}

// -------------------- PANTALLAS --------------------
void pantallaInstrucciones() {
    clearScreen();
    hideCursor();
    printf("INSTRUCCIONES\n");
    printf("-------------\n");
    printf("W A S D : mover nave\n");
    printf("ESPACIO: disparar\n");
    printf("ESC    : salir al menu\n");
    printf("\nObjetivo:\n");
    printf("- Destruir enemigos 'O' (1 vida) y 'X' (2 vidas)\n");
    printf("- Cada disparo consume 1\n");
    printf("- Si te quedas sin disparos y aun hay enemigos -> GAME OVER\n");
    printf("- Power-up '+' te da +%d disparos si lo tocas\n", POWERUP_BONUS);
    printf("\nPresiona cualquier tecla para volver...\n");
    getch();
}

// -------------------- JUEGO PRINCIPAL --------------------
void iniciarJuego() {
    clearScreen();
    hideCursor();
    srand((unsigned)time(NULL));

    int score = 0;
    int level = 1;
    int shots = START_SHOTS;

    // nave
    int shipX = 20, shipY = 20;
    clampShip(shipX, shipY);

    // enemigos
    Enemy enemies[MAX_ENEMIES];
    int enemyCount = 0;
    spawnEnemies(enemies, enemyCount, level);

    // powerup
    PowerUp p;
    p.active = false;
    p.ch = '+';

    // dibujar escenario
    drawBorders();
    drawShip(shipX, shipY);
    drawAllEnemies(enemies, enemyCount);

    int tick = 0;
    bool fin = false;

    while (!fin) {
        int alive = enemiesAlive(enemies, enemyCount);
        drawHUD(score, shots, level, alive);

        // win -> siguiente nivel
        if (alive == 0) {
            level++;
            shots += 5; // mini premio
            // limpiar zona y respawn
            clearScreen();
            drawBorders();
            p.active = false;
            spawnEnemies(enemies, enemyCount, level);
            drawAllEnemies(enemies, enemyCount);
            clampShip(shipX, shipY);
            drawShip(shipX, shipY);
        }

        // game over por disparos
        if (shots <= 0 && alive > 0) {
            fin = true;
            break;
        }

        // power-up: cada cierto tiempo intenta aparecer
        tick++;
        if (tick % POWERUP_SPAWN_TICKS == 0) {
            trySpawnPowerUp(p, enemies, enemyCount);
        }

        // Parpadeo del power-up
        if (p.active) {
            int powerColor = (tick % 20 < 10) ? C_POWER_1 : C_POWER_2;
            drawPowerUp(p, powerColor);
        }

        // input
        if (kbhit()) {
          char key = getch();

          if (key == 27) { // ESC
              fin = true;
              break;
          }

          // MOVIMIENTO
          if (key == 'w' || key == 'a' || key == 's' || key == 'd') {
              eraseShip(shipX, shipY);
              updateShipByKey(shipX, shipY, key);
              drawShip(shipX, shipY);
          }

          // DISPARO
          if (key == 32) {
              if (shots > 0) {
                  shots--;
                  shoot(shipX, shipY, enemies, enemyCount, score);
              }
          }

          // POWERUP (después de mover)
          if (shipTouchesPowerUp(shipX, shipY, p)) {
              shots += POWERUP_BONUS;
              score += 5;
              erasePowerUp(p);
              p.active = false;
          }
        }

        Sleep(25);
    }

    // pantalla final
    clearScreen();
    hideCursor();

    int alive = enemiesAlive(enemies, enemyCount);
    if (shots <= 0 && alive > 0) {
        printf("GAME OVER\n");
        printf("Te quedaste sin disparos.\n\n");
    } else {
        printf("SALISTE DEL JUEGO\n\n");
    }
    printf("Puntaje final: %d\n", score);
    printf("Nivel alcanzado: %d\n", level);
    printf("Enemigos restantes: %d\n\n", alive);
    printf("Presiona cualquier tecla para volver al menu...\n");
    getch();
}

void menuPrincipal() {
    bool salir = false;
    while (!salir) {
        clearScreen();
        hideCursor();
        printf("=========== MENU ===========\n");
        printf("1) Jugar\n");
        printf("2) Instrucciones\n");
        printf("3) Salir\n");
        printf("============================\n");
        printf("Elige una opcion: ");

        char op = getch();
        if (op == '1') iniciarJuego();
        else if (op == '2') pantallaInstrucciones();
        else if (op == '3') salir = true;
    }
}

// -------------------- MAIN --------------------
int main() {
    menuPrincipal();
    return 0;
}
