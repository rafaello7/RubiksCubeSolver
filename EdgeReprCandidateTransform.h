#ifndef EDGEREPRCANDIDATETRANSFORM_H
#define EDGEREPRCANDIDATETRANSFORM_H

#include "cubeedges.h"

struct EdgeReprCandidateTransform {
    unsigned transformedIdx;
    bool reversed;
    bool symmetric;
    cubeedges ceTrans;
};

#endif // EDGEREPRCANDIDATETRANSFORM_H
