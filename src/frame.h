#ifndef frame_h
#define frame_h

#include "region.h"

class Frame {
public:
    vector<Region> regions;
    
    int capacity[GRID_RES_Y][GRID_RES_X];
    int region[GRID_RES_Y][GRID_RES_X];
    
    int frameID;
    long long timestamp;
    
    int isDirty;
    int hasDownFromDirty;
    int frameClassifyResult;
    
    Frame();
    Frame(const Frame &frame);
    void operator=(const Frame &frame);
    ~Frame();
    
    int load(ifstream &fin);
	int load2(ifstream &fin);
    void save(ofstream &fout);
	void save2(ofstream &fout);
};

#endif /* frame_h */
