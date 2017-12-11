#ifndef GLUT_DISABLE_ATEXIT_HACK
#define GLUT_DISABLE_ATEXIT_HACK
#endif

#include "viewer.h"
#include <ctime>
#include <iostream>

#include <winsock.h>
#include <string.h>

#pragma comment(lib,"WS2_32.lib")  

#undef TEXT
#define TEXT(font, x, y, fmt, ...) do { char *text = new char[MAX_LOG_LENGTH + 1]; _snprintf(text, MAX_LOG_LENGTH, fmt, ##__VA_ARGS__); displayText(font, text, x, y); delete[] text; } while (0)

#define COLOR(cond) do { if (cond) { glColor3ub(255, 255, 255); } else { glColor3ub(255, 0, 0); } } while (0)

#define CAPACITY_NOISE_UPPER_BOUND 50
#define CAPACITY_NORMAL_UPPER_BOUND 9000

#define GRID_SIZE 22.5
#define GRID_DELTA 10.0

#define WINDOW_POS_X 200
#define WINDOW_POS_Y 200

#define ELLIPSE_EDGES 360

#define REGION_NUM_MAX 20


#define MAX_DELTA_BUFFER_SIZE 10
deque<Frame> Viewer::frames_buffer;
deque<Point> Viewer::points_buffer;
deque<Point> Viewer::ropoints_buffer;

deque<Point> Viewer::realpoints_buffer;

deque<int> Viewer::deltas_buffer;


deque<Point> Viewer::sums_buffer;
deque<Point> Viewer::stablepoints_buffer;

Frame *lastFrame = new Frame();


SOCKADDR_IN Viewer::servAddr;
SOCKET Viewer::sSocket;
SOCKET Viewer::sockClient;
int Viewer::nServAddLen;
char Viewer::buf[BUFFER_SIZE];
WSADATA Viewer::wsd;

deque<Mat> Viewer::review_buffer;

char Viewer::viewer_type = 'V';
char Viewer::frame_type = 'D';
int Viewer::frame_index = 0;
int Viewer::region_index = 0;
int Viewer::region_info_switch = 1;
int Viewer::last_frame_classify_result = 0;


InjectEventController Viewer::m_inject;
bool Viewer::last_dirty = false;
bool Viewer::has_start = false;

int Viewer::maxcap = 5000;
int Viewer::lastsum = 10000;
int Viewer::deltasum = 1000;
int Viewer::stablesum = 1000;
int Viewer::leavesum = 0;
int Viewer::dirtylow = 50000;


Mat Viewer::pattern, Viewer::lanc(560, 320, CV_32F), Viewer::Image[2];

Point Viewer::last_result(160, 180), Viewer::last_point[2], Viewer::start_point, Viewer::end_point;
int Viewer::pattern_sum;
Ptr<TrackerKCF> Viewer::tracker[2];


Point firstPoint = Point(0.0, 0.0);

//timeval last_frame_time;
//timeval current_frame_time;

#define KCF_REFRESH_INTERVAL 25
pthread_t draw_thread, update_threads[2];
const int MIN_X = 130, MAX_X = 750, MIN_Y = 200, MAX_Y = 1315, FINGER_Y = 305;
int last = 0, now = 1, frame_count = 0;
int tracker_last = 1, tracker_now = 0;
Rect2d box, box_last;
bool succ, succ_last;

bool isFlick = false;

void* Viewer::draw(void* nouse)
{
	while (true)
	{
		Mat drawing = Mat::zeros(Size(360, 640), CV_8UC3);
		for (int i = 0; i < Viewer::points_buffer.size(); i++)
			circle(drawing, Viewer::points_buffer[i] / 3, 0.5, Scalar(255, 0, 255), -1);
		imshow("result", drawing);

		Mat real = Mat::zeros(Size(360, 640), CV_8UC3);
		for (int i = 0; i < Viewer::realpoints_buffer.size(); i++)
			circle(real, Viewer::realpoints_buffer[i] / 3, 0.5, Scalar(0, 0, 255), -1);
		imshow("real", real);
		waitKey(5);

		//Mat sum = Mat::zeros(Size(1500, 800), CV_8UC3);
		//for (int i = 1; i < Viewer::sums_buffer.size(); i++)
		//	circle(sum, Point(Viewer::sums_buffer[i].x % 1500,800- Viewer::sums_buffer[i].y / 100), 1, Scalar(255, 255, 0), -1);
		//imshow("sum", sum);
		//waitKey(30);
	}
}

void* Viewer::update(void* nouse)
{
	succ = tracker[tracker_now]->update(Image[last], box);
	return NULL;
}

void* Viewer::update_last(void* nouse)
{
	succ_last = tracker[tracker_last]->update(Image[now], box_last);
	return NULL;
}

void Viewer::initUDP() {

	if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0)
	{
		printf("WSAStartup failed !/n");
	}

	sSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (sSocket == INVALID_SOCKET)
	{
		printf("socket() failed, Error Code:%d/n", WSAGetLastError());
		WSACleanup();
	}

	sockClient = socket(AF_INET, SOCK_DGRAM, 0);
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.S_un.S_addr = inet_addr("101.5.132.252");
	servAddr.sin_port = htons(4567);
	nServAddLen = sizeof(servAddr);

	ZeroMemory(buf, BUFFER_SIZE);
}

void Viewer::sent(int x, int y,string type) {
	string message = type + "|" + to_string(x) + "|" + to_string(y) + "|";
	//cout << message << endl << endl;
	strcpy_s(buf, message.c_str());
	if (sendto(sockClient, buf, BUFFER_SIZE, 0, (sockaddr *)&servAddr, nServAddLen) == SOCKET_ERROR)
	{
		closesocket(sSocket);
		WSACleanup();
	}
	//closesocket(sSocket);
	//WSACleanup();
}

void rotateAxis(Point first, Point now, float theta, Point& rotated) {
	double nowX2 = now.x - first.x;
	double nowY2 = first.y - now.y;
	float angle = (theta / 180.0*3.1415927);
	double roX2 = nowX2*cos(angle) + nowY2*sin(angle);
	double roY2 = nowY2*cos(angle) - nowX2*sin(angle);
	rotated.x = first.x + roX2;
	rotated.y = first.y - roY2;
}

void calcFirstPoint(RotatedRect& rect, Point& firstPoint) {
	Point pointB;
	Point2f vertices[4];
	rect.points(vertices);
	Point pointA;
	if (rect.size.width < rect.size.height) { //normal
		pointB.x = vertices[2].x;
		pointB.y = vertices[2].y;
		pointA.x = pointB.x - 100;
		pointA.y = pointB.y + 100;
	}
	else {
		pointB.x = vertices[3].x;
		pointB.y = vertices[3].y;
		pointA.x = pointB.x - 100;
		pointA.y = pointB.y - 100;
	}
	float theta = abs(rect.angle) / 180 * 3.1415927;
	double pointA2x = pointB.x - pointA.x;
	double pointA2y = pointA.y - pointB.y;
	double pointC2x = pointA2x*cos(theta) - pointA2y*sin(theta);
	double pointC2y = pointA2y*cos(theta) + pointA2x*sin(theta);
	//double pointCx = pointB.x - pointC2x;
	//double pointCy = pointB.y + pointC2y;
	firstPoint.x = pointB.x - pointC2x;
	firstPoint.y = pointB.y + pointC2y;
}

void restrict(Point& p, int lx, int rx, int ly, int ry)
{
	if (p.x < lx) p.x = lx;
	if (p.x > rx) p.x = rx;
	if (p.y < ly) p.y = ly;
	if (p.y > ry) p.y = ry;
}

void cubicSmooth5(deque<Point>& in)
{
	int N = in.size();
	Point out[5];
	if (N >= 5)
	{
		for (int i = N - 5; i <= N - 3; i++)
		{
			if (i == 0) out[5 - N + i] = (69.0 * in[0] + 4.0 * in[1] - 6.0 * in[2] + 4.0 * in[3] - in[4]) / 70.0;
			else if (i == 1) out[5 - N + i] = (2.0 * in[0] + 27.0 * in[1] + 12.0 * in[2] - 8.0 * in[3] + 2.0 * in[4]) / 35.0;
			else out[5 - N + i] = (-3.0 * (in[i - 2] + in[i + 2]) + 12.0 * (in[i - 1] + in[i + 1]) + 17.0 * in[i]) / 35.0;
		}
		out[3] = (2.0 * in[N - 5] - 8.0 * in[N - 4] + 12.0 * in[N - 3] + 27.0 * in[N - 2] + 2.0 * in[N - 1]) / 35.0;
		out[4] = (-in[N - 5] + 4.0 * in[N - 4] - 6.0 * in[N - 3] + 4.0 * in[N - 2] + 69.0 * in[N - 1]) / 70.0;
		for (int i = N - 5; i < N; i++)
			in[i] = out[5 - N + i];
	}
}

template <typename T>
int matSum(Mat& m)
{
	int sum = 0;
	T min = 255, max = 0;
	for (int i = 0; i < m.rows; i++)
		for (int j = 0; j < m.cols; j++)
		{
			T ele = m.at<T>(i, j);
			sum += ele;
			if (ele < min) min = ele;
			if (ele > max) max = ele;
		}
	return sum;
}

bool findPattern(Mat& m, Rect& pattern, RotatedRect& firstTouch)
{
	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;
	findContours(m, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

	bool rect = false;
	double tmp_max = 0;
	/*
	int index;
	for (int i = 0; i < contours.size(); i++)
	{
		if (contours[i].size() > 50)
		{
			rect = true;
			double area = contourArea(contours[i]);
			firstTouch = minAreaRect(contours[i]);
			if (area > tmp_max)
			{
				tmp_max = area;
				index = i;
			}
		}
	}
	if (rect)
	{
		pattern = boundingRect(contours[index]);
	}
	return rect;
	*/
	Rect rectsum;
	vector<Point> save;
	for (int i = 0; i < contours.size(); i++) {
		//cout << "***   " << i << contours[i].size() << endl << endl;
	if (contours[i].size() > 20 && contours[i].size()) {
			rect = true;
			Rect recti = boundingRect(contours.at(i));
			rectsum = rectsum | recti;
			save.insert(save.end(),contours.at(i).begin(), contours.at(i).end());
		}
	}
	if (rect) {
		pattern = rectsum;
		firstTouch = minAreaRect(save);
	}
	return rect;

}

Point matchPattern(Mat& m, Mat& p)
{
	Mat ImageResult;

	matchTemplate(m, p, ImageResult, CV_TM_SQDIFF);


	double minValue, maxValue;
	Point minPoint, maxPoint;
	minMaxLoc(ImageResult, &minValue, &maxValue, &minPoint, &maxPoint, Mat());
	return minPoint;
}

void trackerInit(Ptr<TrackerKCF>& t, Mat& m, Rect& r)
{
	TrackerKCF::Params para;
	para.desc_pca = TrackerKCF::GRAY;
	para.compressed_size = 1;
	t = TrackerKCF::create(para);
	t->init(m, r);
}

void performSwipe(Point& start_point, Point& end_point)
{
	cout << start_point << ' ' << end_point << endl;
	int dx = end_point.x - start_point.x, dy = end_point.y - start_point.y;
	cout << dx * dx + dy * dy << endl;
	if (dx * dx + dy * dy > 10000)
	{

		if (abs(dy) > abs(dx))
		{
			if (dy > 0) dy += 300;
			else dy -= 300;
			int fy = 1000 - dy / 2, ty = 1000 + dy / 2;
			if (fy < 0) fy = 0;
			if (fy > 1919) fy = 1919;
			if (ty < 0) ty = 0;
			if (ty > 1919) ty = 1919;
			Viewer::m_inject.swipe(500, fy, 500, ty);
		}
		else
		{
			if (dx > 0) 
			{
				Viewer::m_inject.touch_down(300, 500);
				Sleep(20);
				for (int i = 400; i < 1100; i += 200)
				{
					Viewer::m_inject.touch_move(i, 500);
					Sleep(20);
				}
				Viewer::m_inject.touch_up();
			}
			else
			{
				Viewer::m_inject.touch_down(1000, 500);
				Sleep(20);
				for (int i = 900; i > 200; i -= 200)
				{
					Viewer::m_inject.touch_move(i, 500);
					Sleep(20);
				}
				Viewer::m_inject.touch_up();
			}
			/*
			if (dx > 0 && dx < 300) { dx = 300; }
			if (dx < 0 && dx > -300) { dx = -300; }
			int fx = 500 - dx / 2, tx = 500 + dx / 2;
			if (fx < 0) fx = 0;
			if (fx > 1079) fx = 1079;
			if (tx < 0) tx = 0;
			if (tx > 1079) tx = 1079;
			Viewer::m_inject.swipe(fx, 1000, tx, 1000);
			*/
		}
	}
	else
	{
		Viewer::m_inject.touch_down(500, 1000);
		Sleep(5);
		Viewer::m_inject.touch_up();
		Sleep(100);
		Viewer::m_inject.touch_down(500, 1000);
		Sleep(5);
		Viewer::m_inject.touch_up();
	}
}

float Viewer::calculateColor(int value) {
	float color = 0;
	if (value <= CAPACITY_NOISE_UPPER_BOUND) {
		color = 0;
	}
	else if (value > CAPACITY_NORMAL_UPPER_BOUND) {
		color = 0;
	}
	else {
		color = (float)value / 500;
		// color = (float) (value + 100) / 800;
		if (color > 1) {
			color = 1;
		}
	}
	return color;
}

void Viewer::displayText(void *font, char *text, float x, float y) {
	glRasterPos2f(x, y);
	for (int i = 0; i < strlen(text); ++i) {
		glutBitmapCharacter(font, text[i]);
	}
}

void Viewer::displayCapacity(int value, int region, int isStable, float x, float y) {
	if (value <= CAPACITY_NOISE_UPPER_BOUND) {
		//value = 0;
		glColor3ub(255, 255, 255);
	}
	else if (value > CAPACITY_NORMAL_UPPER_BOUND) {
		value = 0;
		glColor3ub(255, 255, 255);
	}
	else if (region > 1) {
		if (isStable > 0) {
			glColor3ub(255, 0, 0);
		}
		else {
			glColor3ub(0, 127, 255);
		}
	}
	else if (region > 0) {
		glColor3ub(255, 132, 96);
	}
	else if (calculateColor(value) > 0.5) {
		glColor3ub(0, 0, 0);
	}
	else {
		glColor3ub(255, 255, 255);
	}
	TEXT(GLUT_BITMAP_TIMES_ROMAN_10, x, y, "%d", value);
}

void Viewer::displayFrame(Frame &frame, float dx, float dy) {
	glPolygonMode(GL_FRONT, GL_FILL);
	glPolygonMode(GL_BACK, GL_FILL);
	for (int y = 0; y < GRID_RES_Y; ++y) {
		for (int x = 0; x < GRID_RES_X; ++x) {
			float color = calculateColor(frame.capacity[y][x]);
			glColor3f(color, color, color);
			glRectf(dx + x, dy + GRID_RES_Y - y - 1, dx + x + 1, dy + GRID_RES_Y - y);
		}
	}
	if (region_info_switch > 0) {
		glLineWidth(1.5);
		glPolygonMode(GL_FRONT, GL_LINE);
		glPolygonMode(GL_BACK, GL_LINE);
		for (int i = 0; i < frame.regions.size(); ++i) {
			Region region = frame.regions[i];
			glLoadIdentity();
			glColor3ub(255, 0, 0);
			glTranslatef(dx + region.weightedCenterX, dy + GRID_RES_Y - region.weightedCenterY, 0.0);
			glRotatef(90.0 - region.theta / PI * 180.0, 0.0, 0.0, 1.0);
			glBegin(GL_POLYGON);
			for (int j = 0; j < ELLIPSE_EDGES; ++j) {
				glVertex3f(region.major * cos(2 * PI / ELLIPSE_EDGES * j), region.minor * sin(2 * PI / ELLIPSE_EDGES * j), 0.0);
			}
			glEnd();
			if ((!region.isNoiseRegion()) && (region.touchID >= 0)) {
				glLoadIdentity();
				glColor3ub(160, 255, 255);
				glRectf(floor(dx + region.weightedCenterX - 4.5), floor(dy + GRID_RES_Y - region.weightedCenterY - 4.5), floor(dx + region.weightedCenterX + 5.5), floor(dy + GRID_RES_Y - region.weightedCenterY + 5.5));
			}
		}
	}
	glPolygonMode(GL_FRONT, GL_FILL);
	glPolygonMode(GL_BACK, GL_FILL);
	glLoadIdentity();
	for (int y = 0; y < GRID_RES_Y; ++y) {
		for (int x = 0; x < GRID_RES_X; ++x) {
			int region = 0;
			int isStable = 0;
			if ((frame.region[y][x] > 0) && (frame.region[y][x] <= frame.regions.size())) {
				region = frame.regions[frame.region[y][x] - 1].region[y][x];
				isStable = frame.regions[frame.region[y][x] - 1].isStable;
			}
			displayCapacity(frame.capacity[y][x], region, isStable, dx + x + 0.1, dy + GRID_RES_Y - y - 1 + 0.2);
		}
	}
	glColor3ub(255, 132, 96);
	glBegin(GL_LINES);
	glVertex3f(dx, dy + GRID_RES_Y / 2, 0.0);
	glVertex3f(dx + GRID_RES_X, dy + GRID_RES_Y / 2, 0.0);
	glEnd();
	if (region_info_switch > 0) {
		for (int i = 0; i < frame.regions.size(); ++i) {
			Region region = frame.regions[i];
			if (region.touchID >= 0) {
				glColor3ub(255, 132, 96);
				glLoadIdentity();
				glTranslatef(dx + region.weightedCenterX, dy + GRID_RES_Y - region.weightedCenterY, 0.0);
				glBegin(GL_POLYGON);
				for (int j = 0; j < ELLIPSE_EDGES; ++j) {
					glVertex3f(0.32 * cos(2 * PI / ELLIPSE_EDGES * j), 0.32 * sin(2 * PI / ELLIPSE_EDGES * j), 0.0);
				}
				glEnd();
				glLoadIdentity();
				glColor3ub(0, 0, 0);
				TEXT(GLUT_BITMAP_TIMES_ROMAN_10, dx + region.weightedCenterX - 0.1, dy + GRID_RES_Y - region.weightedCenterY - 0.1, "%d", region.touchID + 1);
			}
		}
	}
}

void Viewer::gravityCenter(Mat src, Point &cent) {
	long xsum = 0;
	long ysum = 0;
	long count = 0;
	for (int i = 0;i<src.rows;i++) {
		for (int j = 0;j<src.cols;j++) {
			if (src.at<uchar>(i, j) != 0) {
				xsum += j;
				ysum += i;
				count++;
			}
		}
	}
	cent.x = cvRound(double(xsum) / double(count));
	cent.y = cvRound(double(ysum) / double(count));
}

bool judgeLeave(Frame *frame, Frame *lastframe) {
	int delta = 0;
	bool allneg = true;
	cout << "lastframe"<< &lastframe << endl;
	if (&lastframe) {
		for (int y = 0; y < GRID_RES_Y; ++y) {
			for (int x = 0; x < GRID_RES_X; ++x) {
				if (frame->capacity[y][x] > 200) {
					delta = frame->capacity[y][x] - lastframe->capacity[y][x];
					if (delta > 0) {
						allneg = false;
						break;
					}
				}
			}
		}
	}
	return (&lastframe)&&allneg;
}

bool Viewer::judgeDirty(int sum)
{
	bool isStable = (deltasum > 0) && (sum - lastsum < 0);
	int min_sum = 10000;

	bool isDirty = (sum > min_sum) && isStable;

	int newDelta = sum - lastsum;

	if (deltas_buffer.size() < MAX_DELTA_BUFFER_SIZE) {
		deltas_buffer.push_back(newDelta);
		leavesum = leavesum + newDelta;
	}
	else {
		leavesum = leavesum - deltas_buffer.front();
		deltas_buffer.pop_front();
		deltas_buffer.push_back(newDelta);
		leavesum = leavesum + newDelta;
	}

	if (last_dirty) {
		//change stablesum to dirtylow
		if ((sum < 10000) || ((leavesum < -8000) && (sum < dirtylow) && (sum - lastsum < -3000))) {
			isDirty = false;
		}
		else {
			isDirty = true;
		}
	}
	return isDirty;
}

void Viewer::sendPoint(bool touchEnd, Point result = Point())
{
	if (touchEnd)
	{
		//for points_buffer
		int N = points_buffer.size();
		if (N > 0 && N < 5)
		{
			Point p = points_buffer[0];
			p.x = (float)(p.x - MIN_X) / (MAX_X - MIN_X) * 1080;
			p.y = (float)(p.y - MIN_Y) / (MAX_Y - MIN_Y) * 1920;
			m_inject.touch_down(p.x, p.y);
			//sent(p.x, p.y, "down");
			realpoints_buffer.push_back(p);
		}

		m_inject.touch_up();

		points_buffer.clear();
		ropoints_buffer.clear();
		realpoints_buffer.clear();
		dirtylow = 50000;
	}
	else
	{
		points_buffer.push_back(result);

		//for points_buffer
		int N = points_buffer.size();
		if (N >= 5)
		{
			cubicSmooth5(points_buffer);

			if (N == 5)
			{
				Point p = points_buffer[0];
				p.x = (float)(p.x - MIN_X) / (MAX_X - MIN_X) * 1080;
				p.y = (float)(p.y - MIN_Y) / (MAX_Y - MIN_Y) * 1920;
				m_inject.touch_down(p.x, p.y);
				//sent(p.x, p.y, "down");
				realpoints_buffer.push_back(p);
				p = points_buffer[1];
				p.x = (float)(p.x - MIN_X) / (MAX_X - MIN_X) * 1080;
				p.y = (float)(p.y - MIN_Y) / (MAX_Y - MIN_Y) * 1920;
				m_inject.touch_move(p.x, p.y);
				//sent(p.x, p.y, "move");
			}
			Point p = points_buffer[N - 3];
			p.x = (float)(p.x - MIN_X) / (MAX_X - MIN_X) * 1080;
			p.y = (float)(p.y - MIN_Y) / (MAX_Y - MIN_Y) * 1920;
			m_inject.touch_move(p.x, p.y);
			//sent(p.x, p.y, "move");
			realpoints_buffer.push_back(p);

		}
	}
}

void Viewer::displayFrameCV(Frame &frame) {
	bool has_find_pattern;
	Rect patternRect;
	RotatedRect firstTouch;

	Mat input;
	input = cv::Mat(GRID_RES_Y, GRID_RES_X, CV_32S, frame.capacity);
	input.convertTo(input, CV_32F);

	int sum = matSum<float>(input);                    //计算该帧电容和作为判断帧可靠性的依据

	Point newsum(frame.frameID, sum);
	sums_buffer.push_back(newsum);
	if (sums_buffer.size() > 1500) {
		sums_buffer.clear();
	}
	if (stablepoints_buffer.size() > 6) {
		stablepoints_buffer.clear();
	}

	bool isDirty = judgeDirty(sum);                    //判断该帧是否足够可靠，以确定耳朵是否抬起

	if (isDirty)
	{
		if (sum < dirtylow) {
			dirtylow = sum;
		}

		Mat& binaryImage = Image[now];
		Mat& lastImage = Image[last];
		if (now)
			now = 0, last = 1;
		else now = 1, last = 0;

		Point result;
		resize(input, lanc, Size(), 20.0, 20.0, INTER_LANCZOS4);
		threshold(lanc, binaryImage, 200, 0, THRESH_TOZERO);        //gray
		binaryImage.convertTo(binaryImage, CV_8U);
		imshow("image", binaryImage);
		waitKey(5);

		if (!last_dirty)                                         //触摸开始
		{
			frame_count = 0;
			has_find_pattern = findPattern(binaryImage, patternRect, firstTouch);
			////////////////////////////////////for show the rectangle///////////////////////////////
			Point2f vertices[4];
			firstTouch.points(vertices);
			Mat show3 = binaryImage.clone();
			for (int i = 0; i < 4; i++) {
				line(show3, vertices[i], vertices[(i + 1) % 4], 255, 1);
			}
			imshow("tmp", show3);
			waitKey(5);
			///////////////////////////////////////////////////////////////////////////
			if (has_find_pattern)
			{
				Point want;
				calcFirstPoint(firstTouch, want);
				restrict(want, 0, 320, 0, 560);

				trackerInit(tracker[0], binaryImage, patternRect);
				trackerInit(tracker[1], binaryImage, patternRect);
				pattern = binaryImage(patternRect);

				Point real_result(want.x / 320.0 * 1080.0, want.y / 560.0 * 1920.0);

				//record the first point
				firstPoint.x = want.x;
				firstPoint.y = want.y;

				//circle(tmp, Point(patternRect.x + cp.x, patternRect.y + cp.y), 5, Scalar(255, 0, 255), -1);

				result = last_result = Point(want.x, want.y);
				result.x = result.x / 320.0 * 1080.0;
				result.y = result.y / 560.0 * 1920.0;

				if (result.x < MIN_X || result.x > MAX_X || result.y < MIN_Y || result.y > MAX_Y)
				{
					if (result.y > MAX_Y || result.y < MIN_Y)
						m_inject.vibrate();
					return;
				}
				last_point[0].x = last_point[1].x = patternRect.x;
				last_point[0].y = last_point[1].y = patternRect.y;

				stablepoints_buffer.push_back(newsum);
				stablesum = sum;

			}
			else return;
		}
		else                                            //触摸中
		{
			Point box_point, last_box_point, ptmp;
			pthread_create(&update_threads[0], NULL, update, NULL);
			pthread_create(&update_threads[1], NULL, update_last, NULL);
			pthread_join(update_threads[0], NULL);
			pthread_join(update_threads[1], NULL);
			if (succ)
			{
				if (!succ_last)
				{
					Rect rect;
					findPattern(binaryImage, rect, RotatedRect());
					trackerInit(tracker[tracker_last], binaryImage, rect);
					last_point[tracker_last] = Point(rect.x, rect.y);
					frame_count = 0;
				}

				if (frame_count == KCF_REFRESH_INTERVAL)
				{
					if (box.x >= 0 && box.y >= 0 && box.x + box.width <= binaryImage.cols && box.y + box.height <= binaryImage.rows)
					{
						cout << "refresh\n";
						Rect rect;
						findPattern(binaryImage, rect, RotatedRect());
						trackerInit(tracker[tracker_now], binaryImage, rect);

						last_box_point = last_point[tracker_last];
						box_point = Point(box_last.x, box_last.y);
						if (tracker_now)
							tracker_now = 0, tracker_last = 1;
						else tracker_now = 1, tracker_last = 0;

						frame_count = 0;
						last_point[tracker_now] = box_point, last_point[tracker_last] = Point(rect.x, rect.y);
					}
					else
					{
						last_box_point = last_point[tracker_now];
						box_point = Point(box.x, box.y);
						last_point[tracker_now] = box_point, last_point[tracker_last] = Point(box_last.x, box_last.y);
					}
				}
				else
				{
					frame_count++;
					last_box_point = last_point[tracker_now];
					box_point = Point(box.x, box.y);
					last_point[tracker_now] = box_point;
					if (succ_last) last_point[tracker_last] = Point(box_last.x, box_last.y);
				}

				ptmp = box_point - last_box_point + last_result;
				restrict(ptmp, 0, 320, 0, 560);
				result = ptmp;

				//result without rotated
				result.x = result.x / 320.0 * 1080.0;
				result.y = result.y / 560.0 * 1920.0;
				rectangle(binaryImage, box, Scalar(255, 0, 0), 2, 1);

			}
			else
			{
				cout << "failed\n";
				//判断卷起的情况

				Rect rect;
				if (!findPattern(binaryImage, rect, RotatedRect()))
				{
					if (now)
						now = 0, last = 1;
					else now = 1, last = 0;
					return;
				}

				frame_count = 0;
				if (succ_last)
				{
					cout << "switch\n";
					trackerInit(tracker[tracker_now], binaryImage, rect);
					last_box_point = last_point[tracker_last];
					box_point = Point(box_last.x, box_last.y);

					result = ptmp = box_point - last_box_point + last_result;
					if (tracker_now)
						tracker_now = 0, tracker_last = 1;
					else tracker_now = 1, tracker_last = 0;
					last_point[tracker_now] = box_point, last_point[tracker_last] = Point(rect.x, rect.y);
				}
				else
				{
					cout << "double\n";
					trackerInit(tracker[tracker_last], binaryImage, rect);
					trackerInit(tracker[tracker_now], binaryImage, rect);
					pattern = binaryImage(rect);
					box_point = Point(rect.x, rect.y);

					last_box_point = matchPattern(lastImage, pattern);

					result = ptmp = box_point - last_box_point + last_result;
					last_point[tracker_now] = last_point[tracker_last] = box_point;
				}

				restrict(result, 0, 320, 0, 560);

				//result without rotated
				result.x = result.x / 320.0 * 1080.0;
				result.y = result.y / 560.0 * 1920.0;

			}

			if (result.x < MIN_X || result.x > MAX_X || result.y < MIN_Y || result.y > MAX_Y)
			{
				if (result.y > MAX_Y || result.y < MIN_Y)
					m_inject.vibrate();
				return;
			}

			last_result = ptmp;
		}

		sendPoint(false, result);
	}
	else                                               //触摸结束
	{
		sendPoint(true);
		if (last_dirty) {
			//m_inject.touch_up();
			//sent(0, 0, "up");
			stablepoints_buffer.push_back(newsum);
		}
	}

	last_dirty = isDirty;
	deltasum = sum - lastsum;
	lastsum = sum;
}

void Viewer::display() {
	glClear(GL_COLOR_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);

	pthread_mutex_lock(&Picker::frames_mutex);
	if (Picker::frames.empty()) {
		pthread_mutex_unlock(&Picker::frames_mutex);
		glFlush();
		glutSwapBuffers();
		return;
	}
	Frame *frame_current = &Picker::frames.back();
	pthread_mutex_lock(&Picker::frames_mutex);

	Frame *frame_display = NULL;

	if (viewer_type == 'V') {
		if (frame_type == 'D') {
			frame_display = &Picker::frame_down;
		}
		else if (frame_type == 'S') {
			frame_display = &Picker::frame_stable;
		}
	}
	else if (viewer_type == 'R') {
		if ((!frames_buffer.empty()) && (frame_index < frames_buffer.size())) {
			frame_display = &frames_buffer[frame_index];
		}
	}

	if (frame_current) {
		displayFrame(*frame_current, 0.0, 0.0);

		displayFrameCV(*frame_current);
	}
	if (frame_display) {
		displayFrame(*frame_display, GRID_RES_X + GRID_DELTA, 0.0);
	}
	if (frame_display && viewer_type == 'R' ) {
		displayFrameCV(*frame_display);
	}

	if (viewer_type == 'V') {
		string last_frame_classify_result_string = "";
		switch (last_frame_classify_result) {
		case 1:
			last_frame_classify_result_string = "Finger";
			break;
		case 2:
			last_frame_classify_result_string = "Uncertain";
			break;
		case 3:
			last_frame_classify_result_string = "Face";
			break;
		case 4:
			last_frame_classify_result_string = "Ear";
			break;
		default:
			break;
		}
		COLOR(1);
		TEXT(GLUT_BITMAP_HELVETICA_18, 17.0, 27.0, "%c - %c  %s", viewer_type, frame_type, last_frame_classify_result_string.c_str());
	}
	else if (viewer_type == 'R') {
		COLOR(1);
		TEXT(GLUT_BITMAP_HELVETICA_18, 17.0, 27.0, "%c - %c  %d/%d", viewer_type, frame_type, frame_index + 1, (int)frames_buffer.size());
	}

	if (frame_display) {
		Region *region = NULL;
		if (region_index < frame_display->regions.size()) {
			region = &frame_display->regions[region_index];
		}
		COLOR(1);
		if (viewer_type == 'V') {
			TEXT(GLUT_BITMAP_HELVETICA_18, 43.0, 27.0, "Dirty %d, Down %d, Res %d", frame_current->isDirty, frame_current->hasDownFromDirty, frame_current->frameClassifyResult);
			if (frame_current->frameClassifyResult > 0) {
				last_frame_classify_result = frame_current->frameClassifyResult;
			}
		}
		else if (viewer_type == 'R') {
			TEXT(GLUT_BITMAP_HELVETICA_18, 43.0, 27.0, "Dirty %d, Down %d, Res %d", frame_display->isDirty, frame_display->hasDownFromDirty, frame_display->frameClassifyResult);
			last_frame_classify_result = frame_display->frameClassifyResult;
		}
		if (region) {
			long long delay = -1;
			if (region->downTimestamp > 0) {
				delay = region->currentTimestamp - region->downTimestamp;
			}
			COLOR(1);
			TEXT(GLUT_BITMAP_HELVETICA_18, 43.0, 26.0, "Stable %d, Delay %lld", region->isStable, delay);
			COLOR((region->faceClassifyResult != 1) && (region->earClassifyResult != 1));
			TEXT(GLUT_BITMAP_HELVETICA_18, 43.0, 25.0, "ID %d, Face %d, Ear %d", region->touchID + 1, region->faceClassifyResult, region->earClassifyResult);
			COLOR(1);
			TEXT(GLUT_BITMAP_HELVETICA_18, 43.0, 24.0, "Cumu = %.0lf", region->cumuStrength);
			TEXT(GLUT_BITMAP_HELVETICA_18, 43.0, 23.0, "Max = %d", region->maxCapacity);
			TEXT(GLUT_BITMAP_HELVETICA_18, 43.0, 22.0, "Avg = %.2lf", region->averageCapacity);
			TEXT(GLUT_BITMAP_HELVETICA_18, 43.0, 21.0, "Expand Size = %d", (int)region->capacity2.size());
			COLOR(region->regionSize <= 45);
			TEXT(GLUT_BITMAP_HELVETICA_18, 43.0, 20.0, "Size = %d", region->regionSize);
			COLOR(region->xSpan <= 8);
			TEXT(GLUT_BITMAP_HELVETICA_18, 43.0, 19.0, "xSpan = %d", region->xSpan);
			COLOR(region->ySpan <= 8);
			TEXT(GLUT_BITMAP_HELVETICA_18, 43.0, 18.0, "ySpan = %d", region->ySpan);
			COLOR(region->yxRatio <= 2.00);
			TEXT(GLUT_BITMAP_HELVETICA_18, 43.0, 17.0, "y/x = %.2lf", region->yxRatio);
			COLOR(region->xyRatio <= 2.00);
			TEXT(GLUT_BITMAP_HELVETICA_18, 43.0, 16.0, "x/y = %.2lf", region->xyRatio);
			COLOR(1);
			TEXT(GLUT_BITMAP_HELVETICA_18, 43.0, 14.0, "Center = (%.2lf, %.2lf)", region->weightedCenterX, region->weightedCenterY);
			COLOR(region->major <= 4.10);
			TEXT(GLUT_BITMAP_HELVETICA_18, 43.0, 13.0, "Major = %.2lf", region->major);
			COLOR(region->minor <= 2.90);
			TEXT(GLUT_BITMAP_HELVETICA_18, 43.0, 12.0, "Minor = %.2lf", region->minor);
			COLOR(region->ecc < 0.8900);
			TEXT(GLUT_BITMAP_HELVETICA_18, 43.0, 11.0, "Ecc = %.4lf", region->ecc);
			COLOR(1);
			TEXT(GLUT_BITMAP_HELVETICA_18, 43.0, 10.0, "Theta = %.2lf", region->theta / PI * 180);
			COLOR(region->averageShadowLength < 2.00);
			TEXT(GLUT_BITMAP_HELVETICA_18, 43.0, 8.0, "AvgShadLen = %.2lf", region->averageShadowLength);
			COLOR(region->upperShadowSize <= 2);
			TEXT(GLUT_BITMAP_HELVETICA_18, 43.0, 7.0, "UpShadSize = %d", region->upperShadowSize);
		}
	}

	glFlush();
	glutSwapBuffers();
}

void Viewer::special(int key, int x, int y) {
	if (viewer_type == 'R') {
		switch (key) {
		case GLUT_KEY_UP:
			frame_index -= 10;
			break;
		case GLUT_KEY_DOWN:
			frame_index += 10;
			break;
		case GLUT_KEY_LEFT:
			frame_index -= 1;
			break;
		case GLUT_KEY_RIGHT:
			frame_index += 1;
			break;
		default:
			break;
		}
		if (frame_index > (int)frames_buffer.size() - 1) {
			frame_index = (int)frames_buffer.size() - 1;
		}
		if (frame_index < 0) {
			frame_index = 0;
		}
	}
}

void Viewer::keyboard(unsigned char key, int x, int y) {
	switch (key) {
	case 'V':
	case 'v':
		viewer_type = 'V';
		break;
	case 'R':
	case 'r':
		viewer_type = 'R';
		frame_index = (int)Picker::frames.size() - 1;
		if (frame_index < 0) {
			frame_index = 0;
		}
		region_index = 0;
		pthread_mutex_lock(&Picker::frames_mutex);
		frames_buffer.clear();
		frames_buffer = Picker::frames;
		pthread_mutex_unlock(&Picker::frames_mutex);
		break;
	case 'D':
	case 'd':
		frame_type = 'D';
		break;
	case 'S':
	case 's':
		if (viewer_type == 'V') {
			frame_type = 'S';
		}
		else if (viewer_type == 'R') {
			time_t tt = time(NULL);
			tm *t = localtime(&tt);
			char filename[MAX_LOG_LENGTH + 1];
			_snprintf(filename, MAX_LOG_LENGTH, "record_%d_%02d_%02d_%02d_%02d_%02d.txt", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
			ofstream fout;
			fout.open(filename);
			for (int i = 0; i < frames_buffer.size(); ++i) {
				frames_buffer[i].save2(fout);
			}
			fout.close();
		}
		break;
	case 'L':
	case 'l':
		if (viewer_type == 'R') {
			ifstream fin;
			fin.open("record.txt");
			if (!fin) {
				break;
			}
			frames_buffer.clear();
			while (true) {
				Frame *frame = new Frame();
				if (frame->load(fin)) {
					frames_buffer.push_back(*frame);
				}
				else {
					delete frame;
					break;
				}
			}
			fin.close();
			frame_index = (int)frames_buffer.size() - 1;
			if (frame_index < 0) {
				frame_index = 0;
			}
		}
		break;
	case 'I':
	case 'i':
		region_info_switch = 1 - region_info_switch;
	case '_':
	case '-':
		region_index -= 1;
		if (region_index < 0) {
			region_index = 0;
		}
		break;
	case '+':
	case '=':
		region_index += 1;
		if (region_index > REGION_NUM_MAX) {
			region_index = REGION_NUM_MAX;
		}
		break;
	case 27:
		exit(0);
		break;
	default:
		break;
	}
}

void Viewer::run(int argc, char **argv) {
	pthread_create(&draw_thread, NULL, draw, NULL);
	int lastID = -1;
	while (true)
	{
		pthread_mutex_lock(&Picker::frames_mutex);
		if (Picker::frames.empty()) {
			pthread_mutex_unlock(&Picker::frames_mutex);
			Sleep(100);
			continue;
		}
		Frame *frame_current = &Picker::frames.back();
		pthread_mutex_unlock(&Picker::frames_mutex);

		if (frame_current->frameID == lastID) {
			lastID = frame_current->frameID;
			continue;
		}

		if (frame_current) {
			//cout << "*" << frame_current->frameID << endl;
			//cout << (float)clock() / CLOCKS_PER_SEC << endl;
			displayFrameCV(*frame_current);
			//cout << (float)clock() / CLOCKS_PER_SEC << endl<< endl;
		}
		lastID = frame_current->frameID;
	}
	pthread_join(draw_thread, NULL);

	/*
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB|GLUT_SINGLE);
	glutInitWindowPosition(WINDOW_POS_X, WINDOW_POS_Y);
	glutInitWindowSize((GRID_RES_X * 2 + GRID_DELTA * 2) * GRID_SIZE, GRID_RES_Y * GRID_SIZE);
	glutCreateWindow("HW");
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glMatrixMode(GL_PROJECTION);
	glOrtho(0.0, GRID_RES_X * 2 + GRID_DELTA * 2, 0.0, GRID_RES_Y, 0.0, 1000.0);
	glMatrixMode(GL_MODELVIEW);
	gluLookAt(0.0, 0.0, 1000.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
	glutIdleFunc(display);
	glutDisplayFunc(display);
	glutSpecialFunc(special);
	glutKeyboardFunc(keyboard);
	glutMainLoop();

	cvWaitKey(0);
	*/

}
