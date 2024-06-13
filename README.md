
# Bouncing Balls

## Opis

Ten projekt wizualizuje animację odbijających się piłek przy użyciu OpenGL i FreeGLUT. Piłki pojawiają się losowo, odbijają się od krawędzi ekranu oraz od szarego obszaru (`GrayObs`), do którego mogą się przyklejać i od którego mogą być odpychane.

## Wymagania

- Kompilator C++ (g++)
- Biblioteki FreeGLUT i OpenGL

## Instalacja zależności

Upewnij się, że masz zainstalowane wymagane biblioteki. Możesz je zainstalować za pomocą menedżera pakietów swojego systemu. Dla systemów opartych na Debianie/Ubuntu, użyj:

```bash
sudo apt-get update
sudo apt-get install freeglut3 freeglut3-dev
sudo apt-get install g++
```

## Kompilacja

Skompiluj kod źródłowy przy użyciu kompilatora `g++`. W terminalu wpisz:

```bash
g++ main.cpp -o bouncing_balls -lglut -lGLU -lGL
```

Tutaj:
- `main.cpp` jest nazwą pliku źródłowego.
- `-o bouncing_balls` oznacza, że wyjściowy plik wykonywalny będzie nosił nazwę `bouncing_balls`.
- `-lglut -lGLU -lGL` dodaje odpowiednie biblioteki.

## Uruchomienie

Po skompilowaniu, uruchom program:

```bash
./bouncing_balls
```

## Sterowanie

- Naciśnij klawisz spacji, aby zakończyć program.

