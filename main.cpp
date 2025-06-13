#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <cstdlib>

#define MAX_BULLETS 50
#define MAX_ENEMIES 50

enum GameScreen { MENU, GAMEPLAY, GAMEOVER };

struct Bullet {
    Vector2 position;
    Vector2 direction;
    bool active = false;
};

struct Enemy {
    Vector2 position;
    bool active = false;
};

class Command {
public:
    virtual void Execute(Vector2& playerPos, float playerSpeed) = 0;
    virtual ~Command() {}
};

class MoveUpCommand : public Command {
public:
    void Execute(Vector2& playerPos, float playerSpeed) override {
        playerPos.y -= playerSpeed;
    }
};

class MoveDownCommand : public Command {
public:
    void Execute(Vector2& playerPos, float playerSpeed) override {
        playerPos.y += playerSpeed;
    }
};

class MoveLeftCommand : public Command {
public:
    void Execute(Vector2& playerPos, float playerSpeed) override {
        playerPos.x -= playerSpeed;
    }
};

class MoveRightCommand : public Command {
public:
    void Execute(Vector2& playerPos, float playerSpeed) override {
        playerPos.x += playerSpeed;
    }
};

class InputHandler {
public:
    Command* HandleInput() {
        if (IsKeyDown(KEY_W)) return &moveUp;
        if (IsKeyDown(KEY_S)) return &moveDown;
        if (IsKeyDown(KEY_A)) return &moveLeft;
        if (IsKeyDown(KEY_D)) return &moveRight;
        return nullptr;
    }
private:
    MoveUpCommand moveUp;
    MoveDownCommand moveDown;
    MoveLeftCommand moveLeft;
    MoveRightCommand moveRight;
};

class BulletFactory {
public:
    static Bullet Create(Vector2 playerPos, Vector2 targetPos) {
        Bullet b;
        b.position = playerPos;
        b.direction = Vector2Normalize(Vector2Subtract(targetPos, playerPos));
        b.active = true;
        return b;
    }
};

class EnemyFactory {
public:
    static Enemy Create(int screenWidth, int screenHeight, Vector2 playerPos, float minDistance = 150.0f) {
        Enemy e;
        do {
            e.position = { (float)(rand() % screenWidth), (float)(rand() % screenHeight) };
        } while (Vector2Distance(e.position, playerPos) < minDistance);
        e.active = true;
        return e;
    }
};

int main() {
    const int screenWidth = 1080;
    const int screenHeight = 720;
    InitWindow(screenWidth, screenHeight, "GHOST SHOOTER");
    SetTargetFPS(60);

    Texture2D background = LoadTexture("background.png");
    Texture2D playerTex = LoadTexture("karakter.png");
    Texture2D enemyTex = LoadTexture("enemy.png");

    float playerScale = 0.15f;
    float enemyScale = 0.06f;

    GameScreen currentScreen = MENU;
    Vector2 playerPos = { screenWidth / 2.0f, screenHeight / 2.0f };
    float playerSpeed = 5.0f;

    Bullet bullets[MAX_BULLETS] = {};
    Enemy enemies[MAX_ENEMIES] = {};
    int score = 0;
    float enemySpawnTimer = 0.0f;
    float enemySpawnRate = 1.0f;

    InputHandler inputHandler;

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (currentScreen == MENU) {
            DrawTexture(background, 0, 0, WHITE);
            DrawText("GHOST SHOOTER", 220, 180, 40, DARKGRAY);
            DrawText("Press ENTER to Start", 280, 260, 20, DARKGRAY);
            if (IsKeyPressed(KEY_ENTER)) {
                currentScreen = GAMEPLAY;
                score = 0;
                playerPos = { screenWidth / 2.0f, screenHeight / 2.0f };
                for (int i = 0; i < MAX_ENEMIES; i++) enemies[i].active = false;
                for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = false;
            }
        }
        else if (currentScreen == GAMEPLAY) {
            Command* command = inputHandler.HandleInput();
            if (command) command->Execute(playerPos, playerSpeed);

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                for (int i = 0; i < MAX_BULLETS; i++) {
                    if (!bullets[i].active) {
                        bullets[i] = BulletFactory::Create(playerPos, GetMousePosition());
                        break;
                    }
                }
            }

            for (int i = 0; i < MAX_BULLETS; i++) {
                if (bullets[i].active) {
                    bullets[i].position = Vector2Add(bullets[i].position, Vector2Scale(bullets[i].direction, 10.0f));
                    if (bullets[i].position.x < 0 || bullets[i].position.x > screenWidth ||
                        bullets[i].position.y < 0 || bullets[i].position.y > screenHeight) {
                        bullets[i].active = false;
                    }
                }
            }

            enemySpawnTimer += GetFrameTime();
            if (enemySpawnTimer >= enemySpawnRate) {
                for (int i = 0; i < MAX_ENEMIES; i++) {
                    if (!enemies[i].active) {
                        enemies[i] = EnemyFactory::Create(screenWidth, screenHeight, playerPos);
                        break;
                    }
                }
                enemySpawnTimer = 0.0f;
            }

            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (enemies[i].active) {
                    Vector2 toPlayer = Vector2Subtract(playerPos, enemies[i].position);
                    enemies[i].position = Vector2Add(enemies[i].position, Vector2Scale(Vector2Normalize(toPlayer), 1.5f));
                    if (CheckCollisionCircles(enemies[i].position, 20 * enemyScale, playerPos, 25 * playerScale)) {
                        currentScreen = GAMEOVER;
                    }
                }
            }

            for (int i = 0; i < MAX_BULLETS; i++) {
                if (!bullets[i].active) continue;
                for (int j = 0; j < MAX_ENEMIES; j++) {
                    if (enemies[j].active && CheckCollisionCircles(bullets[i].position, 10, enemies[j].position, 40 * enemyScale)) {
                        bullets[i].active = false;
                        enemies[j].active = false;
                        score++;
                        break;
                    }
                }
            }

            DrawTexture(background, 0, 0, WHITE);
            Rectangle playerRec = { 0, 0, (float)playerTex.width, (float)playerTex.height };
            Rectangle playerDest = { playerPos.x, playerPos.y, (float)playerTex.width * playerScale, (float)playerTex.height * playerScale };
            Vector2 playerOrigin = { (float)playerTex.width * playerScale / 2, (float)playerTex.height * playerScale / 2 };
            DrawTexturePro(playerTex, playerRec, playerDest, playerOrigin, 0.0f, WHITE);

            for (int i = 0; i < MAX_BULLETS; i++) {
                if (bullets[i].active) DrawCircleV(bullets[i].position, 5, RED);
            }
            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (enemies[i].active) {
                    Rectangle enemyRec = { 0, 0, (float)enemyTex.width, (float)enemyTex.height };
                    Rectangle enemyDest = { enemies[i].position.x, enemies[i].position.y, (float)enemyTex.width * enemyScale, (float)enemyTex.height * enemyScale };
                    Vector2 enemyOrigin = { (float)enemyTex.width * enemyScale / 2, (float)enemyTex.height * enemyScale / 2 };
                    DrawTexturePro(enemyTex, enemyRec, enemyDest, enemyOrigin, 0.0f, WHITE);
                }
            }

            DrawText(TextFormat("Score: %d", score), 10, 10, 20, DARKGRAY);
        }
        else if (currentScreen == GAMEOVER) {
            DrawTexture(background, 0, 0, WHITE);
            DrawText("GAME OVER", 300, 200, 40, RED);
            DrawText(TextFormat("Final Score: %d", score), 300, 260, 20, DARKGRAY);
            DrawText("Press ENTER to return to Menu", 240, 320, 20, DARKGRAY);
            if (IsKeyPressed(KEY_ENTER)) currentScreen = MENU;
        }

        EndDrawing();
    }

    UnloadTexture(background);
    UnloadTexture(playerTex);
    UnloadTexture(enemyTex);
    CloseWindow();
    return 0;
}
