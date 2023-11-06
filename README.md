# Rubik's Cube Solver

The program solves Rubik's cube.

The program has two modes of operation: optimal and quick. In optimal mode
it finds solutions having minimal possible number of steps. In quick mode
it finds solutions quickly, but they can be far from optimal. In quick
mode the solution can be (theoretically) up to 30 steps long. In
practice the length rarely exceeds 25 steps.

## Requirements

The program can be compiled and run under Linux. It is compiled using `g++`.

To search for optimal solutions, a computer with at least 6GB RAM
is recommended.  With 3-4 GB of RAM the program executes slightly slower.
With 2GB RAM or less it will perform rather poorly
(512MB is absolute minimum).

Please note that the program performs a lot of computations. The more
processors, the less time the calculations will take.

In quick mode the program uses only about 16MB RAM.

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

## Quick Mode Search

There are two sub-options of the Quick mode. When the _Quick_ option is
selected, the first solution found is returned. When _Quick Multi_
option is selected, the program searches for more and more solutions,
until the _Stop_ button is pressed. The program returns the shortest
solution found.

## Algorithm

For _Optimal_ mode the program uses a dumb algorithm, without any heuristics,
group theory, prunning tables etc. The algorithm is to build a set of mixed
cubes which can be solved in a few moves, then attempt to find a cube in the
set among all which can be reached from the cube to solve with a few moves. The
program does it repeatedly with more and more moves (on both ends), until
found.

In _Quick_ mode the program searches for moves in two phases. The algorithm
resembles the Two-Phase algorihm developed by Herbert Kociemba, but it does
not use any prunning tables.

## More About Two Phase algorithm

The Two-Phase algorithm uses a subset of moves denoted by
G1 = &lt;U,D,R2,L2,F2,B2&gt;. That is, the whole space of all
43252003274489856000 possible shuffle states of the cube is split into a
cartesian product of 2217093120 x 19508428800 states. It means, there are
2217093120 spaces (sets of cube shuffles), each space having 19508428800
shuffle states. Within each space, each cube state can be reached from another
one using only moves from G1. Also, it is not possible to reach any state from
another space using only moves from the G1 set.

Note that instead of G1 it is possible to use other two move subsets.
When the solved cube is altered using G1 subset, the blue and green walls
remain blue-green. For the other two subsets either the yellow and white walls
remain yellow-white, or the orange and red walls remain orange-red.

The first phase of the two-phase algorithm finds moves transitioning
the cube from one of the 2217093120 spaces to the space which includes
the solved cube. Second phase finds moves solving the intermediate cube
using only moves from the subset.

Any cube space can be reached from another using up to 12 moves. Each
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

About 6% spaces can be reached using 8 moves or less. About 43% spaces
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
Let's use the symbol &#8857; for moves composing (concatenation). Let's use
C<sup>-1</sup> for reverse moves of C.  So, we have:
</p>

<p>
    C<sub>slv1</sub> &#8857; C<sub>space</sub> = C<sub>srch1</sub>
</p>

Let's rearrange this equation:

<p>
    C<sup>-1</sup><sub>slv1</sub> &#8857; C<sub>slv1</sub> &#8857; C<sub>space</sub> = C<sup>-1</sup><sub>slv1</sub> &#8857; C<sub>srch1</sub><br>
    C<sub>space</sub> = C<sup>-1</sup><sub>slv1</sub> &#8857; C<sub>srch1</sub>
</p>

<p>
The C<sub>space</sub> cube is the intermediate cube to solve in second phase.
In this phase the algorithm builds a set of cube states which can
be reached from the solved cube using a few moves from the G1 subset, then
attempts to find a cube which:
</p>

1. is included in the set
2. can be reached from the intermediate cube using a few moves from the G1
   subset

For both phases the program does it repeatedly with more and more moves
(on both ends), until found.

Finally, the steps from both phases are concatenated.

### Calculating The Numbers

To better understand the algorithm, it is imortant to know origin of the
mentioned values.

The cube consists of 8 corners and 12 edges. Each corner has 3 walls, so it can
be at any given position in one of 3 orientations. Each edge has 2 walls, so it
can be in one of 2 orientations. The 8 corners can be in one of 8! (factorial
of 8) combinations; edges - 12!. Taking all this into account, we get the
number of combinations:

<p>
    8! * 3<sup>8</sup> * 12! * 2<sup>12</sup> = 519024039293878272000
</p>

(about 5.1e20). But the right number is 43252003274489856000, i.e. 12 times less.
Why the difference? There are 3 reasons.

The first reason is that not all combinations of corner orientations are valid.
The orientation of 7 corners determine 8th corner orientation. It is
impossible to have two cubes having all the 8 corners at the same locations,
with only one corner rotated by 120&deg; or 240&deg;. If one corner is
rotated by 120&deg; clockwise, then another one must be rotated 120&deg;
counterclockwise. Or, other two corners must be also rotated by 120&deg;.
Or whatever. The sum of rotations modulo 3 must be always 0, counting
1 for 120&deg; rotation and counting 2 for 240&deg; rotation. Hence the
number of all corner orientations is divided by 3.

Similar situation applies to the edges. For edges there are only
two possible rotations, by 0&deg; and 180&deg;. The sum is calculated modulo 2.
The number of all edge orientations is divided by 2.

The third reason is permutation parity. What is permutation parity?
Each permutation can be split into some number of element swaps. For example,
assume we have the following permutation of numbers in range 1..8:

    2 7 3 8 1 5 4 6

To sort the numbers, we can use the following sequence of the number swaps:

    swap        outcome
    ----        -------
    2 <-> 1     1 7 3 8 2 5 4 6
    2 <-> 7     1 2 3 8 7 5 4 6
    4 <-> 8     1 2 3 4 7 5 8 6
    5 <-> 7     1 2 3 4 5 7 8 6
    6 <-> 7     1 2 3 4 5 6 8 7
    7 <-> 8     1 2 3 4 5 6 7 8

The above sequence has 6 swaps. We can select different swap sequences,
but for the above permutation the number of swaps always will be even.
Similar, if for some permutation some sequence has odd number of element
swaps, then all swap sequences for the permutation have odd length.


Coming back to the cube: the corner permutations and edge permutations
can be both even and odd, but, if the corner permutation is even, then
also the edge permutation must be even. If the corner permutation is
odd, then also the edge permutation must be odd. Hence the above number
of all combinations is divided by 2.

Summarizing, the number of all combinations is divided by

    3 * 2 * 2 = 12

Another number that appeared above is the space size, 19508428800. This
is the value calculated as:

    8! * 8! * 4! / 2 = 19508428800

Let's take the blue-green subset, which includes the solved cube.

The first number, 8!, is the number of corner permutations. All the
corner permutations in the subset are possible. On the other hand
the corner orientations are determined: the blue and green squares
always remain on either green or blue wall (i.e. on the walls having
blue and green squares in the middle). So, the number of possible
corner orientations is 1.

The second number, also 8!, is the permutation count of the
edges having blue or green square. There are 8 such edges and any
permutation of the 8 edges is possible. But the edge orientation is
determined: the edges, like corners, have
also blue and green squares always on blue and green walls.

The third number, 4!, is the number of permutations of the remaining
4 edges. The edge orientations also cannot be changed: on
yellow and white walls are always yellow or white squares of the edges.
On orange and red walls are always orange or red squares. It is
impossible to have an orange or red square of the edge on yellow or
white wall.

The division by 2 in the above equation comes from permutation parity. To have
the cube solvable, the overall cube permutation parity must be achieved.

The number of spaces, 2217093120, is the division of the all
cube combinations, 43252003274489856000 by the space
size, 19508428800. But it can be also calculated in different way:

<p>
    3<sup>7</sup> * 2<sup>11</sup> * 495 = 2217093120
</p>

<p>
The first number, 3<sup>7</sup> (=2187) is the number of possible
corner orientations. As the combination of corner orientations is determined in
space, each change in the combination of corner orientations causes transition
to a different space. As the valid cube has only 3<sup>7</sup> possible
orientations (orientation of 8th corner is determined by the remaining 7
corners), hence the number.
</p>

<p>
The second number, 2<sup>11</sup> (=2048) is the number of possible
edge orientations. Like corner orientations, the edge orientations are also
determined in space.
</p>

The third number, 495, is the number of possible choices of 4 elements from a set
of 12. This is the number of choices of the 4 edges which are in the 4 locations
beyond blue and green walls. In the blue-green space which includes the solved
cube, they are the edges having only white, yellow, orange or red squares. Other
spaces have different edges chosen.

The general formula for choice of k elements out of n is:

            n!
        -----------
        k! * (n-k)!

For n = 12 and k = 4 we have:

         479001600
        ------------  =  495
         24 * 40320



