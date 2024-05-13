#ifndef CENTROID_TRACKER_H
#define CENTROID_TRACKER_H

/*
*  Developed by: Solomon Muhunyo Githu, 10th May 2024.
*/

#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

// use Arduino.h when you are using Arduino functions in .cpp
//#include <Arduino.h>

struct BoundingBox {
  int x, y, width, height;
};

class CentroidTracker {
  private:
    int nextObjectID = 0;
    std::vector<int> objectIDs;
    std::vector<std::pair<int, int>> centroids;
    std::vector<int> disappeared;
    int maxDisappeared;

  public:
    CentroidTracker(int maxDisappeared = 5);

    void registerObject(std::pair<int, int> centroid);

    void deregisterObject(int objectID);

    std::vector<int> update(std::vector<BoundingBox>& rects);

    std::vector<int> getObjectIDs() const;
    std::vector<std::pair<int, int>> getCentroids() const;
};

#endif