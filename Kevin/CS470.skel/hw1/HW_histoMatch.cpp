#include "IP.h"
using namespace IP;

void histoMatchApprox(ImagePtr, ImagePtr, ImagePtr);
ImagePtr scaleTargetHisto(ImagePtr, ImagePtr);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HW_histoMatch:
//
// Apply histogram matching to I1. Output is in I2.
//
void
HW_histoMatch(ImagePtr I1, ImagePtr targetHisto, bool approxAlg, ImagePtr I2)
{
	if(approxAlg) {
		histoMatchApprox(I1, targetHisto, I2);
		return;
	}
     // copy image header (width, height) of input image I1 to output image I2
     IP_copyImageHeader(I1, I2);
     int i, p, R;
     int left[MXGRAY], right[MXGRAY];
     int total, Hsum, Havg, h1[MXGRAY];
     double scale;
     // Total number of pixels in image
     //total = (long)I1->height * I1->width;
     int w = I1->width();
     int h = I1->height();
     total = w * h;
     // Init I2 dimensions and buffer
     //I2->width = I1->width;
     //I2->height = I1->height;
     //I2->image = (unsigned char*)malloc(total);
     //in = I1->image;
     //out = I2->image;
     ChannelPtr<uchar> p1, p2;
     int type;

     for (i = 0; i < MXGRAY; i++) h1[i] = 0;

     for (int ch = 0; IP_getChannel(I1, ch, p1, type); ch++) {	// get input pointer for channel ch
          for (i = 0; i < total; i++) h1[*p1++]++;	
     }

     // target histogram
     //h2 = (int *) histo->image;


     ChannelPtr<int> h2;
     IP_getChannel(scaleTargetHisto(I1, targetHisto), 0, h2, type);
     R = 0;
     Hsum = 0;

     int preserve[MXGRAY], left2[MXGRAY];
     // Evaluate remapping of all input gray levels;
     // Each input gray value maps to an interval of valid output values.
     // The endpoints of the intervals are left[] and right[]
     for (i = 0; i < MXGRAY; i++) {
          left[i] = R; // left end of interval
          left2[i] = left[i]; // left[i] will be alternated so keep the data
          preserve[i] = h2[R] - Hsum; // Keep what is going to be lost after entering the while loop
          Hsum += h1[i]; // cumulative value for interval
          while (Hsum > h2[R] && R < MXGRAY - 1) { // compute width of interval
               Hsum -= h2[R]; // adjust Hsum as interval widens
               R++; // update
          }
          right[i] = R; // init right end of interval
     }
     // clear h1 and reuse it below
     for (i = 0; i < MXGRAY; i++) h1[i] = 0;
     // visit all input pixels
     for (int ch = 0; IP_getChannel(I1, ch, p1, type); ch++) {	// get input  pointer for channel ch
          IP_getChannel(I2, ch, p2, type);		// get output pointer for channel ch
          for (i = 0; i < total; i++) {
               p = left[*p1];
               if (h1[p] < h2[p]) { // mapping satisfies h2
                    if (left[*p1] != right[*p1] && left[*p1] == left2[*p1]) { 
                         if (preserve[*p1] > 0) {
                              *p2++ = p;
                              preserve[*p1]--;
                         }
                         else {
                              *p2++ = p = left[*p1] = MIN(p + 1, right[*p1]);
                         }
                    }
                    else {
                         *p2++ = p;
                    }
               }
               else {
                    *p2++ = p = left[*p1] = MIN(p + 1, right[*p1]);
               }
               h1[p]++;
               p1++;
          }
     }
}

void histoMatchApprox(ImagePtr I1, ImagePtr targetHisto, ImagePtr I2)
{
     ChannelPtr<uchar> in, out;
     ChannelPtr<int>hist;
     int type;
     int i;
     int w = I1->width();
     int h = I1->height();
     int total = w * h;
     int histogram[MXGRAY];
     int lut[MXGRAY];
     double normHisto[MXGRAY];
     double normHistoTarget[MXGRAY];
     double runninSumHist[MXGRAY];
     double runninSumHistTarget[MXGRAY];

     // init I2 dimensions and buffer
     IP_copyImageHeader(I1, I2);

     //Scale target Histogram properly
     targetHisto = scaleTargetHisto(I1, targetHisto);

     //Compute Normalize Histogram
     IP_getChannel(I1, 0, in, type);
     for (i = 0; i < MXGRAY; i++) histogram[i] = 0;
     for (i = 0; i < total; i++) histogram[in[i]]++;
     for (i = 0; i < MXGRAY; i++) normHisto[i] = (double)histogram[i] / total;


     //Normalized target Histogram
     IP_getChannel(targetHisto, 0, hist, type);
     for (i = 0; i < MXGRAY; i++) normHistoTarget[i] = (double)hist[i] / total;


     //Compute Running sum for normalize Histogram
     runninSumHist[0] = normHisto[0];
     for (i = 1; i < MXGRAY; i++) runninSumHist[i] = runninSumHist[i - 1] + normHisto[i];

     //Compute Running sum for normalized target Histogram
     runninSumHistTarget[0] = normHistoTarget[0];
     for (i = 1; i < MXGRAY; i++) runninSumHistTarget[i] = runninSumHistTarget[i - 1] + normHistoTarget[i];

     for (i = 0; i < MXGRAY; i++) {
          int l = 255;
          do {
               lut[i] = l;
               l--;
          } while (l >= 0 && runninSumHistTarget[l] > runninSumHist[i]);
     }

     //Copy to output image
     for (int ch = 0; IP_getChannel(I1, ch, in, type); ch++) {    // get input  pointer for channel ch
          IP_getChannel(I2, ch, out, type);        // get output pointer for channel ch
          for (i = 0; i < total; i++) *out++ = lut[*in++];    // use lut[] to eval output
     }
}


ImagePtr scaleTargetHisto(ImagePtr I1, ImagePtr targetHisto) {
     int w = I1->width();
     int h = I1->height();
     int total = w * h; //total number of pixels
     ChannelPtr<int> hist;
     int type;
     int i;


     int Havg = 0;
     IP_getChannel(targetHisto, 0, hist, type);
     for (i = 0; i < MXGRAY; i++) Havg += hist[i];
     double scale = (double)total / Havg;

     if (scale != 1) {
          int numOfPixels = 0;
          for (i = 0; i < MXGRAY; i++) {
               hist[i] *= scale;
               numOfPixels += hist[i];
          }
          int missing = total - numOfPixels; //pixels that were not added successfully due to scale factor
          hist[MXGRAY - 1] += missing; //Adding pixels so that: sum = total num of pixels in image
     }
     return targetHisto;
}