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

int BOUNCE_LIMIT = 5;  // Limit odbi� po kt�rym pi�ka zniknie

class GrayObs;
class Ball;

std::mutex mutex;
std::atomic<bool> running(true);
std::vector<std::thread> ballThreads;  // Wektor przechowuj�cy w�tki dla ka�dej pi�ki
std::vector<std::unique_ptr<Ball>> balls;  // Wektor pi�ek
std::chrono::steady_clock::time_point lastBallTime = std::chrono::steady_clock::now();
int refreshMillis = 16;  // Pr�ba uzyskania p�ynniejszej animacji przy oko�o 60 FPS
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<> dis(0.0, 1.0);

float getRandom() {
    return dis(gen);
}

// Klasa reprezentuj�ca pi�k�
class Ball {
public:
    GLfloat radius;  // Promie� pi�ki
    GLfloat x, y;  // Pozycja pi�ki
    GLfloat xSpeed, ySpeed;  // Pr�dko�� pi�ki w osiach X i Y
    GLfloat colorR, colorG, colorB;  // Kolor pi�ki
    int numBounces;  // Liczba odbi� pi�ki
    bool active;  // Flaga okre�laj�ca, czy pi�ka jest aktywna
    bool attached;  // Flaga okre�laj�ca, czy pi�ka jest przyklejona do GrayObs
    std::chrono::steady_clock::time_point cooldownEnd;  // Czas ko�ca cooldownu

    // Konstruktor Ball inicjalizuje losow� pr�dko�� i kolor pi�ki
    Ball() : radius(0.1f), x(0.0f), y(-1.0f + radius),
             xSpeed(getRandom() * 0.12f - 0.01f),
             ySpeed(getRandom() * 0.08f + 0.01f),
             colorR(getRandom()), colorG(getRandom()), colorB(getRandom()),
             numBounces(0), active(true), attached(false) {}

    void run();  // Metoda uruchamiaj�ca w�tek pi�ki
    void draw();  // Metoda rysuj�ca pi�k�
};

// Klasa reprezentuj�ca szary obszar
class GrayObs {
private:
    GLfloat obsWidth;  // Szeroko�� obszaru
    GLfloat obsHeight;  // Wysoko�� obszaru
    GLfloat obsX;  // Pozycja X obszaru
    GLfloat obsY;  // Pozycja Y obszaru
    GLfloat obsSpeed;  // Pr�dko�� obszaru
    GLfloat colorR, colorG, colorB;  // Kolor obszaru
    int dir;  // Kierunek ruchu obszaru

public:
    // Wektor przechowuj�cy pary pi�ek i ich wzgl�dne pozycje przyklejenia
    std::vector<std::pair<Ball*, std::pair<GLfloat, GLfloat>>> attachedBalls;

    // Konstruktor inicjalizuj�cy GrayObs
    GrayObs() : obsWidth(0.4f), obsHeight(0.8f), obsX(-0.55f), obsY(0.75f - obsHeight),
                obsSpeed(getRandom() * 0.02f + 0.01f),
                colorR(0.5f), colorG(0.5f), colorB(0.5f), dir(1) {}

    // Metoda rysuj�ca GrayObs
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

    // Metoda aktualizuj�ca pozycj� GrayObs i przyklejonych pi�ek
    void update() {
        obsY += obsSpeed * dir;

        // Zmiana kierunku ruchu po osi�gni�ciu g�rnej lub dolnej kraw�dzi
        if (obsY + obsHeight > 1.0f || obsY < -1.0f) {
            dir = -dir;
            obsY += 0.05f * dir;
            obsSpeed = getRandom() * 0.02f + 0.005f;
        }

        // Aktualizacja pozycji przyklejonych pi�ek
        for (auto& attachedBall : attachedBalls) {
            Ball* ball = attachedBall.first;
            ball->x = obsX + attachedBall.second.first;
            ball->y = obsY + attachedBall.second.second;
        }

        // Odpchni�cie pi�ek po przyklejeniu czterech z nich
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

    // Metoda przyklejaj�ca pi�k� do GrayObs
    void attachBall(Ball* ball, GLfloat attachX, GLfloat attachY) {
        attachedBalls.emplace_back(ball, std::make_pair(attachX, attachY));
        ball->xSpeed = 0;
        ball->ySpeed = 0;
        ball->attached = true;
    }

    // Metoda sprawdzaj�ca kolizj� pi�ki z GrayObs
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

// Metoda uruchamiaj�ca w�tek pi�ki
void Ball::run() {
    while (running && active) {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));  // Przerwa na 16 ms, aby uzyska� oko�o 60 FPS
        std::lock_guard<std::mutex> lock(mutex);
        if (numBounces < BOUNCE_LIMIT && !attached) {
            x += xSpeed / 4;
            y += ySpeed / 4;

            // Odbicie od kraw�dzi
            if (x + radius > 1.0f || x - radius < -1.0f) {
                xSpeed = -xSpeed;
                numBounces++;
            }

            if (y + radius > 1.0f || y - radius < -1.0f) {
                ySpeed = -ySpeed;
                numBounces++;
            }

            GLfloat attachX, attachY;
            // Sprawdzenie kolizji z GrayObs
            if (grayObs.checkCollision(this, attachX, attachY) && std::chrono::steady_clock::now() > cooldownEnd) {
                grayObs.attachBall(this, attachX, attachY);
            }
        } else if (!attached) {
            active = false;  // Zako�czenie dzia�alno�ci pi�ki
        }
    }
}

// Metoda rysuj�ca pi�k�
void Ball::draw() {
    glColor3f(colorR, colorG, colorB);
    glTranslatef(x, y, 0.0f);
    glutSolidSphere(radius, 20, 20);
}

// Funkcja wy�wietlaj�ca
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

// Funkcja inicjalizuj�ca OpenGL
void initGL() {
    glClearColor(0.7f, 0.7f, 1.0f, 1.0f);
}

// Funkcja obs�uguj�ca naci�ni�cia klawiszy
void keyboard(unsigned char key, int x, int y) {
    if (key == 32) { // Spacja
        running = false;
        for (auto& thread : ballThreads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        exit(0);
    }
}

// Funkcja zarz�dzaj�ca pi�kami
void manageBalls() {
    while (running) {
        auto currentTime = std::chrono::steady_clock::now();
        double elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastBallTime).count();
        if (elapsedTime >= 2000 + rand() % 8000) {
            std::lock_guard<std::mutex> lock(mutex);
            std::unique_ptr<Ball> ball(new Ball());
            ballThreads.emplace_back(&Ball::run, ball.get());
            balls.push_back(std::move(ball));
            lastBallTime = currentTime;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// Funkcja aktualizuj�ca ekran
void update(int value) {
    glutPostRedisplay();
    glutTimerFunc(refreshMillis, update, 0);
}

// Funkcja g��wna
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

