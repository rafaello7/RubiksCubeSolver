package cubesrv;

import static cubesrv.CConsole.*;
import static cubesrv.CServer.*;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class CubeSrv {
    private static class DepthMaxByMem {
        final int memMinMB;
        final int depthMax;
        final boolean useReverse;
        DepthMaxByMem(int memMinMB, int depthMax, boolean useReverse) {
            this.memMinMB   = memMinMB;
            this.depthMax   = depthMax;
            this.useReverse = useReverse;
        }
    }

    private static final DepthMaxByMem[] depthMaxByMem = {
        new DepthMaxByMem(       0, 8, true  ), // ~320MB
        new DepthMaxByMem(    2700, 9, true  ), // ~2.4GB
        new DepthMaxByMem(    5120, 9, false ), // ~4.8GB
        new DepthMaxByMem(1 << 30, 10, true  ), // ~27GB
        new DepthMaxByMem(1 << 30, 10, false ), // ~53GB
    };

    private static int depthMaxSelFn() {
        long memSizeMB = Runtime.getRuntime().maxMemory() / 1048576;
        int model = 0;
        while (model + 1 < depthMaxByMem.length && depthMaxByMem[model+1].memMinMB <= memSizeMB)
            ++model;
        System.out.println("mem MB: " + memSizeMB + ", model: " + model);
        return model;
    }

    private static void printUsage() {
        System.out.println("""
usage:
    cubesrv [<maxdepth>]                       start web server at port 8080
    cubesrv [<maxdepth>] <cubecount>  <mode>   solve random cubes
    cubesrv [<maxdepth>] fname.txt    <mode>   solve cubes from file
    cubesrv [<maxdepth>] <cubestring> <mode>   solve the cube

parameters:
    <maxdepth>          1..10 or 1r..10r
    <cubecount>         a number
    <mode>              o - optimal, q - quick, m - quick multi,
                        O - optimal with preload
""");
    }

    public static void main(String[] args) throws Exception {
        int memModel   = depthMaxSelFn();
        int depthMax   = depthMaxByMem[memModel].depthMax;
        boolean useReverse = depthMaxByMem[memModel].useReverse;

        if (args.length == 1 || args.length == 3) {
            if (args[0].equals("-h") || args[0].equals("--help")) {
                printUsage();
                return;
            }
            Matcher m = Pattern.compile("\\d+").matcher(args[0]);
            if (!m.find()) {
                System.out.println("DEPTH_MAX invalid");
                return;
            }
            depthMax   = Integer.parseInt(m.group());
            useReverse = args[0].contains("r");
            if (depthMax <= 0 || depthMax > 99) {
                System.out.println("DEPTH_MAX invalid: must be in range 1..99");
                return;
            }
        }
        System.out.println("setup: depth " + depthMax + (useReverse ? " rev" : ""));
        if (args.length >= 2) {
            String secondArg = args[args.length-2];
            String modeArg   = args[args.length-1];
            if (Character.isDigit(secondArg.charAt(0)))
                cubeTester(Integer.parseInt(secondArg), modeArg, depthMax, useReverse);
            else
                solveCubes(secondArg, modeArg, depthMax, useReverse);
        } else {
            runServer(depthMax, useReverse);
        }
    }
}

