#ifdef __cplusplus
#include <opencv2/opencv.hpp>
#endif

using namespace cv;

extern "C"
{
    void MotionTrail(char* currentData, char* prevData, int width, int height);
}

// various tracking parameters (in seconds)
const double MHI_DURATION = 1;
const double MAX_TIME_DELTA = 0.5;
const double MIN_TIME_DELTA = 0.05;
// number of cyclic frame buffer used for motion detection
// (should, probably, depend on FPS)
const int N = 4;

// ring image buffer
IplImage **buf = 0;
int last = 0;

// temporary images
IplImage *mhi = 0; // MHI
IplImage *orient = 0; // orientation
IplImage *mask = 0; // valid orientation mask
IplImage *segmask = 0; // motion segmentation map
CvMemStorage* storage = 0; // temporary storage

// parameters:
//  img - input video frame
//  dst - resultant motion picture
//  args - optional parameters
static void  update_mhi( IplImage* img, IplImage* dst, int diff_threshold )
{
    double timestamp = (double)clock()/CLOCKS_PER_SEC; // get current time in seconds
    CvSize size = cvSize(img->width,img->height); // get current frame size
    int i, idx1 = last, idx2;
    IplImage* silh;
    CvSeq* seq;
    CvRect comp_rect;
    double count;
    double angle;
    CvPoint center;
    double magnitude;
    CvScalar color;
    
    // allocate images at the beginning or
    // reallocate them if the frame size is changed
    if( !mhi || mhi->width != size.width || mhi->height != size.height ) {
        if( buf == 0 ) {
            buf = (IplImage**)malloc(N*sizeof(buf[0]));
            memset( buf, 0, N*sizeof(buf[0]));
        }
        
        for( i = 0; i < N; i++ ) {
            cvReleaseImage( &buf[i] );
            buf[i] = cvCreateImage( size, IPL_DEPTH_8U, 1 );
            cvZero( buf[i] );
        }
        cvReleaseImage( &mhi );
        cvReleaseImage( &orient );
        cvReleaseImage( &segmask );
        cvReleaseImage( &mask );
        
        mhi = cvCreateImage( size, IPL_DEPTH_32F, 1 );
        cvZero( mhi ); // clear MHI at the beginning
        orient = cvCreateImage( size, IPL_DEPTH_32F, 1 );
        segmask = cvCreateImage( size, IPL_DEPTH_32F, 1 );
        mask = cvCreateImage( size, IPL_DEPTH_8U, 1 );
    }
    
    cvCvtColor( img, buf[last], CV_BGR2GRAY ); // convert frame to grayscale
    
    idx2 = (last + 1) % N; // index of (last - (N-1))th frame
    last = idx2;
    
    silh = buf[idx2];
    cvAbsDiff( buf[idx1], buf[idx2], silh ); // get difference between frames
    
    cvThreshold( silh, silh, diff_threshold, 1, CV_THRESH_BINARY ); // and threshold it
    cvUpdateMotionHistory( silh, mhi, timestamp, MHI_DURATION ); // update MHI
    
    // convert MHI to blue 8u image
    cvCvtScale( mhi, mask, 255./MHI_DURATION,
               (MHI_DURATION - timestamp)*255./MHI_DURATION );
    cvZero( dst );
    cvMerge( 0, 0, mask, 0, dst );
    
    // calculate motion gradient orientation and valid orientation mask
    cvCalcMotionGradient( mhi, mask, orient, MAX_TIME_DELTA, MIN_TIME_DELTA, 3 );
    
    if( !storage )
        storage = cvCreateMemStorage(0);
    else
        cvClearMemStorage(storage);
    
    // segment motion: get sequence of motion components
    // segmask is marked motion components map. It is not used further
    seq = cvSegmentMotion( mhi, segmask, storage, timestamp, MAX_TIME_DELTA );
    
    // iterate through the motion components,
    // One more iteration (i == -1) corresponds to the whole image (global motion)
    /*for( i = -1; i < seq->total; i++ ) {
        
        if( i < 0 ) { // case of the whole image
            comp_rect = cvRect( 0, 0, size.width, size.height );
            color = CV_RGB(255,255,255);
            magnitude = 100;
        }
        else { // i-th motion component
            comp_rect = ((CvConnectedComp*)cvGetSeqElem( seq, i ))->rect;
            if( comp_rect.width + comp_rect.height < 100 ) // reject very small components
                continue;
            color = CV_RGB(255,0,0);
            magnitude = 30;
        }
        
        // select component ROI
        cvSetImageROI( silh, comp_rect );
        cvSetImageROI( mhi, comp_rect );
        cvSetImageROI( orient, comp_rect );
        cvSetImageROI( mask, comp_rect );
        
        // calculate orientation
        angle = cvCalcGlobalOrientation( orient, mask, mhi, timestamp, MHI_DURATION);
        angle = 360.0 - angle;  // adjust for images with top-left origin
        
        count = cvNorm( silh, 0, CV_L1, 0 ); // calculate number of points within silhouette ROI
        
        cvResetImageROI( mhi );
        cvResetImageROI( orient );
        cvResetImageROI( mask );
        cvResetImageROI( silh );
        
        // check for the case of little motion
        if( count < comp_rect.width*comp_rect.height * 0.05 )
            continue;
        
        // draw a clock with arrow indicating the direction
        center = cvPoint( (comp_rect.x + comp_rect.width/2),
                         (comp_rect.y + comp_rect.height/2) );
        
        cvCircle( dst, center, cvRound(magnitude*1.2), color, 3, CV_AA, 0 );
        cvLine( dst, center, cvPoint( cvRound( center.x + magnitude*cos(angle*CV_PI/180)),
                                     cvRound( center.y - magnitude*sin(angle*CV_PI/180))), color, 3, CV_AA, 0 );
    }*/
}

void MotionTrail(char* currentData, char* prevData, int width, int height)
{
    IplImage* currentImage = cvCreateImageHeader(cvSize(width, height), IPL_DEPTH_8U, 4);
    cvSetData(currentImage, currentData, currentImage->widthStep);
    
    IplImage* prevImage = cvCreateImageHeader(cvSize(width, height), IPL_DEPTH_8U, 4);
    cvSetData(prevImage, prevData, prevImage->widthStep);
    
    IplImage* trailsImage = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 4);
    
    update_mhi(prevImage, trailsImage, 30);
    
    Mat currentMat = Mat(currentImage);
    Mat prevMat = Mat(prevImage);
    Mat trailsMat = Mat(trailsImage);
    
    add(trailsMat, currentMat, currentMat);
    
    cvReleaseImageHeader(&currentImage);
    cvReleaseImageHeader(&prevImage);
    cvReleaseImage(&trailsImage);
}
