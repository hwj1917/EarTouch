//
//  region.h
//  HWViewer
//
//  Created by 徐炜杰 on 2017/5/10.
//  Copyright © 2017年 徐炜杰. All rights reserved.
//

#ifndef region_h
#define region_h

#include "includes.h"

class Region {
public:
    vector<int> x;
    vector<int> y;
    vector<int> capacity;
    
    vector<int> x2;
    vector<int> y2;
    vector<int> capacity2;
    
    int region[GRID_RES_Y][GRID_RES_X];
    
    int touchID;
    int currentRegionID;
    int previousRegionID;
    int nextRegionID;
    
    int previousRegionFrameDelta;
    int previousRegionIndex;
    
    long long firstTimestamp;
    long long downTimestamp;
    long long currentTimestamp;
    
    int xSpan;
    int ySpan;
    int regionSize;
    int maxCapacity;
    double cumuStrength;
    double averageCapacity;
    double yxRatio;
    double xyRatio;
    
    double weightedCenterX;
    double weightedCenterY;
    double major;
    double minor;
    double ecc;
    double theta;
    
    double averageShadowLength;
    int upperShadowSize;
    
    int isStable;
    int earClassifyResult;
    int faceClassifyResult;
    
    Region();
    Region(const Region &region);
    void operator=(const Region &region);
    ~Region();
    
    int isNoiseRegion();
    
    void load(ifstream &fin);
    void save(ofstream &fout);
};

#endif /* region_h */
