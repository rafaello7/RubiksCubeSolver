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

static void printUsage()
{
    std::cout << "usage:" << std::endl
        << "    cubesrv [<maxdepth>]\t\t       start web server at port 8080" << std::endl
        << "    cubesrv [<maxdepth>] <cubecount>  <mode>   solve random cubes" << std::endl
        << "    cubesrv [<maxdepth>] fname.txt    <mode>   solve cubes from file" << std::endl
        << "    cubesrv [<maxdepth>] <cubestring> <mode>   solve the cube" << std::endl
        << std::endl
        << "parameters:" << std::endl
        << "    <maxdepth>\t\t1..10 or 1r..10r" << std::endl
        << "    <cubecount>\t\ta number" << std::endl
        << "    <mode>\t\to - optimal, q - quick, m - quick multi," << std::endl
        << "\t\t\tO - optimal with preload" << std::endl
        << std::endl;
}

int main(int argc, char *argv[]) {
    unsigned memModel = depthMaxSelFn();
    unsigned depthMax = depthMaxByMem[memModel].depthMax;
    bool useReverse = depthMaxByMem[memModel].useReverse;

    if( argc == 2 || argc == 4 ) {
        if( !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help") ) {
            printUsage();
            return 0;
        }
        const char *s = argv[1];
        useReverse = strchr(s, 'r') != NULL;
        depthMax = atoi(s);
        if( depthMax <= 0 || depthMax > 99 ) {
            printf("DEPTH_MAX invalid: must be in range 1..99\n");
            return 1;
        }
    }
    std::cout << "setup: depth " << depthMax << (useReverse ? " rev" : "") <<
        ASM_SETUP << std::endl;
    if( argc >= 3 ) {
        if(isdigit(argv[argc-2][0]))
            cubeTester(atoi(argv[argc-2]), argv[argc-1][0], depthMax, useReverse);
        else
            solveCubes(argv[argc-2], argv[argc-1][0], depthMax, useReverse);
    }else
        runServer(depthMax, useReverse);
    return 0;
}

