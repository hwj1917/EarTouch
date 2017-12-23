#include "InjectEventController.h"
#include <windows.h>
#include <WinDef.h>
#include <atlstr.h>
using namespace std;

InjectEventController::InjectEventController()
{
	last_vibrate = time(NULL);
}

void InjectEventController::touch_down(int x, int y, int pressure)
{
	WinExec((ADB + "0 " + DEVICE + to_string(x) + " " + to_string(y)).c_str(), SW_NORMAL);
}

void InjectEventController::touch_move(int x, int y, int pressure)
{
	WinExec((ADB + "1 " + DEVICE + to_string(x) + " " + to_string(y)).c_str(), SW_NORMAL);
}

void InjectEventController::touch_up()
{
	WinExec((ADB + "2 " + DEVICE).c_str(), SW_NORMAL);
}

void InjectEventController::touch_double_click(int x, int y, int pressure)
{
	touch_down(x, y, pressure);
	Sleep(5);
	touch_up();
	Sleep(100);
	touch_down(x, y, pressure);
	Sleep(5);
	touch_up();
}

void InjectEventController::swipe(int start_x, int start_y, int end_x, int end_y)
{
	string cmd = "adb shell input swipe " + to_string(start_x) + " " + to_string(start_y) + " " + to_string(end_x) + " " + to_string(end_y);
	WinExec(cmd.c_str(), SW_NORMAL);
}

void InjectEventController::vibrate()
{
	if (time(NULL) - last_vibrate > 2)
	{
		WinExec("adb shell am startservice -n com.hwj.foreartouch/com.hwj.foreartouch.MyService", SW_NORMAL);
		last_vibrate = time(NULL);
	}
}