#ifndef _YUV_H_
#define _YUV_H_
extern "C" {
void YUV420toYUV444(int width, int height, unsigned char* src, unsigned char* dst);
}
#endif
