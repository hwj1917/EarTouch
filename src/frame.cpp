#include "frame.h"

Frame::Frame() {
    regions.clear();
    for (int y = 0; y < GRID_RES_Y; ++y) {
        for (int x = 0; x < GRID_RES_X; ++x) {
            capacity[y][x] = 0;
            region[y][x] = 0;
        }
    }
    frameID = -1;
    timestamp = 0;
    isDirty = 0;
    hasDownFromDirty = 0;
    frameClassifyResult = 0;
}

Frame::Frame(const Frame &frame) {
    regions.clear();
    for (int i = 0; i < frame.regions.size(); ++i) {
        regions.push_back(frame.regions[i]);
    }
    for (int y = 0; y < GRID_RES_Y; ++y) {
        for (int x = 0; x < GRID_RES_X; ++x) {
            capacity[y][x] = frame.capacity[y][x];
            region[y][x] = frame.region[y][x];
        }
    }
    frameID = frame.frameID;
    timestamp = frame.timestamp;
    isDirty = frame.isDirty;
    hasDownFromDirty = frame.hasDownFromDirty;
    frameClassifyResult = frame.frameClassifyResult;
}

void Frame::operator=(const Frame &frame) {
    regions.clear();
    for (int i = 0; i < frame.regions.size(); ++i) {
        regions.push_back(frame.regions[i]);
    }
    for (int y = 0; y < GRID_RES_Y; ++y) {
        for (int x = 0; x < GRID_RES_X; ++x) {
            capacity[y][x] = frame.capacity[y][x];
            region[y][x] = frame.region[y][x];
        }
    }
    frameID = frame.frameID;
    timestamp = frame.timestamp;
    isDirty = frame.isDirty;
    hasDownFromDirty = frame.hasDownFromDirty;
    frameClassifyResult = frame.frameClassifyResult;
}

Frame::~Frame() {
    regions.clear();
}

int Frame::load(ifstream &fin) {
    int regionNum = 0;
    if (!(fin >> timestamp)) {
        return 0;
    }
    fin >> frameID >> isDirty >> hasDownFromDirty >> frameClassifyResult >> regionNum;
    for (int y = 0; y < GRID_RES_Y; ++y) {
        for (int x = 0; x < GRID_RES_X; ++x) {
            fin >> capacity[y][x];
        }
    }
    for (int y = 0; y < GRID_RES_Y; ++y) {
        for (int x = 0; x < GRID_RES_X; ++x) {
            fin >> region[y][x];
        }
    }
    for (int i = 0; i < regionNum; ++i) {
        Region *region = new Region();
        region->load(fin);
        regions.push_back(*region);
    }
    return 1;
}

int Frame::load2(ifstream &fin) {
	if (!(fin >> timestamp)) {
		return 0;
	}
	fin >> frameID;
	for (int y = 0; y < GRID_RES_Y; ++y) {
		for (int x = 0; x < GRID_RES_X; ++x) {
			if (abs(capacity[y][x])<50) {
				capacity[y][x] = 0;
			}
			fin >> capacity[y][x];
		}
	}
	return 1;
}

void Frame::save(ofstream &fout) {
    fout << timestamp << ' ' << frameID << ' ' << isDirty << ' ' << hasDownFromDirty << ' ' << frameClassifyResult << ' ' << regions.size() << endl << endl;
    for (int y = 0; y < GRID_RES_Y; ++y) {
        for (int x = 0; x < GRID_RES_X; ++x) {
            fout << capacity[y][x] << ' ';
        }
        fout << endl;
    }
    fout << endl;
    for (int y = 0; y < GRID_RES_Y; ++y) {
        for (int x = 0; x < GRID_RES_X; ++x) {
            fout << region[y][x] << ' ';
        }
        fout << endl;
    }
    fout << endl;
    for (int i = 0; i < regions.size(); ++i) {
        regions[i].save(fout);
    }
    fout << endl;
}

void Frame::save2(ofstream &fout) {
	fout << timestamp<< ' ' << frameID << endl<<endl;
	for (int y = 0; y < GRID_RES_Y; ++y) {
		for (int x = 0; x < GRID_RES_X; ++x) {
			fout << capacity[y][x] << ' ';
		}
		fout << endl;
	}
	fout << endl;
}

void Frame::loadFromTCP(char* data)
{
	int index = 0;
	for (int y = 0; y < GRID_RES_Y; y++) {
		for (int x = 0; x < GRID_RES_X; x++) {
			short addr = data[index + 1] & 0xFF;
			addr |= ((data[index] << 8) & 0xFF00);
			capacity[y][x] = addr;
			index += 2;
		}
	}
}
