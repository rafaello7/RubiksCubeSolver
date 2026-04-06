#ifndef CUBEREAD_H
#define CUBEREAD_H

#include "cube.h"
#include "responder.h"

bool cubeFromColorsOnSquares(Responder&, const char *squareColors, cube&);
bool cubeFromString(Responder&, const char *cubeStr, cube&);

#endif // CUBEREAD_H
