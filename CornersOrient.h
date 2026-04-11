#ifndef CORNERSORIENT_H
#define CORNERSORIENT_H

#include "CornersPerm.h"

class CornersOrient {
	unsigned short orients;
public:
	CornersOrient() : orients(0) {}
	CornersOrient(unsigned corner0orient, unsigned corner1orient,
			unsigned corner2orient, unsigned corner3orient,
			unsigned corner4orient, unsigned corner5orient,
			unsigned corner6orient, unsigned corner7orient);

	void setAt(unsigned idx, unsigned char orient) {
		orients &= ~(3 << 2*idx);
		orients |= orient << 2*idx;
	}
	unsigned getAt(unsigned idx) const { return orients >> 2*idx & 3; }
	unsigned get() const { return orients; }
	void set(unsigned orts) { orients = orts; }
    static CornersOrient compose(CornersOrient cco1,
            CornersPerm ccp2, CornersOrient cco2);
    static CornersOrient compose3(CornersOrient cco1,
            CornersPerm ccp2, CornersOrient cco2,
            CornersPerm ccp3, CornersOrient cco3);
    CornersOrient symmetric() const {
        CornersOrient ccores;
        // set orient 2 -> 1, 1 -> 2, 0 unchanged
        unsigned short orie = (orients & 0xaaaa) >> 1 | (orients & 0x5555) << 1;
        ccores.orients = (orie >> 8 | orie << 8) & 0xffff;
        return ccores;
    }
    CornersOrient reverse(CornersPerm) const;
    CornersOrient transform(CornersPerm, unsigned transformDir) const;
	bool operator==(const CornersOrient &cco) const { return orients == cco.orients; }
	bool operator!=(const CornersOrient &cco) const { return orients != cco.orients; }
	bool operator<(const CornersOrient &cco) const { return orients < cco.orients; }
    unsigned short getOrientIdx() const;
    static CornersOrient fromOrientIdx(unsigned short);
    bool isBGspace() const;
    bool isYWspace(CornersPerm) const;
    bool isORspace(CornersPerm) const;
    CornersOrient representativeBG(CornersPerm) const;
    CornersOrient representativeYW(CornersPerm) const;
    CornersOrient representativeOR(CornersPerm) const;
};

#endif // CORNERSORIENT_H
