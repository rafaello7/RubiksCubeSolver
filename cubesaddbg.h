#ifndef CUBESADDBG_H
#define CUBESADDBG_H

#include "cubesreprbg.h"
#include "responder.h"
#include <mutex>

enum {
    TWOPHASE_DEPTH2_MAX = 8u
};

class BGCubesReprByDepthAdd {
    std::mutex m_mtx;
    BGCubesReprByDepth m_cubesReprByDepth;
public:
    BGCubesReprByDepthAdd(bool useReverse);

    bool isUseReverse() const { return m_cubesReprByDepth.isUseReverse(); }

    /* Returns the cube set filled up to at least the specified depth (inclusive).
     * The function triggers filling procedure when the depth is requested first time.
     * When the filling procedure is canceled, NULL is returned.
     */
    const BGCubesReprByDepth *getReprCubes(unsigned depth, Responder&);
};

#endif // CUBESADDBG_H
