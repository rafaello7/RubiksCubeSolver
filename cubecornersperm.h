#ifndef CUBECORNERSPERM_H
#define CUBECORNERSPERM_H

class cubecorners_perm {
	unsigned perm;
public:
	cubecorners_perm() : perm(0) {}
	cubecorners_perm(unsigned corner0perm, unsigned corner1perm, unsigned corner2perm,
			unsigned corner3perm, unsigned corner4perm, unsigned corner5perm,
			unsigned corner6perm, unsigned corner7perm);

	void setAt(unsigned idx, unsigned char p) {
		perm &= ~(7 << 3*idx);
		perm |= p << 3*idx;
	}
	unsigned getAt(unsigned idx) const { return perm >> 3*idx & 7; }
	unsigned get() const { return perm; }
	void set(unsigned p) { perm = p; }
    static cubecorners_perm compose(cubecorners_perm, cubecorners_perm);
    static cubecorners_perm compose3(cubecorners_perm, cubecorners_perm, cubecorners_perm);
    cubecorners_perm symmetric() const {
        unsigned permRes = perm ^ 0x924924;
        cubecorners_perm ccpres;
        ccpres.perm = (permRes >> 12 | permRes << 12) & 0xffffff;
        return ccpres;
    }
    cubecorners_perm reverse() const;
    cubecorners_perm transform(unsigned transformDir) const;
	bool operator==(const cubecorners_perm &ccp) const { return perm == ccp.perm; }
	bool operator!=(const cubecorners_perm &ccp) const { return perm != ccp.perm; }
	bool operator<(const cubecorners_perm &ccp) const { return perm < ccp.perm; }
    unsigned short getPermIdx() const;
    static cubecorners_perm fromPermIdx(unsigned short);
    bool isPermParityOdd() const;
};

#endif // CUBECORNERSPERM_H
