#ifndef FILTERS_H
#define FILTERS_H

void negativeFilter(unsigned char * in, unsigned char * out, int width, int height);
void grayscalefilter(unsigned char * in, unsigned char * out, int width, int height);
void duplicatefilter(unsigned char * in, unsigned char * out, int width, int height);
void coolFilter(unsigned char * in, unsigned char * out, int width, int height);
#endif
