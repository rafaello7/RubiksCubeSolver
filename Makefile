O = -O3

OBJS = cubedefs.o cubesrepr.o cubesreprbg.o cubecosets.o responder.o cuberead.o \
	   progressbase.o threadpoolhelper.o cubesadd.o cubesaddbg.o cubecosetsadd.o \
	   searchoptimal.o searchbg.o searchquick.o cubesearch.o cserver.o cconsole.o \
	   cubesrv.o

cubesrv: $(OBJS)
	g++ $O -pthread $(OBJS) -o cubesrv

.cc.o:
	g++ -pthread $O -c -Wall -Wno-parentheses -Wno-unused-function $<

clean:
	rm -f $(OBJS) cubesrv

tar:
	d=$${PWD##*/}; cd .. && tar czf $$d.tar.gz $$d/*.[chijm]* $$d/Makefile

perf:
	sudo perf stat -e 'assists.sse_avx_mix' ./cubesrv

perf9r:
	sudo perf stat -e 'assists.sse_avx_mix' ./cubesrv 9r

pkg: cubesrv
	d=$${PWD##*/}; cd .. && tar czf $$d-$$(uname -m).tar.gz $$d/cube.* $$d/favicon.ico $$d/cubesrv
