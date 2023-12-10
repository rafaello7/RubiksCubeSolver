#include "cconsole.h"
#include "cubesadd.h"
#include "cuberead.h"
#include "responder.h"
#include "cubesearch.h"
#include <iostream>
#include <cstring>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <map>
#include <vector>


static cube generateCube()
{
    unsigned rndarr[3];

    FILE *fp = fopen("/dev/urandom", "r");
    fread(rndarr, sizeof(rndarr), 1, fp);
    fclose(fp);
    unsigned long rnd = (unsigned long)rndarr[1] << 32 | rndarr[0];
    cube c;
    c.ccp = cubecorners_perm::fromPermIdx(rnd % 40320);
    rnd /= 40320;
    c.cco = cubecorner_orients::fromOrientIdx(rnd % 2187);
    rnd /= 2187;
    unsigned ceOrient = rnd % 2048;
    rnd /= 2048;
    rnd = rnd << 32 | rndarr[2];
    c.ce = cubeedges::fromPermAndOrientIdx(rnd % 479001600, ceOrient);
    if( c.ccp.isPermParityOdd() != c.ce.isPermParityOdd() ) {
        unsigned p = c.ce.getPermAt(10);
        c.ce.setPermAt(10, c.ce.getPermAt(11));
        c.ce.setPermAt(11, p);
    }
    return c;
}

class ConsoleResponder : public Responder {
    const unsigned m_verboseLevel;
    std::string m_solution, m_movecount;
    std::chrono::time_point<std::chrono::system_clock> m_startTime;
    void handleMessage(MessageType, const char*);
public:
    explicit ConsoleResponder(unsigned verboseLevel);
    unsigned durationTimeMs() const;
    std::string durationTime() const;
    const char *getSolution() const { return m_solution.empty() ? NULL : m_solution.c_str(); }
    const char *getMoveCount() const { return m_movecount.c_str(); }
};

ConsoleResponder::ConsoleResponder(unsigned verboseLevel)
    : m_verboseLevel(verboseLevel), m_startTime(std::chrono::system_clock::now())
{
}

unsigned ConsoleResponder::durationTimeMs() const
{
    std::chrono::time_point<std::chrono::system_clock> curTime(std::chrono::system_clock::now());
    auto durns = curTime - m_startTime;
    std::chrono::milliseconds dur = std::chrono::duration_cast<std::chrono::milliseconds>(durns);
    return dur.count();
}

std::string ConsoleResponder::durationTime() const
{
    std::ostringstream os;
    unsigned ms = durationTimeMs();
    unsigned minutes = ms / 60000;
    ms %= 60000;
    unsigned seconds = ms / 1000;
    ms %= 1000;
    if( minutes )
        os << minutes << ":" << std::setw(2) << std::setfill('0');
    os << seconds << "." << std::setw(3) << std::setfill('0') << ms;
    return os.str();
}

void ConsoleResponder::handleMessage(MessageType mt, const char *msg) {
    const char *pad = "                                                 ";
    pad += std::min(strlen(msg), strlen(pad));
    switch(mt) {
    case MT_UNQUALIFIED:
        if( m_verboseLevel ) {
            flockfile(stdout);
            std::cout << "\r" << durationTime() << " " << msg << pad << std::flush;
            if( !strncmp(msg, "finished at ", 12) || m_verboseLevel >= 3 )
                std::cout << std::endl;
            funlockfile(stdout);
        }
        break;
    case MT_PROGRESS:
        if( m_verboseLevel > 1 ) {
            flockfile(stdout);
            std::cout << "\r" << durationTime() << " " << msg << pad << std::flush;
            funlockfile(stdout);
        }
        break;
    case MT_MOVECOUNT:
        flockfile(stdout);
        std::cout << "\r" << durationTime() << " moves: " << msg << pad << std::endl;
        m_movecount = msg;
        funlockfile(stdout);
        break;
    case MT_SOLUTION:
        flockfile(stdout);
        std::cout << "\r" << durationTime() << msg << pad << std::endl;
        m_solution = msg;
        funlockfile(stdout);
        break;
    }
}

static void solveCubes(const std::vector<cube> &cubes, char mode, unsigned depthMax)
{
    if( mode == 'O' ) {
        ConsoleResponder responder(2);
        getReprCubes(depthMax, responder);
        std::cout << std::endl;
    }
    std::map<std::string, std::pair<unsigned, unsigned>> moveCounters;
    for(unsigned i = 0; i < cubes.size(); ++i) {
        cube c = cubes[i];
        std::cout << i << "  " << c.toParamText() << std::endl;
        std::cout << std::endl;
        cubePrint(c);
        ConsoleResponder responder(mode == 'q' ? 0 : mode == 'm' ? 1 : 2);
        searchMoves(c, mode, depthMax, responder);
        std::cout << std::endl;
        const char *s = responder.getSolution();
        while( s ) {
            if( *s == ' ' )
                ++s;
            int rd = rotateNameToDir(s);
            if( rd == RCOUNT ) {
                std::cout << "fatal: unrecognized move " << s << std::endl;
                return;
            }
            c = cube::compose(c, crotated[rd]);
            s = strchr(s, ' ');
        }
        if( c != csolved ) {
            std::cout << "fatal: bad solution!" << std::endl;
            return;
        }
        std::pair<unsigned, unsigned> &ent = moveCounters[responder.getMoveCount()];
        ++ent.first;
        ent.second += responder.durationTimeMs();
    }
    unsigned durationTimeTot = 0;
    std::cout << "          moves  cubes  avg time (s)" << std::endl;
    std::cout << "---------------  -----  -------------" << std::endl;
    for(auto [moveCount, stats] : moveCounters) {
        std::cout << std::setw(15) << moveCount << std::setw(7) <<
            stats.first << "  " << std::setprecision(4) <<
            stats.second/1000.0/stats.first << std::endl;
        durationTimeTot += stats.second;
    }
    std::cout << "---------------  -----  -------------" << std::endl;
        std::cout << "total " << std::setw(7) << durationTimeTot/1000.0 << " s" << std::setw(7) <<
            cubes.size() << "  " << std::setprecision(4) <<
            durationTimeTot/1000.0/cubes.size() << std::endl;
    std::cout << std::endl;
}

void solveCubes(const char *fnameOrCubeStr, char mode, unsigned depthMax)
{
    std::vector<cube> cubes;

    int len = strlen(fnameOrCubeStr);
    if( len > 4 && !strcmp(fnameOrCubeStr+len-4, ".txt") ) {
        FILE *fp = fopen(fnameOrCubeStr, "r");
        if( fp == NULL ) {
            perror(fnameOrCubeStr);
            return;
        }
        ConsoleResponder responder(2);
        cube c;
        char buf[2000];
        while( fgets(buf, sizeof(buf), fp) ) {
            if( !cubeFromString(responder, buf, c) ) {
                fclose(fp);
                return;
            }
            cubes.push_back(c);
        }
        fclose(fp);
    }else{
        ConsoleResponder responder(2);
        cube c;
        if( !cubeFromString(responder, fnameOrCubeStr, c) )
            return;
        cubes.push_back(c);
    }
    solveCubes(cubes, mode, depthMax);
}

void cubeTester(unsigned cubeCount, char mode, unsigned depthMax)
{
    std::vector<cube> cubes;

    for(unsigned i = 0; i < cubeCount; ++i)
        cubes.push_back(generateCube());
    solveCubes(cubes, mode, depthMax);
}

