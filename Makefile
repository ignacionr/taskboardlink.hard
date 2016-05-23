all: grabtest

grab-upload: images.tar
	rsync images.tar root@dev2.run.fastmicroservices.com:/root/Data/.

images.tar: grabtest
	./grabtest && tar -cf images.tar *.jpg && rm *.jpg

grabtest: grabtest.o yuv.o
	g++ yuv.o grabtest.o -o grabtest -ljpeg -lv4l2

clean:
	rm v4l2grab *.o

yuv.o: yuv.c yuv.h
	g++ -c yuv.c

grabtest.o: grabtest.cpp v4l2grab.hpp yuv.h pantilt.hpp
	g++ -std=c++11 -c grabtest.cpp

