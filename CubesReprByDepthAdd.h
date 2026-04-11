#ifndef CUBESREPRBYDEPTHADD_H
#define CUBESREPRBYDEPTHADD_H

#include "CubesReprByDepth.h"
#include "responder.h"

class CubesReprByDepthAdd {
    CubesReprByDepth m_cubesReprByDepth;
public:
    CubesReprByDepthAdd(bool useReverse);

    bool isUseReverse() const { return m_cubesReprByDepth.isUseReverse(); }

    /* Returns the cube set filled up to at least the specified depth (inclusive).
     * The function triggers filling procedure when the depth is requested first time.
     * When the filling procedure is canceled, NULL is returned.
     */
    const CubesReprByDepth *getReprCubes(unsigned depth, Responder&);
};

#endif // CUBESREPRBYDEPTHADD_H
