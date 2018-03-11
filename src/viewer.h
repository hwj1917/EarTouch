#ifndef viewer_h
#define viewer_h

#define _CRT_SECURE_NO_WARNINGS
#define BUFFER_SIZE 20
#include "picker.h"
#include "InjectEventController.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <deque>
#include <vector>
#include <mutex>
#include <numeric>
#include <stdio.h>
#include <time.h>
#include <cv.h>
#include <highgui.h>
#include <cv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/tracking/tracker.hpp>

using namespace cv;
using namespace std;

class Viewer {
public:
    static deque<Frame> frames_buffer;
	static deque<Point> points_buffer;
	static deque<Point> ropoints_buffer;
	static deque<Point> realpoints_buffer;

	static deque<Point> stablepoints_buffer;
	static deque<Point> sums_buffer;
	static deque<int> deltas_buffer;

	static SOCKADDR_IN servAddr;
	static SOCKET sSocket;
	static SOCKET sockClient;
	static int nServAddLen;
	static char buf[BUFFER_SIZE];
	static WSADATA wsd;

	static deque<Mat> review_buffer;

    static void run(int argc, char **argv);

	static void displayFrameCV(Frame &frame);

	static void initUDP();

	static void sent(int x, int y, string type);

	static void* draw(void* nouse);

	static InjectEventController m_inject;


private:

    static char viewer_type;
    static char frame_type;
    static int frame_index;
    static int region_index;
    static int region_info_switch;
    static int last_frame_classify_result;
	static int maxcap;
	static int leavesum;
	static int stablesum;
	static int dirtylow;


    
    static float calculateColor(int value);
    static void displayText(void *font, char *text, float x, float y);
    static void displayCapacity(int value, int region, int isStable, float x, float y);
    static void displayFrame(Frame &frame, float dx, float dy);
	static void gravityCenter(Mat src,Point &cent);

    static void display();
    static void special(int key, int x, int y);
    static void keyboard(unsigned char key, int x, int y);

	static bool last_dirty, has_start;

	static Mat pattern, lanc, Image[2];

	static Point last_result, last_point[2], start_point, end_point;
	static int pattern_sum;
	static int lastsum, deltasum;
	static Ptr<TrackerKCF> tracker[2];

	static void* update(void*);
	static void* update_last(void*);


	static bool judgeDirty(int);
	static void sendPoint(bool, Point);
	static void checkSpin(int, Mat&, double);
};

#endif /* viewer_h */
