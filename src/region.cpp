//
//  region.cpp
//  HWViewer
//
//  Created by 徐炜杰 on 2017/5/16.
//  Copyright © 2017年 徐炜杰. All rights reserved.
//

#include "region.h"

int Region::isNoiseRegion() {
    return (regionSize <= 3) && (maxCapacity < 100);
}

Region::Region() {
    x.clear();
    y.clear();
    capacity.clear();
    
    x2.clear();
    y2.clear();
    capacity2.clear();
    
    for (int y = 0; y < GRID_RES_Y; ++y) {
        for (int x = 0; x < GRID_RES_X; ++x) {
            region[y][x] = 0;
        }
    }
    
    touchID = -1;
    currentRegionID = -1;
    previousRegionID = -1;
    nextRegionID = -1;
    
    previousRegionFrameDelta = -1;
    previousRegionIndex = -1;
    
    firstTimestamp = 0;
    downTimestamp = 0;
    currentTimestamp = 0;
    
    xSpan = 0;
    ySpan = 0;
    regionSize = 0;
    maxCapacity = 0;
    cumuStrength = 0;
    averageCapacity = 0;
    yxRatio = 0;
    xyRatio = 0;
    
    weightedCenterX = 0;
    weightedCenterY = 0;
    major = 0;
    minor = 0;
    ecc = 0;
    theta = 0;
    
    averageShadowLength = 0;
    upperShadowSize = 0;
    
    isStable = 0;
    earClassifyResult = 0;
    faceClassifyResult = 0;
}

Region::Region(const Region &region) {
    x = region.x;
    y = region.y;
    capacity = region.capacity;
    
    x2 = region.x2;
    y2 = region.y2;
    capacity2 = region.capacity2;
    
    for (int y = 0; y < GRID_RES_Y; ++y) {
        for (int x = 0; x < GRID_RES_X; ++x) {
            this->region[y][x] = region.region[y][x];
        }
    }
    
    touchID = region.touchID;
    currentRegionID = region.currentRegionID;
    previousRegionID = region.previousRegionID;
    nextRegionID = region.nextRegionID;
    
    previousRegionFrameDelta = region.previousRegionFrameDelta;
    previousRegionIndex = region.previousRegionIndex;
    
    firstTimestamp = region.firstTimestamp;
    downTimestamp = region.downTimestamp;
    currentTimestamp = region.currentTimestamp;
    
    xSpan = region.xSpan;
    ySpan = region.ySpan;
    regionSize = region.regionSize;
    maxCapacity = region.maxCapacity;
    cumuStrength = region.cumuStrength;
    averageCapacity = region.averageCapacity;
    yxRatio = region.yxRatio;
    xyRatio = region.xyRatio;
    
    weightedCenterX = region.weightedCenterX;
    weightedCenterY = region.weightedCenterY;
    major = region.major;
    minor = region.minor;
    ecc = region.ecc;
    theta = region.theta;
    
    averageShadowLength = region.averageShadowLength;
    upperShadowSize = region.upperShadowSize;
    
    isStable = region.isStable;
    earClassifyResult = region.earClassifyResult;
    faceClassifyResult = region.faceClassifyResult;
}

void Region::operator=(const Region &region) {
    x = region.x;
    y = region.y;
    capacity = region.capacity;
    
    x2 = region.x2;
    y2 = region.y2;
    capacity2 = region.capacity2;
    
    for (int y = 0; y < GRID_RES_Y; ++y) {
        for (int x = 0; x < GRID_RES_X; ++x) {
            this->region[y][x] = region.region[y][x];
        }
    }
    
    touchID = region.touchID;
    currentRegionID = region.currentRegionID;
    previousRegionID = region.previousRegionID;
    nextRegionID = region.nextRegionID;
    
    previousRegionFrameDelta = region.previousRegionFrameDelta;
    previousRegionIndex = region.previousRegionIndex;
    
    firstTimestamp = region.firstTimestamp;
    downTimestamp = region.downTimestamp;
    currentTimestamp = region.currentTimestamp;
    
    xSpan = region.xSpan;
    ySpan = region.ySpan;
    regionSize = region.regionSize;
    maxCapacity = region.maxCapacity;
    cumuStrength = region.cumuStrength;
    averageCapacity = region.averageCapacity;
    yxRatio = region.yxRatio;
    xyRatio = region.xyRatio;
    
    weightedCenterX = region.weightedCenterX;
    weightedCenterY = region.weightedCenterY;
    major = region.major;
    minor = region.minor;
    ecc = region.ecc;
    theta = region.theta;
    
    averageShadowLength = region.averageShadowLength;
    upperShadowSize = region.upperShadowSize;
    
    isStable = region.isStable;
    earClassifyResult = region.earClassifyResult;
    faceClassifyResult = region.faceClassifyResult;
}

Region::~Region() {
    x.clear();
    y.clear();
    capacity.clear();
    
    x2.clear();
    y2.clear();
    capacity2.clear();
}

void Region::load(ifstream &fin) {
    int capacity_num = 0;
    fin >> capacity_num;
    for (int i = 0; i < capacity_num; ++i) {
        int grid_x, grid_y, grid_capacity;
        fin >> grid_x >> grid_y >> grid_capacity;
        region[grid_y][grid_x] += 1;
        x.push_back(grid_x);
        y.push_back(grid_y);
        capacity.push_back(grid_capacity);
    }
    int capacity2_num = 0;
    fin >> capacity2_num;
    for (int i = 0; i < capacity2_num; ++i) {
        int grid_x2, grid_y2, grid_capacity2;
        fin >> grid_x2 >> grid_y2 >> grid_capacity2;
        region[grid_y2][grid_x2] += 1;
        x2.push_back(grid_x2);
        y2.push_back(grid_y2);
        capacity2.push_back(grid_capacity2);
    }
    fin >> touchID >> currentRegionID >> previousRegionID >> nextRegionID >> previousRegionFrameDelta >> previousRegionIndex >> firstTimestamp >> downTimestamp >> currentTimestamp >> isStable >> earClassifyResult >> faceClassifyResult;
    fin >> xSpan >> ySpan >> regionSize >> maxCapacity >> cumuStrength >> averageCapacity >> yxRatio >> xyRatio;
    fin >> weightedCenterX >> weightedCenterY >> major >> minor >> ecc >> theta;
    fin >> averageShadowLength >> upperShadowSize;
}

void Region::save(ofstream &fout) {
    fout << capacity.size();
    for (int i = 0; i < capacity.size(); ++i) {
        fout << ' ' << x[i] << ' ' << y[i] << ' ' << capacity[i];
    }
    fout << endl;
    fout << capacity2.size();
    for (int i = 0; i < capacity2.size(); ++i) {
        fout << ' ' << x2[i] << ' ' << y2[i] << ' ' << capacity2[i];
    }
    fout << endl;
    fout << touchID << ' ' << currentRegionID << ' ' << previousRegionID << ' ' << nextRegionID << ' ' << previousRegionFrameDelta << ' ' << previousRegionIndex << ' ' << firstTimestamp << ' ' << downTimestamp << ' ' << currentTimestamp << ' ' << isStable << ' ' << earClassifyResult << ' ' << faceClassifyResult << endl;
    fout << xSpan << ' ' << ySpan << ' ' << regionSize << ' ' << maxCapacity << ' ' << cumuStrength << ' ' << averageCapacity << ' ' << yxRatio << ' ' << xyRatio << ' ';
    fout << weightedCenterX << ' ' << weightedCenterY << ' ' << major << ' ' << minor << ' ' << ecc << ' ' << theta << ' ';
    fout << averageShadowLength << ' ' << upperShadowSize << endl;
}
