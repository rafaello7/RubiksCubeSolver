#ifndef CUBEEDGES_H
#define CUBEEDGES_H

class cubeedges {
	unsigned long edges;
public:
	cubeedges() : edges(0) {}
	cubeedges(unsigned edge0perm, unsigned edge1perm, unsigned edge2perm,
              unsigned edge3perm, unsigned edge4perm, unsigned edge5perm,
              unsigned edge6perm, unsigned edge7perm, unsigned edge8perm,
              unsigned edge9perm, unsigned edge10perm,unsigned edge11perm,
              unsigned edge0orient, unsigned edge1orient, unsigned edge2orient,
              unsigned edge3orient, unsigned edge4orient, unsigned edge5orient,
              unsigned edge6orient, unsigned edge7orient, unsigned edge8orient,
              unsigned edge9orient, unsigned edge10orient, unsigned edge11orient);

	void setAt(unsigned idx, unsigned char perm, unsigned char orient) {
		edges &= ~(0x1FUL << 5*idx);
		edges |= (unsigned long)(orient<<4 | perm) << 5*idx;
	}
	void setPermAt(unsigned idx, unsigned char perm) {
		edges &= ~(0xFUL << 5*idx);
		edges |= (unsigned long)perm << 5*idx;
	}
	void setOrientAt(unsigned idx, unsigned char orient) {
		edges &= ~(0x1UL << 5*idx+4);
		edges |= (unsigned long)orient << 5*idx+4;
	}
    static cubeedges compose(cubeedges, cubeedges);
    static cubeedges compose3(cubeedges, cubeedges, cubeedges);

    // the middle cubeedges should be reversed before compose
    static cubeedges compose3revmid(cubeedges, cubeedges, cubeedges);
    cubeedges symmetric() const {
        cubeedges ceres;
        // map perm: 0 1 2 3 4 5 6 7 8 9 10 11 -> 8 9 10 11 4 5 6 7 0 1 2 3
        unsigned long edg = edges ^ (~edges & 0x210842108421084ul) << 1;
        ceres.edges = edg >> 40 | edg & 0xfffff00000ul | (edg & 0xfffff) << 40;
        return ceres;
    }
    cubeedges reverse() const;
    cubeedges transform(int idx) const;
	unsigned getPermAt(unsigned idx) const { return edges >> 5 * idx & 0xf; }
	unsigned getOrientAt(unsigned idx) const { return edges >> (5*idx+4) & 1; }
	bool operator==(const cubeedges &ce) const { return edges == ce.edges; }
	bool operator!=(const cubeedges &ce) const { return edges != ce.edges; }
	bool operator<(const cubeedges &ce) const { return edges < ce.edges; }
    unsigned getPermIdx() const;
    unsigned short getOrientIdx() const;
    static cubeedges fromPermAndOrientIdx(unsigned permIdx, unsigned short orientIdx);
    bool isPermParityOdd() const;
    unsigned long get() const { return edges; }
    void set(unsigned long e) { edges = e; }
    bool isNil() const { return edges == 0; }
    bool isBGspace() const;
    bool isYWspace() const;
    bool isORspace() const;
    cubeedges representativeBG() const;
    cubeedges representativeYW() const;
    cubeedges representativeOR() const;
};

#endif // CUBEEDGES_H
