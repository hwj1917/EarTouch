#include "picker.h"
#include "viewer.h"
#include <windows.h>
#include <process.h>
#include <fstream>

using namespace std;
using namespace cv;

#define REAL_TIME

#ifdef REAL_TIME

int _argc;
char** _argv;

void* picker_getlog(void* n) {
	Picker::getLog();
	return NULL;
}

void* run(void* n)
{
	Viewer::run(_argc, _argv);
	return NULL;
}

void viewer_udpinit() {
	Viewer::initUDP();
}

void* runTCP(void* nouse)
{
	Picker::getSensorData();
	return NULL;
}

int main(int argc, char **argv) {
	//UDP
	//viewer_udpinit();

	_argc = argc, _argv = argv;

	pthread_t threads[3];
	pthread_mutex_init(&Picker::frames_mutex, NULL);
	pthread_mutex_init(&Picker::tcp_mutex, NULL);
	pthread_create(&threads[0], NULL, picker_getlog, NULL);
	pthread_create(&threads[1], NULL, run, NULL);
	pthread_create(&threads[2], NULL, runTCP, NULL);

	pthread_join(threads[0], NULL);
	pthread_join(threads[1], NULL);
	pthread_join(threads[2], NULL);
	pthread_mutex_destroy(&Picker::frames_mutex);
	pthread_mutex_destroy(&Picker::tcp_mutex);
    return 0;

}

#else


deque<Frame> file_buffer;
int index = 0;
Frame frameCV;
bool review_switch = false;

void loadFile(String add) {
	ifstream fin;
	fin.open(add);
	file_buffer.clear();
	while (true) {
		Frame *frame = new Frame();
		if (frame->load2(fin)) {
			file_buffer.push_back(*frame);
		}
		else {
			delete frame;
			break;
		}
	}
	fin.close();
}

void onMouse(int event, int x, int y, int flags, void *ustc) {
	if (event == CV_EVENT_LBUTTONUP) {
		review_switch = true;
	}
	if (event == CV_EVENT_RBUTTONUP) {
		review_switch = false;
	}
}


int main() {

	//ear.txt 1730
	//
	loadFile("D:\\Projects\\EarTouch\\HWViewer\\HWViewer\\data\\data_face.txt");
	index = 0;
	/*Mat Image(560, 320,CV_8UC1);
	imshow("tracking", Image);
	waitKey(10);*/
	Viewer::displayFrameCV(file_buffer.at(index));
	setMouseCallback("tracking", onMouse);

	pthread_t thread;
	pthread_create(&thread, NULL, Viewer::draw, NULL);

	while (true) {
		if (true) {
			/*Mat input;
			input = cv::Mat(GRID_RES_Y, GRID_RES_X, CV_32S, file_buffer.at(index).capacity);
			input.convertTo(input, CV_32F);


			Mat lanc(560, 320, CV_32F), tmp, binaryImage;
			Point result;
			resize(input, lanc, Size(), 20.0, 20.0, INTER_LANCZOS4);
			lanc.convertTo(tmp, CV_8U);
			threshold(tmp, binaryImage, 200, 0, THRESH_TOZERO);
			imshow("tracking2", binaryImage);*/
			//cout << "*" << index << endl;
			
			Viewer::displayFrameCV(file_buffer.at(index));
			index = index + 1;
		}
		if (waitKey(100) == 27 || index == file_buffer.size()) {
			break;
		}


		/*Mat drawing = Mat::zeros(Size(360, 640), CV_8UC3);
		for (int i = 0; i < Viewer::points_buffer.size(); i++)
			circle(drawing, Viewer::points_buffer[i] / 3, 0.5, Scalar(255, 0, 255), -1);
		imshow("result", drawing);*/

		/*Mat real = Mat::zeros(Size(360, 640), CV_8UC3);
		for (int i = 0; i < Viewer::realpoints_buffer.size(); i++)
			circle(real, Viewer::realpoints_buffer[i] / 3, 0.5, Scalar(0, 0, 255), -1);
		imshow("real", real);*/

		//Mat sum = Mat::zeros(Size(1500, 800), CV_8UC3);
		//for (int i = 0; i < Viewer::sums_buffer.size(); i++)
		//	circle(sum, Point(Viewer::sums_buffer[i].x % 1500, 800 - Viewer::sums_buffer[i].y / 100), 0.5, Scalar(255, 255, 0), -1);
		//for (int i = 0; i < Viewer::stablepoints_buffer.size(); i++)
		//	circle(sum, Point(Viewer::stablepoints_buffer[i].x % 1500, 800 - Viewer::stablepoints_buffer[i].y / 100), 5, Scalar(255, 0, 255), -1);
		//imshow("sum", sum);


	}

	pthread_join(thread, NULL);
}

#endif