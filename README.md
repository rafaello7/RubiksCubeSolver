# Rubik's Cube Solver

The program solves Rubik's cube.

The program has two modes of operation: optimal and quick. In optimal mode
it finds solutions having minimal possible number of steps. In quick mode
it finds solutions quickly, but they can be far from optimal. In quick
mode the solution can be up to 30 steps long.

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
The list of moves to solve the cube appears on the right side of the cube.
Clicking on an item on the list moves the cube to an intermediate state, the
one after the selected (clicked) move.

## Optimal Searching

If the Optimal mode is selected, searching for a solution may take from a few
seconds to hours.  On a modern PC (PassMark ~4000) the solution search of a
randomly mixed cube (typically requiring 17-18 moves) takes about 1-2 minutes.
Solving the first cube (since program startup) takes longer due to data
structures initialization. Note that the program does not build any persistent
data structures on disk. All the data structures are kept in memory.

## Algorithm

For _Optimal_ mode the program uses a dumb algorithm, without any heuristics,
group theory, prunning tables etc. The algorithm is to build a set of mixed
cubes which can be solved in a few moves, then attempt to find a cube in the
set among all which can be reached from the cube to solve with a few moves. The
program does it repeatedly with more and more moves (on both ends), until
found.

In _Quick_ mode the program searches for moves in two phases. The algorithm
resembles the Two-Phase algorihm developed by Herbert Kociemba, but it does
not use any prunning tables to find shortest solution. Instead, it catches
the first solution found.

## More About Two Phase algorithm

The Two-Phase algorithm uses a subset of moves denoted by
G1 = &lt;U,D,R2,L2,F2,B2&gt;. That is, the whole space of all
43252003274489856000 possible shuffle states of the cube is split into a
cartesian product of 2217093120 x 19508428800 states. It means, there are
2217093120 spaces (sets of cubes), each space having 19508428800 shuffle
states.  Within each space, each cube state can be reached from another
using only moves from G1. Also, it is not possible to reach any state from
another space using only moves from the G1 set.

Note that there are three distinct move subsets, so the cartesian products
can be formed in three ways. Using move subsets starting from the solved
cube, for the first subset the blue and green walls remain blue-green;
for the second the yellow and white walls remain yellow-white; for the
third subset the orange and red walls remain orange-red.

The first phase of the two-phase algorithm finds moves which transition
the cube from one of the 2217093120 spaces to the space which includes
the solved cube. Second phase finds moves solving the intermediate cube
using only moves from the subset.

Each cube space can be reached from another using 12 moves at maximum. Each
cube within the space can be reached from another cube state in the same space
using 18 moves at maximum. Hence the 30 moves max in quick mode.

Below are the space counts reachable by particular move numbers:

    moves   spaces
    0       1
    1       4
    2       50
    3       592
    4       7156
    5       87236
    6       1043817
    7       12070278
    8       124946368
    9       821605960
    10      1199128738
    11      58202444
    12      476

About 6% spaces can be reached with 8 moves or less. About 43% spaces
using 9 moves or less, and about 97% of spaces using 10 moves or less.

As mentioned, within the space each cube can be reached using 18 moves
or less. Below are the counts of distinct cubes reachable by particular
move counts. Distinct means that one cube cannot be transformed into another
using cube rotations, symmetry, or reversing move sequence. The cubes are
counted for union of three cube spaces, for three move subsets. Lets
name the subsets: blue-green, yellow-white and orange-red. If some cube
is reached from the solved cube using moves from blue-green subset,
the cube remains in blue-green space. The cube transformation can change
the space to yellow-white or orange-red, but always will remain in one
of the three kinds of spaces common with the solved cube. Hence the union
of the three spaces. If a cube is in at least one of the three spaces, it is
counted.

    moves   cubes
    0       1
    1       2
    2       6
    3       20
    4       109
    5       671
    6       3972
    7       23809
    8       135388
    9       739198
    10      3827216
    11      18080333
    12      70352435
    13      177573980
    14      222191002
    15      111124247
    16      6715395
    17      47795
    18      75


The Two-Phase algorithm uses the same search technique as for the
_Optimal_ mode. First, it builds a set of cube spaces which can be reached
from the space with solved cube using a few moves. Next, it attempts to find a
space which:

1. is included in the set
2. can be reached from the cube to solve with a few moves

So, the first phase gives two sets of moves: one from the solved cube,
another from the cube to solve. Lets use the following names for particular
cubes:

<div>
    <ul>
     <li>C<sub>solved</sub> - the solved cube</li>
     <li>C<sub>search</sub> - the cube to solve</li>
     <li>C<sub>slv1</sub> - some cube from space X reached from C<sub>solve</sub></li>
     <li>C<sub>srch1</sub> - some cube from space X reached from C<sub>search</sub></li>
     <li>C<sub>space</sub> - a a set of moves transitioning from
        C<sub>slv1</sub> to C<sub>srch1</sub> within the space X</li>
    </ul>
</div>

<p>
Let's use the symbol &#8857; for moves composing. C<sup>-1</sup> for reverse moves of C.
So, we have:
</p>

<p>
    C<sub>slv1</sub> &#8857; C<sub>space</sub> = C<sub>srch1</sub>
</p>

Let's rearrange this equation:

<p>
    C<sub>space</sub> = C<sup>-1</sup><sub>slv1</sub> &#8857; C<sub>srch1</sub>
</p>

<p>
The C<sub>space</sub> cube is the intermediate cube to solve in second phase.
In this phase the algorithm builds a set of cube states which can
be reached from the solved cube using a few moves, then attempts to find a
cube which:
</p>

1. is included in the set
2. can be reached from the intermediate cube with a few moves

For both phases the program does it repeatedly with more and more moves
(on both ends), until found.

Finally, the steps from both phases are concatenated.

