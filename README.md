# Rubik's Cube Solver - kotlin version

The program solves Rubik's cube.

The program has two modes of operation: optimal and quick. In optimal mode
it finds solutions having minimal possible number of steps. In quick mode
it finds solutions quickly, but they can be far from optimal. In quick
mode the solution can be (theoretically) up to 30 steps long. In
practice the length rarely exceeds 25 steps.

## Requirements

The program is compiled using `mvn`.

To search for optimal solutions, a computer with at least 6GB RAM
is recommended.  With 3-4 GB of RAM the program executes slightly slower.
With 2GB RAM or less it will perform rather poorly
(512MB is absolute minimum).

Please note that the program performs a lot of computations. The more
processors, the less time the calculations will take.

In _Quick_ mode the program uses only about 40MB RAM. There is also
a _Quick Multi_ mode, which uses about 300MB RAM. Both _Quick_ and
_Quick Multi_ modes use the same algorithm, but for _Quick Multi_ mode
a bigger search table is initialized to speed up the search.


## Compilation

To compile, run `mvn install`.

## Running

To run, invoke `./cubesrv`
(without parameters).

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

## Optimal Search

If the Optimal mode is selected, searching for a solution may take from a few
seconds to hours.  On a modern PC (PassMark ~4000) the solution search of a
randomly mixed cube (typically requiring 17-18 moves) takes about 1-2 minutes.
Solving the first cube (since program startup) takes slightly longer due to data
structures initialization. Note that the program does not store any persistent
data structures on disk.

## Quick Mode Search

There are two sub-options of the Quick mode. When the _Quick_ option is
selected, the first solution found is returned. When _Quick Multi_
option is selected, the program searches for more and more solutions,
until the _Stop_ button is pressed. The program returns the shortest
solution found.

## Algorithm

For _Optimal_ mode the program uses a dumb algorithm, without any heuristics,
group theory, etc. The algorithm is to build a set of mixed cubes which can
be solved in a few moves, then attempt to find a cube in the set among all
which can be reached from the cube to solve with a few moves. The program does
it repeatedly with more and more moves (on both ends), until found.  The cube
sets built by program contain only distinct cubes in respect of a cube
rotation, symmetry and optionally the cube moves reversing.

In _Quick_ mode the program searches for moves in two phases. The algorithm
resembles the Two-Phase algorihm developed by Herbert Kociemba, but it does
not use any prunning tables.

## Command line

The _cubesrv_ program accepts either one or three parameters. The first parameter
is the maximum depth of search sets used for optimal search. The number
can be suffixed with `r` letter, which means that moves reversing will
be included in check for distinct cubes. Including the moves reversing
decreases the search set sizes rougly by half. The rough amount of memory
used by program for particular depths is shown below:

    8r      270MB
    8       500MB
    9r      2.3GB
    9       4.5GB
    10r     25GB

If the depth is not provided, the program selects it using the computer
RAM size as a base. Among the following three options is made the choice:
8r, 9r, 9.

The maximum search depth is 3 times larger than maximum depth of the sets.
For example, if the maximum depth is set to 4, the solution is searched
maximum 12 moves deep.

It is also possible to specify cubes to solve in the command line. The
possibility is generally used for testing. The cube(s) to solve are
passed as the second parameter. The third parameter is the solve mode:

 * `o` - _optimal_,
 * `O` - also _optimal_, but before search the cube sets are built
 * `q` - _quick_
 * `m` - _quick multi_

The cube to solve (second parameter) can be passed as follows:

 * 54 letters with colors on walls, like:

        YYYYYYYYYOOOOOOOOOBBBBBBBBBRRRRRRRRRGGGGGGGGGWWWWWWWWW

   This format is used internally by the program, when the cube to solve
   is passed by web browser in query.
 * Also 54 letters of the cube in format used by Herbert Kociemba solver:

        UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB

 * The wall rotations (scramble), like: `B3L3D3F1D3B2L`...
 * Name of a file with list of cubes to solve. One cube per line, in
   formats specified above. The file name must have `.txt` extension. 
 * A number. In this case random cubes will be generated. The number is
   the number of cubes to generate.

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

