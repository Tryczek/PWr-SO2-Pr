
# Bouncing Balls

## Description

This project visualizes an animation of bouncing balls using OpenGL and FreeGLUT. The balls appear randomly, bounce off the edges of the screen, and interact with a gray area (`GrayObs`), to which they can stick and from which they can be repelled.

## Requirements

- C++ compiler (g++)
- FreeGLUT and OpenGL libraries

## Dependency Installation

Ensure that you have the required libraries installed. You can install them using your system's package manager. For Debian/Ubuntu-based systems, use:

```bash
sudo apt-get update
sudo apt-get install freeglut3 freeglut3-dev
sudo apt-get install g++
```

## Compilation

Compile the source code using the `g++` compiler. In the terminal, type:

```bash
g++ main.cpp -o bouncing_balls -lglut -lGLU -lGL
```

Here:
- `main.cpp` is the name of the source file.
- `-o bouncing_balls` specifies that the output executable will be named `bouncing_balls`.
- `-lglut -lGLU -lGL` links the appropriate libraries.

## Running

After compiling, run the program:

```bash
./bouncing_balls
```

## Controls

- Press the spacebar to exit the program.
