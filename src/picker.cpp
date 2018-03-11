#include "picker.h"
#include <stdio.h>

#pragma comment(lib,"WS2_32.lib")  

#define ADB_SHELL_SETENFORCE "adb shell setenforce 0"
#define ADB_LOGCAT_LOG "adb logcat -s aptouch_daemon"

pthread_mutex_t Picker::frames_mutex;
deque<Frame> Picker::frames;
vector<long long> Picker::delays;

Frame Picker::frame_down;
Frame Picker::frame_stable;
SOCKET Picker::tcpClient;

void Picker::getLog() {

    system(ADB_SHELL_SETENFORCE);
    
    frames.clear();
    delays.clear();
    int frameID = 0;
    Frame *frame = NULL;
    Region *region = NULL;
    
    int previousHasDownFromDirty = 0;
    int currentHasDownFromDirty = 0;
    int currentHasStableFromDirty = 0;
    
    char buffer[MAX_LOG_LENGTH + 1];
    FILE *pf = _popen(ADB_LOGCAT_LOG, "r");

	volatile int last_time = time(NULL);
    while (fgets(buffer, MAX_LOG_LENGTH, pf) != NULL) {
		volatile int tmp = time(NULL);
		if (tmp - last_time > 1)
		{
			cout << "restart\n";
			last_time = time(NULL);

			frameID = 0;
			frame = NULL;
			region = NULL;

			previousHasDownFromDirty = 0;
			currentHasDownFromDirty = 0;
			currentHasStableFromDirty = 0;
			system(ADB_SHELL_SETENFORCE);
			continue;
		}
        stringstream ss;
        string str;
        ss << buffer;
        ss >> str >> str >> str >> str >> str >> str >> str >> str >> str >> str >> str;
        if (str.substr(0, 22) != "debug--tsinghua--xwj--") {
            continue;
        }
        if (str == "debug--tsinghua--xwj--begin") {
            region = new Region();
            ss >> region->currentRegionID >> region->previousRegionID >> region->nextRegionID >> region->touchID >> region->isStable;
            ss >> region->faceClassifyResult >> region->earClassifyResult;
            if (region->previousRegionID > 0) {
				pthread_mutex_lock(&Picker::frames_mutex);
                for (int i = (int) frames.size() - 1; i >= 0; --i) {
                    int findPrevious = 0;
                    for (int j = 0; j < frames[i].regions.size(); ++j) {
                        if (frames[i].regions[j].currentRegionID == region->previousRegionID) {
                            frames[i].regions[j].nextRegionID = region->currentRegionID;
                            region->previousRegionFrameDelta = (int) frames.size() - i - 1;
                            region->previousRegionIndex = j;
                            findPrevious = 1;
                            break;
                        }
                    }
                    if (findPrevious > 0) {
                        break;
                    }
                }
				pthread_mutex_unlock(&Picker::frames_mutex);
            }
        }
        else if (str == "debug--tsinghua--xwj--end") {
            continue;
        }
        else if (str == "debug--tsinghua--xwj--status") {
			last_time = time(NULL);
            if (!frame) {
                continue;
            }
            ss >> frame->timestamp >> frame->isDirty >> frame->hasDownFromDirty >> frame->frameClassifyResult;
            previousHasDownFromDirty = currentHasDownFromDirty;
            currentHasDownFromDirty = frame->hasDownFromDirty;
            if ((previousHasDownFromDirty == 0) && (currentHasDownFromDirty > 0)) {
                frame_down = *frame;
                currentHasStableFromDirty = 0;
            }
            else if (currentHasDownFromDirty == 0) {
                currentHasStableFromDirty = 0;
            }
            if ((currentHasDownFromDirty > 0) && (currentHasStableFromDirty == 0)) {
                for (int i = 0; i < frame->regions.size(); ++i) {
                    Region region = frame->regions[i];
                    if ((region.touchID >= 0) && (region.isStable > 0)) {
                        frame_stable= *frame;
                        currentHasStableFromDirty = 1;
                        break;
                    }
                }
            }
			pthread_mutex_lock(&Picker::frames_mutex);
            frames.push_back(*frame);
            if (frames.size() > MAX_FRAME_BUFFER_SIZE) {
                frames.pop_front();
            }
			pthread_mutex_unlock(&Picker::frames_mutex);
        }
        else if (str == "debug--tsinghua--xwj--capa1") {
			//cout << (float)clock() / CLOCKS_PER_SEC << endl;
            frame = new Frame();
            frameID += 1;
            frame->frameID = frameID;
            for (int y = 0; y < GRID_RES_Y / 4; ++y) {
                for (int x = 0; x < GRID_RES_X; ++x) {
                    ss >> frame->capacity[y][x];
                }
            }
        }
        else if (str == "debug--tsinghua--xwj--capa2") {
            if (!frame) {
                continue;
            }
            for (int y = GRID_RES_Y / 4; y < GRID_RES_Y / 4 * 2; ++y) {
                for (int x = 0; x < GRID_RES_X; ++x) {
                    ss >> frame->capacity[y][x];
                }
            }
        }
        else if (str == "debug--tsinghua--xwj--capa3") {
            if (!frame) {
                continue;
            }
            for (int y = GRID_RES_Y / 4 * 2; y < GRID_RES_Y / 4 * 3; ++y) {
                for (int x = 0; x < GRID_RES_X; ++x) {
                    ss >> frame->capacity[y][x];
                }
            }
        }
        else if (str == "debug--tsinghua--xwj--capa4") {
            if (!frame) {
                continue;
            }
            for (int y = GRID_RES_Y / 4 * 3; y < GRID_RES_Y; ++y) {
                for (int x = 0; x < GRID_RES_X; ++x) {
                    ss >> frame->capacity[y][x];
                }
            }
        }
        else if (str == "debug--tsinghua--xwj--region") {
            if ((!frame) || (!region)) {
                continue;
            }
            int x, y;
            region->cumuStrength = 0;
            region->maxCapacity = 0;
            while (ss >> y >> x) {
                region->x.push_back(x);
                region->y.push_back(y);
                region->capacity.push_back(frame->capacity[y][x]);
                region->cumuStrength += frame->capacity[y][x];
                region->region[y][x] += 1;
                frame->region[y][x] = (int) frame->regions.size() + 1;
                if (frame->capacity[y][x] > region->maxCapacity) {
                    region->maxCapacity = frame->capacity[y][x];
                }
            }
        }
        else if (str == "debug--tsinghua--xwj--region2") {
            if ((!frame) || (!region)) {
                continue;
            }
            int x, y;
            while (ss >> y >> x) {
                region->x2.push_back(x);
                region->y2.push_back(y);
                region->capacity2.push_back(frame->capacity[y][x]);
                region->region[y][x] += 1;
                if (frame->region[y][x] <= 0) {
                    frame->region[y][x] = (int) frame->regions.size() + 1;
                }
            }
        }
        else if (str == "debug--tsinghua--xwj--delay") {
            long long delay;
            ss >> delay;
            delays.push_back(delay);
            long long max_delay = delay;
            long long min_delay = delay;
            long long sum_delay = 0;
            for (int i = 0; i < delays.size(); ++i) {
                if (delays[i] > max_delay) {
                    max_delay = delays[i];
                }
                if (delays[i] < min_delay) {
                    min_delay = delays[i];
                }
                sum_delay += delays[i];
            }
            double average_delay = (double) sum_delay / delays.size();
            cout << "N = " << delays.size() << ", Last = " << delays.back() << ", Min = " << min_delay << ", Max = " << max_delay << ", Average = " << average_delay << endl;
        }
        else if (str == "debug--tsinghua--xwj--timecost") {
            long long timecost1, timecost2, timecost3;
            ss >> timecost1 >> timecost2 >> timecost3;
            if (timecost3 > 300) {
                printf("T1 = %.2lf ms, T2 = %.2lf ms, T = %.2lf ms\n", (double) timecost1 / 1000, (double) timecost2 / 1000, (double) timecost3 / 1000);
            }
        }
        else if (str == "debug--tsinghua--xwj--lifetime") {
            if ((!frame) || (!region)) {
                continue;
            }
            ss >> region->firstTimestamp >> region->downTimestamp >> region->currentTimestamp;
        }
        else if (str == "debug--tsinghua--xwj--features") {
            if ((!frame) || (!region)) {
                continue;
            }
            ss >> region->regionSize >> region->xSpan >> region->ySpan >> region->weightedCenterX >> region->weightedCenterY >> region->major >> region->minor >> region->ecc >> region->theta;
            ss >> region->averageShadowLength >> region->upperShadowSize;
            region->yxRatio = (double) region->ySpan / region->xSpan;
            region->xyRatio = (double) region->xSpan / region->ySpan;
            region->averageCapacity = region->cumuStrength / region->regionSize;
            frame->regions.push_back(*region);
        }
        else {
            cout << str;
            while (ss >> str) {
                cout << ' ' << str;
            }
            cout << endl;
        }
    }
    _pclose(pf);
}

void Picker::initTCP()
{
	//初始化WSA
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(sockVersion, &wsaData) != 0)
	{
		printf("start failed!\n");
		return;
	}

	//创建套接字
	SOCKET slisten = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (slisten == INVALID_SOCKET)
	{
		printf("socket error !\n");
		return;
	}

	//绑定IP和端口
	sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(8086);
	sin.sin_addr.S_un.S_addr = INADDR_ANY;
	::bind(slisten, (LPSOCKADDR)&sin, sizeof(sin));

	//开始监听
	if (listen(slisten, 5) == SOCKET_ERROR)
	{
		printf("listen error !\n");
		return;
	}

	sockaddr_in remoteAddr;
	int nAddrlen = sizeof(remoteAddr);

	tcpClient = accept(slisten, (SOCKADDR *)&remoteAddr, &nAddrlen);
	if (tcpClient == INVALID_SOCKET)
	{
		printf("accept error !\n");
		return;
	}
	printf("接受到一个连接\n");
}

void Picker::getLogFromTCP()
{
	char recvData[32 * 18 * 2];
	int len = 32 * 18 * 2;
	int ret = 0;
	int framID = 0;
	while (true)
	{
		int recvNum = recv(tcpClient, recvData + ret, len, 0);
		ret += recvNum;
		len -= recvNum;
		if (!len)
		{
			Frame f;
			f.loadFromTCP(recvData);
			f.frameID = framID++;
			pthread_mutex_lock(&Picker::frames_mutex);
			frames.push_back(f);
			if (frames.size() > MAX_FRAME_BUFFER_SIZE) {
				frames.pop_front();
			}
			pthread_mutex_unlock(&Picker::frames_mutex);

			len = 32 * 18 * 2, ret = 0;
		}
	}
}