#ifdef __cplusplus
#include <opencv2/opencv.hpp>
#endif

using namespace cv;

extern "C"
{
    void MotionTrail(char* currentData, char* prevData, int width, int height, int hCycle);
}

void MotionTrail(char* currentData, char* prevData, int width, int height, int hCycle)
{
    IplImage* currentImage = cvCreateImageHeader(cvSize(width, height), IPL_DEPTH_8U, 4);
    cvSetData(currentImage, currentData, currentImage->widthStep);
    
    IplImage* prevImage = cvCreateImageHeader(cvSize(width, height), IPL_DEPTH_8U, 4);
    cvSetData(prevImage, prevData, prevImage->widthStep);
    
    IplImage* trailsImage = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);
    
    Mat currentMat = Mat(currentImage);
    Mat prevMat = Mat(prevImage);
    Mat trailsMat = Mat(trailsImage);
    
    Mat diffMat, grayMat, blurMat;
    
    absdiff(currentMat, prevMat, diffMat);
    cvtColor(diffMat, grayMat, CV_RGBA2GRAY);
    blur(grayMat, blurMat, Size(3,3));
    threshold(blurMat, blurMat, 20, 255, CV_THRESH_BINARY);
    add(trailsMat, blurMat, trailsMat);
    cvtColor(trailsMat, trailsMat, CV_GRAY2RGBA);
    add(trailsMat, currentMat, currentMat);
    cvReleaseImageHeader(&currentImage);
    cvReleaseImageHeader(&prevImage);
    cvReleaseImage(&trailsImage);
}
