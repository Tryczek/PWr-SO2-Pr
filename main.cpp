#include <GL/freeglut.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <random>
#include <memory>
#include <algorithm>
#include <cmath>
#include <condition_variable>

int BOUNCE_LIMIT = 5;

class GrayObs;
class Ball;

std::mutex mutex;
std::condition_variable ballCond;
std::atomic<bool> running(true);
std::vector<std::thread> ballThreads;
std::vector<std::unique_ptr<Ball>> balls;
std::chrono::steady_clock::time_point lastBallTime = std::chrono::steady_clock::now();
int refreshMillis = 16;
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<> dis(0.0, 1.0);

float getRandom() {
    return dis(gen);
}

// Klasa reprezentuj¹ca pi³kê
class Ball {
public:
    GLfloat radius;
    GLfloat x, y;
    GLfloat xSpeed, ySpeed;
    GLfloat colorR, colorG, colorB;
    int numBounces;
    bool active;
    bool attached;
    std::chrono::steady_clock::time_point cooldownEnd;

    Ball() : radius(0.1f), x(0.0f), y(-1.0f + radius),
             xSpeed(getRandom() * 0.24f - 0.12f),
             ySpeed(getRandom() * 0.16f - 0.08f),
             colorR(getRandom()), colorG(getRandom()), colorB(getRandom()),
             numBounces(0), active(true), attached(false) {}

    void run();  // Metoda uruchamiaj¹ca w¹tek pi³ki
    void draw();  // Metoda rysuj¹ca pi³kê
};

// Klasa reprezentuj¹ca szary obszar
class GrayObs {
private:
    GLfloat obsWidth;
    GLfloat obsHeight;
    GLfloat obsX;
    GLfloat obsY;
    GLfloat obsSpeed;
    GLfloat colorR, colorG, colorB;
    int dir;

public:
    std::vector<std::pair<Ball*, std::pair<GLfloat, GLfloat>>> attachedBalls;

    GrayObs() : obsWidth(0.4f), obsHeight(0.8f), obsX(-0.55f), obsY(0.75f - obsHeight),
                obsSpeed(getRandom() * 0.02f + 0.01f),
                colorR(0.5f), colorG(0.5f), colorB(0.5f), dir(1) {}
    // Metoda rysuj¹ca GrayObs
    void draw() {
        glColor3f(colorR, colorG, colorB);
        glTranslatef(obsX, obsY, 0.0f);
        glBegin(GL_QUADS);
        glVertex2f(0.0f, 0.0f);
        glVertex2f(obsWidth, 0.0f);
        glVertex2f(obsWidth, obsHeight);
        glVertex2f(0.0f, obsHeight);
        glEnd();
    }

    // Metoda aktualizuj¹ca pozycjê GrayObs i przyklejonych pi³ek
    void update() {
        obsY += obsSpeed * dir;

        // Zmiana kierunku ruchu po osi¹gniêciu górnej lub dolnej krawêdzi
        if (obsY + obsHeight > 1.0f || obsY < -1.0f) {
            dir = -dir;
            obsY += 0.05f * dir;
            obsSpeed = getRandom() * 0.02f + 0.005f;
        }

        // Aktualizacja pozycji przyklejonych pi³ek
        for (auto& attachedBall : attachedBalls) {
            Ball* ball = attachedBall.first;
            ball->x = obsX + attachedBall.second.first;
            ball->y = obsY + attachedBall.second.second;
        }

        // Odpchniêcie pi³ek po przyklejeniu czterech z nich
        if (attachedBalls.size() >= 4) {
            GLfloat centerX = obsX + obsWidth / 2;
            GLfloat centerY = obsY + obsHeight / 2;

            for (auto& attachedBall : attachedBalls) {
                Ball* ball = attachedBall.first;
                GLfloat angle = atan2(ball->y - centerY, ball->x - centerX) + (getRandom() - 0.5f) * 0.2f;
                ball->xSpeed = cos(angle) * 0.05f;
                ball->ySpeed = sin(angle) * 0.05f;
                ball->attached = false;
                ball->cooldownEnd = std::chrono::steady_clock::now() + std::chrono::milliseconds(400);
            }

            attachedBalls.clear();
        }
    }

    // Metoda przyklejaj¹ca pi³kê do GrayObs
    void attachBall(Ball* ball, GLfloat attachX, GLfloat attachY) {
        attachedBalls.emplace_back(ball, std::make_pair(attachX, attachY));
        ball->xSpeed = 0;
        ball->ySpeed = 0;
        ball->attached = true;
    }

    // Metoda sprawdzaj¹ca kolizjê pi³ki z GrayObs
    bool checkCollision(Ball* ball, GLfloat& attachX, GLfloat& attachY) {
        if (ball->x + ball->radius > obsX && ball->x - ball->radius < obsX + obsWidth &&
            ball->y + ball->radius > obsY && ball->y - ball->radius < obsY + obsHeight) {

            attachX = ball->x - obsX;
            attachY = ball->y - obsY;
            return true;
        }
        return false;
    }
};

GrayObs grayObs;  // Globalna instancja GrayObs

// Metoda uruchamiaj¹ca w¹tek pi³ki
void Ball::run() {
    while (running && active) {
        std::unique_lock<std::mutex> lock(mutex);
        if (ballCond.wait_for(lock, std::chrono::milliseconds(16), [this] { return !active || !running; })) {
            if (!running || !active) break; // Wyjœcie, jeœli pi³ka nie jest aktywna lub running jest false
        }
        if (numBounces < BOUNCE_LIMIT && !attached) {
            x += xSpeed / 4;
            y += ySpeed / 4;
            if (x + radius > 1.0f || x - radius < -1.0f) {
                xSpeed = -xSpeed;
                numBounces++;
            }
            if (y + radius > 1.0f || y - radius < -1.0f) {
                ySpeed = -ySpeed;
                numBounces++;
            }
            GLfloat attachX, attachY;
            if (grayObs.checkCollision(this, attachX, attachY) && std::chrono::steady_clock::now() > cooldownEnd) {
                grayObs.attachBall(this, attachX, attachY);
            }
        } else if (!attached) {
            active = false;
        }
    }
}


// Metoda rysuj¹ca pi³kê
void Ball::draw() {
    glColor3f(colorR, colorG, colorB);
    glTranslatef(x, y, 0.0f);
    glutSolidSphere(radius, 20, 20);
}

// Funkcja wyœwietlaj¹ca
void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    std::lock_guard<std::mutex> lock(mutex);
    balls.erase(std::remove_if(balls.begin(), balls.end(),
                               [](const std::unique_ptr<Ball>& b) { return !b->active; }),
                balls.end());

    for (auto& ball : balls) {
        glLoadIdentity();
        ball->draw();
    }

    glLoadIdentity();
    grayObs.update();
    grayObs.draw();

    glutSwapBuffers();
}

// Funkcja inicjalizuj¹ca OpenGL
void initGL() {
    glClearColor(0.7f, 0.7f, 1.0f, 1.0f);
}

// Funkcja obs³uguj¹ca naciœniêcia klawiszy
void keyboard(unsigned char key, int x, int y) {
    if (key == 32) { // Spacja
        running = false;
        ballCond.notify_all();
        for (auto& thread : ballThreads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        exit(0);
    }
}

// Funkcja zarz¹dzaj¹ca pi³kami
void manageBalls() {
    while (running) {
        std::unique_lock<std::mutex> lock(mutex);
        auto nextBallTime = lastBallTime + std::chrono::milliseconds(2000 + rand() % 8000);
        if (ballCond.wait_until(lock, nextBallTime, [] { return !running; })) {
            break; // Wyjœcie, jeœli running jest false
        }
        if (running) {
            std::unique_ptr<Ball> ball(new Ball());
            ballThreads.emplace_back(&Ball::run, ball.get());
            balls.push_back(std::move(ball));
            lastBallTime = std::chrono::steady_clock::now();
        }
    }
}

// Funkcja aktualizuj¹ca ekran
void update(int value) {
    glutPostRedisplay();
    glutTimerFunc(refreshMillis, update, 0);
}

// Funkcja g³ówna
int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(1000, 1000);
    glutCreateWindow("Bouncing Balls");

    initGL();
    glutDisplayFunc(display);
    glutTimerFunc(0, update, 0);
    glutKeyboardFunc(keyboard);

    std::thread managerThread(manageBalls);

    glutMainLoop();

    running = false;
    ballCond.notify_all();
    if (managerThread.joinable()) {
        managerThread.join();
    }
    for (auto& thread : ballThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    return 0;
}

