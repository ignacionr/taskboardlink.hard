all: grabtest

grabtest: grabtest.o yuv.o
	g++ yuv.o grabtest.o -o grabtest -ljpeg -lv4l2

clean:
	rm v4l2grab *.o

yuv.o: yuv.c yuv.h
	g++ -c yuv.c

grabtest.o: grabtest.cpp v4l2grab.hpp yuv.h pantilt.hpp
	g++ -std=c++11 -c grabtest.cpp

