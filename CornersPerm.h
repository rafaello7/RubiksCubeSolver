#ifndef CORNERSPERM_H
#define CORNERSPERM_H

class CornersPerm {
	unsigned perm;
public:
	CornersPerm() : perm(0) {}
	CornersPerm(unsigned corner0perm, unsigned corner1perm, unsigned corner2perm,
			unsigned corner3perm, unsigned corner4perm, unsigned corner5perm,
			unsigned corner6perm, unsigned corner7perm);

	void setAt(unsigned idx, unsigned char p) {
		perm &= ~(7 << 3*idx);
		perm |= p << 3*idx;
	}
	unsigned getAt(unsigned idx) const { return perm >> 3*idx & 7; }
	unsigned get() const { return perm; }
	void set(unsigned p) { perm = p; }
    static CornersPerm compose(CornersPerm, CornersPerm);
    static CornersPerm compose3(CornersPerm, CornersPerm, CornersPerm);
    CornersPerm symmetric() const {
        unsigned permRes = perm ^ 0x924924;
        CornersPerm ccpres;
        ccpres.perm = (permRes >> 12 | permRes << 12) & 0xffffff;
        return ccpres;
    }
    CornersPerm reverse() const;
    CornersPerm transform(unsigned transformDir) const;
	bool operator==(const CornersPerm &ccp) const { return perm == ccp.perm; }
	bool operator!=(const CornersPerm &ccp) const { return perm != ccp.perm; }
	bool operator<(const CornersPerm &ccp) const { return perm < ccp.perm; }
    unsigned short getPermIdx() const;
    static CornersPerm fromPermIdx(unsigned short);
    bool isPermParityOdd() const;
};

#endif // CORNERSPERM_H
