# Rubik's Cube Solver

The program solves Rubik's cube. The found solution is always optimal,
i.e. has minimal possible number of steps.

## Requirements

The program can be compiled and run under Linux. It is compiled using `g++`.

To run, a computer with at least 6GB RAM is recommended.
The program can run with less memory, but it will perform poorly.
Please note that the program performs a lot of computations. The more
processors, the less time the calculations will take.

## Compilation

To compile, go to the directory with sources and run `make`.

## Running

To run, go to the directory with sources and invoke `./cubesrv`
(without parameters). Selecting the source directory as the current
for program is important, because the program reads additional files
(_cube.html_, _cube.css_ etc.) from the current directory.

The program works as an http server, listening on port 8080. So,
after starting it, open [http://localhost:8080/](http://localhost:8080/)
in a browser.

The page can operate the cube in two modes: _Manipulate_ and _Edit_.
Initially the _Manipulate_ mode is active, which allows to rotate walls.
In _Edit_ mode, it is possible to select colors of squares.

If you want to solve a physical cube, start _Edit_ mode (select the
`Edit` radio button). Each square shows 6 colors to choose from.
Click the appropriate colors on all walls and then click the `Apply` button.

Clicking on an already selected color deselects that color as selected
and returns to the list of colors in that square to choose from.

Until the colors on all faces are selected, the `Apply` button remains
greyed out. Clicking `Apply` confirms the appearance of the cube and
returns to `Manipulate` mode.

In `Manipulate` mode, clicking the `Solve` button starts searching for
moves to solve the cube. Depending on the layout of the cube and the
power of the computer, searching for a solution may take from
a few seconds to hours. The list of moves to solve the cube appears on
the right side of the cube. Clicking on an item on the list moves the
cube to an intermediate state, the one after the selected (clicked) move.

Note that while the program is looking for a solution, does not
dispatch any HTTP requests.

## Algorithm

The program uses a dumb algorithm, without any heuristics, group theory
etc.  The algorithm is to form a set of mixed cubes which can be solved in a
few moves, then attempt to find a cube in the set among all which can be
reached from the cube to solve with a few moves. The program does it repeatedly
with more and more moves (on both ends), until found.

