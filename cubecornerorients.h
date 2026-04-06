#ifndef CUBECORNERORIENTS_H
#define CUBECORNERORIENTS_H

#include "cubecornersperm.h"

class cubecorner_orients {
	unsigned short orients;
public:
	cubecorner_orients() : orients(0) {}
	cubecorner_orients(unsigned corner0orient, unsigned corner1orient,
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
    static cubecorner_orients compose(cubecorner_orients cco1,
            cubecorners_perm ccp2, cubecorner_orients cco2);
    static cubecorner_orients compose3(cubecorner_orients cco1,
            cubecorners_perm ccp2, cubecorner_orients cco2,
            cubecorners_perm ccp3, cubecorner_orients cco3);
    cubecorner_orients symmetric() const {
        cubecorner_orients ccores;
        // set orient 2 -> 1, 1 -> 2, 0 unchanged
        unsigned short orie = (orients & 0xaaaa) >> 1 | (orients & 0x5555) << 1;
        ccores.orients = (orie >> 8 | orie << 8) & 0xffff;
        return ccores;
    }
    cubecorner_orients reverse(cubecorners_perm) const;
    cubecorner_orients transform(cubecorners_perm, unsigned transformDir) const;
	bool operator==(const cubecorner_orients &cco) const { return orients == cco.orients; }
	bool operator!=(const cubecorner_orients &cco) const { return orients != cco.orients; }
	bool operator<(const cubecorner_orients &cco) const { return orients < cco.orients; }
    unsigned short getOrientIdx() const;
    static cubecorner_orients fromOrientIdx(unsigned short);
    bool isBGspace() const;
    bool isYWspace(cubecorners_perm) const;
    bool isORspace(cubecorners_perm) const;
    cubecorner_orients representativeBG(cubecorners_perm) const;
    cubecorner_orients representativeYW(cubecorners_perm) const;
    cubecorner_orients representativeOR(cubecorners_perm) const;
};

#endif // CUBECORNERORIENTS_H
