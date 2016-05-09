/***************************************************************************
 *   v4l2grab Version 0.3                                                  *
 *   Copyright (C) 2012 by Tobias Müller                                   *
 *   Tobias_Mueller@twam.info                                              *
 *                                                                         *
 *   based on V4L2 Specification, Appendix B: Video Capture Example        *
 *   (http://v4l2spec.bytesex.org/spec/capture-example.html)               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
 
 /**************************************************************************
 *   Modification History                                                  *
 *                                                                         *
 *   Matthew Witherwax      21AUG2013                                      *
 *      Added ability to change frame interval (ie. frame rate/fps)        *
 * Martin Savc              7JUL2015
 *      Added support for continuous capture using SIGINT to stop.
 ***************************************************************************/

// compile with all three access methods
#if !defined(IO_READ) && !defined(IO_MMAP) && !defined(IO_USERPTR)
#define IO_READ
#define IO_MMAP
#define IO_USERPTR
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <linux/videodev2.h>
#include <jpeglib.h>
#include <libv4l2.h>
#include <signal.h>
#include <stdint.h>
#include <inttypes.h>

#include <functional>
#include <fstream>

#include "config.h"
#include "yuv.h"

class Grabber {

#define CLEAR(x) memset (&(x), 0, sizeof (x))

#define VIDIOC_REQBUFS_COUNT 4

#ifndef VERSION
#define VERSION "unknown"
#endif

typedef enum {
        IO_METHOD_READ,
        IO_METHOD_MMAP,
        IO_METHOD_USERPTR,
} io_method;

struct buffer {
        void *                  start;
        size_t                  length;
};

	io_method        io              = IO_METHOD_MMAP;
	int              fd              = -1;
	struct buffer *         buffers         = NULL;
	unsigned int     n_buffers       = 0;

// global settings
 unsigned int width = 640;
 unsigned int height = 480;
//  unsigned int fps = 30;
//  int continuous = 0;
 unsigned char jpegQuality = 70;
 const char* jpegFilename = NULL;
 char* jpegFilenamePart = NULL;
 const char* deviceName = "/dev/video0";
 bool _writeJpeg = true;
 bool _writeRaw = false;
 std::function<bool(int,int,unsigned char*)> _processor;

/**
SIGINT interput handler
*/
// void StopContCapture(int sig_id) {
// 	printf("stoping continuous capture\n");
// 	continuous = 0;
// }

/**
	Print error message and terminate programm with EXIT_FAILURE return code.

	\param s error message to print
*/
static void errno_exit(const char* s)
{
	fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
	exit(EXIT_FAILURE);
}

/**
	Do ioctl and retry if error was EINTR ("A signal was caught during the ioctl() operation."). Parameters are the same as on ioctl.

	\param request request
	\param argp argument
	\returns result from ioctl
*/
int xioctl(int request, void* argp)
{
	int r;

	do r = v4l2_ioctl(fd, request, argp);
	while (-1 == r && EINTR == errno);

	return r;
}

/**
	Write image to jpeg file.

	\param img image to write
*/
void jpegWrite(unsigned char* img, const char* jpegFilename)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	JSAMPROW row_pointer[1];
	FILE *outfile = fopen( jpegFilename, "wb" );

	// try to open file for saving
	if (!outfile) {
		errno_exit("jpeg");
	}

	// create jpeg data
	cinfo.err = jpeg_std_error( &jerr );
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, outfile);

	// set image parameters
	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_YCbCr;

	// set jpeg compression parameters to default
	jpeg_set_defaults(&cinfo);
	// and then adjust quality setting
	jpeg_set_quality(&cinfo, jpegQuality, TRUE);

	// start compress
	jpeg_start_compress(&cinfo, TRUE);

	// feed data
	while (cinfo.next_scanline < cinfo.image_height) {
		row_pointer[0] = &img[cinfo.next_scanline * cinfo.image_width *  cinfo.input_components];
		jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	// finish compression
	jpeg_finish_compress(&cinfo);

	// destroy jpeg data
	jpeg_destroy_compress(&cinfo);

	// close output file
	fclose(outfile);
}

/**
	process image read
*/
bool imageProcess(const void* p, struct timeval timestamp)
{
	//timestamp.tv_sec
	//timestamp.tv_usec
	unsigned char* src = (unsigned char*)p;
	auto imgSize = width*height*3*sizeof(char);
	unsigned char* dst = (unsigned char*)malloc(imgSize);

	YUV420toYUV444(width, height, src, dst);
	
	bool writeFile = (_processor) ? _processor(width, height, dst) : true;
	

	if (writeFile && _writeRaw)
		{
			std::ofstream outfile(jpegFilename, std::ofstream::binary);
			outfile.write((const char *)dst, imgSize);
		}

	if (writeFile && _writeJpeg)
		jpegWrite(dst,jpegFilename);

	// free temporary image
	free(dst);
	
	return writeFile;
}

/**
	read single frame
*/
int frameRead(bool &result)
{
	struct v4l2_buffer buf;

	unsigned int i;

	CLEAR(buf);

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl(VIDIOC_DQBUF, &buf)) {
		switch (errno) {
			case EAGAIN:
				return 0;

			case EIO:
				// Could ignore EIO, see spec
				// fall through

			default:
				errno_exit("VIDIOC_DQBUF");
		}
	}

	assert(buf.index < n_buffers);

	result = imageProcess(buffers[buf.index].start,buf.timestamp);

	if (-1 == xioctl(VIDIOC_QBUF, &buf))
		errno_exit("VIDIOC_QBUF");


	return 1;
}

/**
	mainloop: read frames and process them
*/
bool mainLoop(void)
{	
	int count;
	bool result;
	unsigned int numberOfTimeouts;

	numberOfTimeouts = 0;
	count = 3;

	while (count-- > 0) {
		for (;;) {
			fd_set fds;
			struct timeval tv;
			int r;

			FD_ZERO(&fds);
			FD_SET(fd, &fds);

			/* Timeout. */
			tv.tv_sec = 1;
			tv.tv_usec = 0;

			r = select(fd + 1, &fds, NULL, NULL, &tv);

			if (-1 == r) {
				if (EINTR == errno)
					continue;

				errno_exit("select");
			}

			if (0 == r) {
				if (numberOfTimeouts <= 0) {
					count++;
				} else {
					fprintf(stderr, "select timeout\n");
					exit(EXIT_FAILURE);
				}
			}

			if (frameRead(result))
				break;

			/* EAGAIN - continue select loop. */
		}
	}
	return result;
}

/**
	stop capturing
*/
void captureStop(void)
{
	enum v4l2_buf_type type;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (-1 == xioctl(VIDIOC_STREAMOFF, &type))
	errno_exit("VIDIOC_STREAMOFF");
}

/**
  start capturing
*/
void captureStart(void)
{
	unsigned int i;
	enum v4l2_buf_type type;

	for (i = 0; i < n_buffers; ++i) {
		struct v4l2_buffer buf;

		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;

		if (-1 == xioctl(VIDIOC_QBUF, &buf))
			errno_exit("VIDIOC_QBUF");
		}

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (-1 == xioctl(VIDIOC_STREAMON, &type))
		errno_exit("VIDIOC_STREAMON");

}

void deviceUninit(void)
{
	unsigned int i;

	for (i = 0; i < n_buffers; ++i)
		if (-1 == v4l2_munmap(buffers[i].start, buffers[i].length))
			errno_exit("munmap");
	free(buffers);
}


void mmapInit(void)
{
	struct v4l2_requestbuffers req;

	CLEAR(req);

	req.count = VIDIOC_REQBUFS_COUNT;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl(VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s does not support memory mapping\n", deviceName);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_REQBUFS");
		}
	}

	if (req.count < 2) {
		fprintf(stderr, "Insufficient buffer memory on %s\n", deviceName);
		exit(EXIT_FAILURE);
	}

	buffers = (buffer*)calloc(req.count, sizeof(*buffers));

	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
		struct v4l2_buffer buf;

		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = n_buffers;

		if (-1 == xioctl(VIDIOC_QUERYBUF, &buf))
			errno_exit("VIDIOC_QUERYBUF");

		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start = v4l2_mmap(NULL /* start anywhere */, buf.length, PROT_READ | PROT_WRITE /* required */, MAP_SHARED /* recommended */, fd, buf.m.offset);

		if (MAP_FAILED == buffers[n_buffers].start)
			errno_exit("mmap");
	}
}

void userptrInit(unsigned int buffer_size)
{
	struct v4l2_requestbuffers req;
	unsigned int page_size;

	page_size = getpagesize();
	buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);

	CLEAR(req);

	req.count = VIDIOC_REQBUFS_COUNT;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;

	if (-1 == xioctl(VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s does not support user pointer i/o\n", deviceName);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_REQBUFS");
		}
	}

	buffers = (buffer*)calloc(4, sizeof(*buffers));

	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
		buffers[n_buffers].length = buffer_size;
		buffers[n_buffers].start = memalign(/* boundary */ page_size, buffer_size);

		if (!buffers[n_buffers].start) {
			fprintf(stderr, "Out of memory\n");
			exit(EXIT_FAILURE);
		}
	}
}


/**
	initialize device
*/
void deviceInit(void)
{
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;
	struct v4l2_streamparm frameint;
	unsigned int min;

	if (-1 == xioctl(VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s is no V4L2 device\n",deviceName);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_QUERYCAP");
		}
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf(stderr, "%s is no video capture device\n",deviceName);
		exit(EXIT_FAILURE);
	}

	switch (io) {
		case IO_METHOD_READ:
			if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
				fprintf(stderr, "%s does not support read i/o\n",deviceName);
				exit(EXIT_FAILURE);
			}
			break;
		case IO_METHOD_MMAP:
		case IO_METHOD_USERPTR:
      			if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
				fprintf(stderr, "%s does not support streaming i/o\n",deviceName);
				exit(EXIT_FAILURE);
			}
			break;
	}

	/* Select video input, video standard and tune here. */
	CLEAR(cropcap);

	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (0 == xioctl(VIDIOC_CROPCAP, &cropcap)) {
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; /* reset to default */

		if (-1 == xioctl(VIDIOC_S_CROP, &crop)) {
			switch (errno) {
				case EINVAL:
					/* Cropping not supported. */
					break;
				default:
					/* Errors ignored. */
					break;
			}
		}
	} else {
		/* Errors ignored. */
	}

	CLEAR(fmt);

	// v4l2_format
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = width;
	fmt.fmt.pix.height = height;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;

	if (-1 == xioctl(VIDIOC_S_FMT, &fmt))
		errno_exit("VIDIOC_S_FMT");

	if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_YUV420) {
		fprintf(stderr,"Libv4l didn't accept YUV420 format. Can't proceed.\n");
		exit(EXIT_FAILURE);
	}

	/* Note VIDIOC_S_FMT may change width and height. */
	if (width != fmt.fmt.pix.width) {
		width = fmt.fmt.pix.width;
		fprintf(stderr,"Image width set to %i by device %s.\n", width, deviceName);
	}

	if (height != fmt.fmt.pix.height) {
		height = fmt.fmt.pix.height;
		fprintf(stderr,"Image height set to %i by device %s.\n", height, deviceName);
	}
	
  /* If the user has set the fps to -1, don't try to set the frame interval */
//   if (fps != -1)
//   {
//     CLEAR(frameint);
    
//     /* Attempt to set the frame interval. */
//     frameint.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//     frameint.parm.capture.timeperframe.numerator = 1;
//     frameint.parm.capture.timeperframe.denominator = fps;
//     if (-1 == xioctl(VIDIOC_S_PARM, &frameint))
//       fprintf(stderr,"Unable to set frame interval.\n");
//   }

	/* Buggy driver paranoia. */
	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;

	mmapInit();
}

/**
	close device
*/
void deviceClose(void)
{
	if (-1 == v4l2_close(fd))
		errno_exit("close");

	fd = -1;
}

/**
	open device
*/
void deviceOpen(void)
{
	struct stat st;

	// stat file
	if (-1 == stat(deviceName, &st)) {
		fprintf(stderr, "Cannot identify '%s': %d, %s\n", deviceName, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	// check if its device
	if (!S_ISCHR(st.st_mode)) {
		fprintf(stderr, "%s is no device\n", deviceName);
		exit(EXIT_FAILURE);
	}

	// open device
	fd = v4l2_open(deviceName, O_RDWR /* required */ | O_NONBLOCK, 0);

	// check if opening was successfull
	if (-1 == fd) {
		fprintf(stderr, "Cannot open '%s': %d, %s\n", deviceName, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
}

public:
	Grabber(
		const char *oDeviceName = NULL,  
		int oQuality = 0, 
		int oWidth = 0, 
		int oHeight = 0)
	{
		if (oDeviceName)
			deviceName = oDeviceName;
		if (oQuality)
			jpegQuality = oQuality;
		if (oWidth)
			width = oWidth;
		if (oHeight)
			height = oHeight;
		io = IO_METHOD_MMAP;
	}
	
	void setPreprocessor (std::function<bool(int,int,unsigned char*)> fnPreprocessor) {
		_processor = fnPreprocessor;
	}
	
	void setOutput(bool jpeg, bool omit = false) {
		_writeRaw = !jpeg && !omit;
		_writeJpeg = jpeg && !omit;
	}
	
	void Open() {
		// open and initialize device
		deviceOpen();
		deviceInit();
		// start capturing
		captureStart();
	}
	
	bool Capture(const char *oJpegFilename=NULL) {
		if (oJpegFilename)
			jpegFilename = oJpegFilename;

		// process frames
		return mainLoop();

	}
	
	void Close() {
		// stop capturing
		captureStop();
		// close device
		deviceUninit();
		deviceClose();
	}
};
