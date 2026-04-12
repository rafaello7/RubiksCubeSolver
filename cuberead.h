#ifndef CUBEREAD_H
#define CUBEREAD_H

#include "cube.h"
#include "Responder.h"

bool cubeFromColorsOnSquares(Responder&, const char *squareColors, cube&);
bool cubeFromString(Responder&, const char *cubeStr, cube&);

#endif // CUBEREAD_H
