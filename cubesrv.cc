#include "cubesadd.h"
#include "cubesreprbg.h"
#include "cserver.h"
#include "cconsole.h"
#include <iostream>
#include <cstring>
#include <unistd.h>

const struct {
    unsigned memMinMB;
    unsigned depthMax;
    bool useReverse;
} depthMaxByMem[] = {
    { .memMinMB =      0, .depthMax = 8,  .useReverse = true  }, // ~320MB
//  { .memMinMB =    768, .depthMax = 8,  .useReverse = false }, slower than 8 rev; ~600MB
    { .memMinMB =   2700, .depthMax = 9,  .useReverse = true  }, // ~2.4GB
    { .memMinMB =   5120, .depthMax = 9,  .useReverse = false }, // ~4.8GB
    { .memMinMB = 1u<<31, .depthMax = 10, .useReverse = true  }, // ~27GB
    { .memMinMB = 1u<<31, .depthMax = 10, .useReverse = false }, // ~53GB
};

static unsigned depthMaxSelFn() {
    long pageSize = sysconf(_SC_PAGESIZE);
    long pageCount = sysconf(_SC_PHYS_PAGES);
    long memSizeMB = pageSize * pageCount / 1048576;
    unsigned model = 0;
    while( depthMaxByMem[model+1].memMinMB <= memSizeMB )
        ++model;
    std::cout << "mem MB: " << memSizeMB << ", model: " << model << std::endl;
    return model;
}

int main(int argc, char *argv[]) {
    unsigned memModel = depthMaxSelFn();
    unsigned depthMax = depthMaxByMem[memModel].depthMax;
    bool useReverse = depthMaxByMem[memModel].useReverse;

    if( argc >= 2 ) {
        const char *s = argv[1];
        useReverse = strchr(s, 'r') != NULL;
        depthMax = atoi(s);
        if( depthMax <= 0 || depthMax > DEPTH_MAX_AVAIL ) {
            printf("DEPTH_MAX invalid: must be in range 1..%d\n", DEPTH_MAX_AVAIL);
            return 1;
        }
    }
    std::cout << "setup: depth " << depthMax << (useReverse ? " rev" : "") <<
        ASM_SETUP << std::endl;
    permReprInit(useReverse);
    bgspacePermReprInit(useReverse);
    if( argc >= 4 ) {
        if(isdigit(argv[2][0]))
            cubeTester(atoi(argv[2]), argv[3][0], depthMax);
        else
            solveCubes(argv[2], argv[3][0], depthMax);
    }else
        runServer(depthMax);
    return 0;
}

