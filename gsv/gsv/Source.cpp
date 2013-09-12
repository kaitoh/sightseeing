
# pragma comment(lib, "wsock32.lib")	


#include <stdio.h>
#include <string>
#include <math.h>
#include "opencv2/opencv.hpp"
#include <winsock2.h>
#include <ws2tcpip.h>
#include "opencv2/nonfree/nonfree.hpp"
#include <opencv2/legacy/legacy.hpp> 

#define SCALE 1.3

char *filename = "C:\\opencv\\opencv2.4.5\\data\\haarcascades\\haarcascade_frontalface_default.xml";
static int soc = 0;

void open_tonnel( void )
{
	char *dest = "127.0.0.1";
	unsigned short port = 8821;

	WSADATA data;
	WSAStartup(MAKEWORD(2,0), &data);

	soc = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in dest_addr;
	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_addr.s_addr = inet_addr(dest);
	dest_addr.sin_port = htons(port);

	if (connect(soc, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) == SOCKET_ERROR) {
		fprintf(stderr, "connection error\n");
		return;	// todo exception
	} else {
		printf("connect\n");
	}
}

void close_tunnel( void )
{
	closesocket(soc);
	WSACleanup();
}

void send_tunnel(char *in_mes)
{
	send(soc, in_mes, strlen(in_mes), 0);

}

void face_detect( void )
{
	CvCapture *capture = 0;
	IplImage *frame = 0;
	IplImage *frame_copy = 0;
	double height = 480;
	double width = 640;
	int c;
	CvRect last_rect = {0};

	CvHaarClassifierCascade* cvHCC = (CvHaarClassifierCascade*)cvLoad(filename);
	CvMemStorage* cvMStr = cvCreateMemStorage(0);
	CvSeq* face;
	capture = cvCreateCameraCapture (0);
	
	cvSetCaptureProperty (capture, CV_CAP_PROP_FRAME_WIDTH, width);
	cvSetCaptureProperty (capture, CV_CAP_PROP_FRAME_HEIGHT, height);
	cvNamedWindow ("capture_face_detect", CV_WINDOW_AUTOSIZE);
		
	open_tonnel();

	while (1) {
		CvRect near_rect = {0};
		frame = cvQueryFrame (capture);


		frame_copy = cvCreateImage(cvSize(frame->width, frame->height), IPL_DEPTH_8U, frame->nChannels);
		if(frame->origin == IPL_ORIGIN_TL) {
			cvCopy(frame, frame_copy);
		} else {
			cvFlip(frame, frame_copy);
		}

		IplImage* gray = cvCreateImage(cvSize(frame_copy->width, frame_copy->height), IPL_DEPTH_8U, 1);
		IplImage* detect_frame = cvCreateImage(cvSize((frame_copy->width / SCALE), (frame_copy->height / SCALE)), IPL_DEPTH_8U, 1);
		cvCvtColor(frame_copy, gray, CV_BGR2GRAY);
		cvResize(gray, detect_frame, CV_INTER_LINEAR);
		cvEqualizeHist(detect_frame, detect_frame);


		face = cvHaarDetectObjects(detect_frame, cvHCC, cvMStr, 1.1, 2, CV_HAAR_DO_CANNY_PRUNING, cvSize(30, 30) );

		CvScalar detect_color = CV_RGB(255, 0, 0);

		double d = 1000000000000000.0;

		for (int i = 0; i < face->total; i++) {
			CvRect* faceRect = (CvRect*)cvGetSeqElem(face, i);

			if(last_rect.x == 0 && last_rect.y == 0) {

			} else {
				double x = abs(last_rect.x - faceRect->x);
				double y = abs(last_rect.y - faceRect->y);
				double e = sqrt( x*x+y*y );
				if( d > e) {
					last_rect.x = faceRect->x;
					last_rect.y = faceRect->y;
					last_rect.width = faceRect->width;
					last_rect.height = faceRect->height;
					printf("x\n");
				}
			}

			// rect
			cvRectangle(frame_copy,
				cvPoint(faceRect->x * SCALE, faceRect->y * SCALE),
				cvPoint((faceRect->x + faceRect->width) * SCALE, (faceRect->y + faceRect->height) * SCALE),
				detect_color,
				3, CV_AA);
			detect_color = CV_RGB(0, 0, 255);


		}

		// send to server
		{
			char str[1024];
			sprintf_s(str, "[{ \"x\" : %f, \"y\" : %f}]", last_rect.x * SCALE, last_rect.y * SCALE);
			printf("%s", str);
			send_tunnel(str);
		}
		
		cvShowImage ("capture_face_detect", frame_copy);
		cvReleaseImage(&gray);
		cvReleaseImage(&detect_frame);

		// key event
		c = cvWaitKey (16);
		if (c == 'e') {
			break;
		}
		if( c == 's') {
				CvRect* faceRect = (CvRect*)cvGetSeqElem(face, 0);
				if(faceRect != NULL) {
					last_rect.x = faceRect->x;
					last_rect.y = faceRect->y;
					last_rect.width = faceRect->width;
					last_rect.height = faceRect->height;
				}
		}
	}

	close_tunnel();

	/* free */
	cvReleaseMemStorage(&cvMStr);
	cvReleaseCapture (&capture);	
	cvDestroyWindow("capture_face_detect");
	cvReleaseHaarClassifierCascade(&cvHCC);
	


}


int main(int argc, char* argv[]) {
	face_detect();
	return 0;
}
