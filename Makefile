O = -O3
cubesrv: cubesrv.o
	g++ $O -pthread cubesrv.o -o cubesrv

.cc.o:
	g++ -pthread $O -c $<

clean:
	rm -f cubesrv.o cubesrv

tar:
	d=$${PWD##*/}; cd .. && tar czf $$d.tar.gz $$d/*.[chij]* $$d/Makefile

perf:
	sudo perf stat -e 'assists.sse_avx_mix' ./cubesrv

perf9r:
	sudo perf stat -e 'assists.sse_avx_mix' ./cubesrv 9r
