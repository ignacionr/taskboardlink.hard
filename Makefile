all: grabtest

grabtest: grabtest.o yuv.o
	gcc yuv.o grabtest.o -o grabtest -ljpeg -lv4l2

clean:
	rm v4l2grab *.o

yuv.o: yuv.c yuv.h
	gcc -c yuv.c

grabtest.o: grabtest.cpp v4l2grab.hpp yuv.h
	gcc -std=c++11 -c grabtest.cpp

