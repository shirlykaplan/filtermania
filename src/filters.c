#include "filters.h"

void negativeFilter(unsigned char * in, unsigned char * out, int width, int height) 
{
	int totalSize = width * height * 3;
	for (int i = 0; i < totalSize; i++)
	{
		out[i] = 255 - in[i];  // Invert each color component
	}

}
int grayscale(int red,int green,int blue)
{
	int sum;
	int avg;
	 sum = green+blue+red;
	 avg = sum / 3;
	 return avg;
	
}
void grayscalefilter(unsigned char * in, unsigned char * out, int width, int height)
{

    int gray;
	
	int totalSize = width * height * 3;
	for (int i = 0; i < totalSize; i++)
	{
		gray = grayscale(in[i],in[i+1],in[i+2]);
        

        // החלת אותו ערך על כל ערוץ
        out[i] = gray; // R
        out[i + 1] = gray; // G
        out[i + 2] = gray; // B
    }
}
void duplicatefilter(unsigned char * in, unsigned char * out, int width, int height)
{
	int totalSize = width * height * 3;
	for (int i = 0; i < totalSize / 2; i++)
	{
		out[i] = in[i];  
		out[i+totalSize/2] = in[i];
	}
}
void coolFilter(unsigned char * in, unsigned char * out, int width, int height)
{
	int totalSize = width * height * 3; 
	for(int i = 0; i < totalSize /4; i++)
	{
		out[i] = in[i];  
		out[i+totalSize/4] = in[i];
	}

}
void transflag (unsigned char * in, unsigned char * out, int width, int height)
{
	 int totalSize = width * height * 3; 
	 for(int i = 0; i < totalSize; i++)
	{
		out[i] = in[i];  
		out[i+totalSize] = in[i];

	}
}
int transflag (int red,int green,int blue)
{
	int sum;
	int avg;
	sum = green+blue+red;
	avg = sum;
    return 
}
