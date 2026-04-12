#ifndef CUBE_H
#define CUBE_H

#include <string>
#include "cubedefs.h"
#include "CornersPerm.h"
#include "CornersOrient.h"
#include "cubeedges.h"

struct cube {
	CornersPerm ccp;
    CornersOrient cco;
	cubeedges ce;

    static cube compose(const cube &c1, const cube &c2) {
        return {
            .ccp = CornersPerm::compose(c1.ccp, c2.ccp),
            .cco = CornersOrient::compose(c1.cco, c2.ccp, c2.cco),
            .ce = cubeedges::compose(c1.ce, c2.ce)
        };
    }
    cube symmetric() const {
        return {
            .ccp = ccp.symmetric(),
            .cco = cco.symmetric(),
            .ce = ce.symmetric()
        };
    }
    cube reverse() const {
        return {
            .ccp = ccp.reverse(),
            .cco = cco.reverse(ccp),
            .ce = ce.reverse()
        };
    }
    cube transform(unsigned transformDir) const;
    bool operator==(const cube &c) const;
    bool operator!=(const cube &c) const;
    bool operator<(const cube &c) const;
    bool isNil() const { return ce.isNil(); }
    bool isBGspace() const { return cco.isBGspace() && ce.isBGspace(); }
    bool isYWspace() const { return cco.isYWspace(ccp) && ce.isYWspace(); }
    bool isORspace() const { return cco.isORspace(ccp) && ce.isORspace(); }
    cube representativeBG() const;
    cube representativeYW() const;
    cube representativeOR() const;
    std::string toParamText() const;
};

extern const struct cube csolved;
extern const struct cube crotated[RCOUNT];
extern const struct cube ctransformed[TCOUNT];
void cubePrint(const cube&);

#endif // CUBE_H
