# Rubik's Cube Solver

The program solves Rubik's cube. The found solution is always optimal,
i.e. has minimal possible number of steps.

## Requirements

The program can be compiled and run under Linux. It is compiled using `g++`.

To run, a computer with at least 6GB RAM is recommended.
With 3-4 GB of RAM the program executes slightly slower.
With 2GB RAM or less it will perform rather poorly
(512MB is absolute minimum).

Please note that the program performs a lot of computations. The more
processors, the less time the calculations will take.

## Compilation

To compile, go to the directory with sources and run `make`.

## Running

To run, go to the directory with sources and invoke `./cubesrv`
(without parameters). Selecting the source directory as the current
for program is important, because the program reads additional files
(_cube.html_, _cube.css_ etc.) from the current directory.

The program uses a web browser interface. After startup it waits
for http requests on port 8080. So, after starting it, open
[http://localhost:8080/](http://localhost:8080/) in a browser.

The cube can be modified in two ways. The first way is to rotate walls using
buttons in the _Rotate Wall_ section. The second way is to set/adjust colors of
particular squares.

If you want to solve a physical cube, click the _New_ button.  Each square will
show 6 colors to choose from. Clicking on a color on the square selects the
color for square. Clicking on an already selected color deselects that color as
selected and returns to the list of colors to choose from.
Clicking with Ctrl key deselects colors on the whole clicked edge or corner.

Until the colors on all faces are selected, the `Solve` button remain greyed
out.

This method works also when the _New_ button is not pressed. Initially the
colors on all squares are selected. To change the colors, first deselect the
colors on at least two edges or two corners.

Clicking the _New_ button twice resets the cube to solved state.

Clicking the `Solve` button starts searching for moves to solve.
Depending on the layout of the cube and the power of the computer, searching
for a solution may take from a few seconds to hours. The list of moves to solve
the cube appears on the right side of the cube. Clicking on an item on the list
moves the cube to an intermediate state, the one after the selected (clicked)
move.

On a modern PC (PassMark ~4000) the solution search of a randomly mixed cube
(typically requiring 17-18 moves) takes about 1-2 minutes.  Solving the first
cube (since program startup) takes longer due to data structures
initialization. Note that the program does not build any persistent data
structures on disk. All the data structures are kept in memory.

## Algorithm

The program uses a dumb algorithm, without any heuristics, group theory,
prunning tables etc. The algorithm is to build a set of mixed cubes which can
be solved in a few moves, then attempt to find a cube in the set among all
which can be reached from the cube to solve with a few moves. The program does
it repeatedly with more and more moves (on both ends), until found.

