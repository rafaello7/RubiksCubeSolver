O = -O3
cubesrv: cubesrv.o
	g++ $O -pthread cubesrv.o -o cubesrv

.cc.o:
	g++ -pthread $O -c $<

clean:
	rm -f cubesrv.o cubesrv

tar:
	d=$${PWD##*/}; cd .. && tar czf $$d.tar.gz $$d/*.cc $$d/*.txt $$d/Makefile
