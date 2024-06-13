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

int BOUNCE_LIMIT = 5;

class Ball;
class GrayObs;

std::mutex mutex;
std::atomic<bool> running(true);
std::vector<std::thread> ballThreads;  // Vector przechowuj¹cy w¹tki dla ka¿dej pi³ki
std::vector<std::unique_ptr<Ball>> balls;  // Wektor pi³ek
std::chrono::steady_clock::time_point lastBallTime = std::chrono::steady_clock::now();
int refreshMillis = 16;  // Próba uzyskania p³ynniejszej animacji przy oko³o 60 FPS
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<> dis(0.0, 1.0);

float getRandom() {
    return dis(gen);
}

class GrayObs {
private:
    GLfloat obsWidth;
    GLfloat obsHeight;
    GLfloat obsX;
    GLfloat obsY;
    GLfloat obsSpeed;
    GLfloat colorR;
    GLfloat colorG;
    GLfloat colorB;
    int dir;

public:
    GrayObs() : obsWidth(0.4f), obsHeight(0.8f), obsX(-0.55f), obsY(0.75f - obsHeight),
                obsSpeed(getRandom() * 0.02f + 0.01f),
                colorR(0.5f), colorG(0.5f), colorB(0.5f), dir(1) {}

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

    void update() {
        obsY += obsSpeed * dir;

        if (obsY + obsHeight > 1.0f || obsY < -1.0f) {
            dir = -dir;
            obsY += 0.05f * dir;
            obsSpeed = getRandom() * 0.02f + 0.01f;
        }
    }
};

class Ball {
public:
    GLfloat radius;
    GLfloat x, y;
    GLfloat xSpeed, ySpeed;
    GLfloat colorR, colorG, colorB;
    int numBounces;
    bool active;

    Ball() : radius(0.1f), x(0.0f), y(-1.0f + radius),
             xSpeed(getRandom() * 0.04f - 0.02f),
             ySpeed(getRandom() * 0.04f + 0.02f),
             colorR(getRandom()), colorG(getRandom()), colorB(getRandom()),
             numBounces(0), active(true) {}

    void run() {
        while (running && active) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            std::lock_guard<std::mutex> lock(mutex);
            if (numBounces < BOUNCE_LIMIT) {
                x += xSpeed / 2;
                y += ySpeed / 2;

                if (x + radius > 1.0f || x - radius < -1.0f) {
                    xSpeed = -xSpeed;
                    numBounces++;
                }

                if (y + radius > 1.0f || y - radius < -1.0f) {
                    ySpeed = -ySpeed;
                    numBounces++;
                }
            } else {
                active = false;  // Zakoñczenie dzia³alnoœci pi³ki
            }
        }
    }

    void draw() {
        glColor3f(colorR, colorG, colorB);
        glTranslatef(x, y, 0.0f);
        glutSolidSphere(radius, 20, 20);
    }
};

GrayObs grayObs;  // Globalna instancja GrayObs

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

void initGL() {
    glClearColor(0.7f, 0.7f, 1.0f, 1.0f);
}

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


void manageBalls() {
    while (running) {
        auto currentTime = std::chrono::steady_clock::now();
        double elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastBallTime).count();
        if (elapsedTime >= 1000 + rand() % 3000) {
            std::lock_guard<std::mutex> lock(mutex);
            std::unique_ptr<Ball> ball(new Ball());
            ballThreads.emplace_back(&Ball::run, ball.get());
            balls.push_back(std::move(ball));
            lastBallTime = currentTime;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void update(int value) {
    glutPostRedisplay();
    glutTimerFunc(refreshMillis, update, 0);
}

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

