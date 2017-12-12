#ifndef INJECTEVENTCONTROLLER_H
#define INJECTEVENTCONTROLLER_H
#include <ctime>
#include <string>

class InjectEventController
{
public:
	InjectEventController();
	void touch_down(int x, int y, int pressure = 50);
	void touch_move(int x, int y, int pressure = 50);
	void touch_up();
	void touch_double_click(int x, int y, int pressure = 50);
	void swipe(int start_x, int start_y, int end_x, int end_y);
	void vibrate();

private:
	int last_vibrate;

	std::string ADB = "adb shell mysend ";
	std::string DEVICE = "/dev/input/event4 ";
};

#endif // 
