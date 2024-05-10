#include "centroid_tracker.h"


// Credits : https://www.pyimagesearch.com/2018/07/23/simple-object-tracking-with-opencv/

/* This cpp file does the object tracking and returns the: 
    - number of objects being tracked
    - centroids of the tracked objects 
*/

CentroidTracker::CentroidTracker(int maxDisappeared) : maxDisappeared(maxDisappeared) {}

void CentroidTracker::registerObject(std::pair<int, int> centroid) {
  objectIDs.push_back(nextObjectID);
  centroids.push_back(centroid);
  disappeared.push_back(0);
  nextObjectID++;
}

void CentroidTracker::deregisterObject(int objectID) {
  auto it = std::find(objectIDs.begin(), objectIDs.end(), objectID);
  if (it != objectIDs.end()) {
    int index = std::distance(objectIDs.begin(), it);
    objectIDs.erase(objectIDs.begin() + index);
    centroids.erase(centroids.begin() + index);
    disappeared.erase(disappeared.begin() + index);
  }
}

std::vector<int> CentroidTracker::update(std::vector<BoundingBox>& rects) {
  if (rects.empty()) {
    for (size_t i = 0; i < disappeared.size(); ++i) {
      disappeared[i]++;
      if (disappeared[i] > maxDisappeared) {
        deregisterObject(objectIDs[i]);
      }
    }
    //std::cout << "Number of objects: " << objectIDs.size() << std::endl;
    //Serial.print("Number of objects: ");
    //Serial.println(objectIDs.size());
    return objectIDs;
  }

  std::vector<std::pair<int, int>> inputCentroids;
  inputCentroids.reserve(rects.size());

  for (const auto& rect : rects) {
    int cX = rect.x + rect.width / 2;
    int cY = rect.y + rect.height / 2;
    inputCentroids.emplace_back(cX, cY);
  }

  if (objectIDs.empty()) {
    for (const auto& centroid : inputCentroids) {
      registerObject(centroid);
    }
  } else {
    std::vector<bool> usedRows(objectIDs.size(), false);
    std::vector<bool> usedCols(inputCentroids.size(), false);

    for (size_t i = 0; i < objectIDs.size(); ++i) {
      int minDistance = std::numeric_limits<int>::max();
      size_t minIndex = 0;

      for (size_t j = 0; j < inputCentroids.size(); ++j) {
        int distance = std::sqrt(std::pow(centroids[i].first - inputCentroids[j].first, 2) +
                                 std::pow(centroids[i].second - inputCentroids[j].second, 2));
        if (distance < minDistance && !usedCols[j]) {
          minDistance = distance;
          minIndex = j;
        }
      }

      if (minDistance < 20) {
        centroids[i] = inputCentroids[minIndex];
        disappeared[i] = 0;
        usedRows[i] = true;
        usedCols[minIndex] = true;
      }
    }

    for (size_t i = 0; i < usedRows.size(); ++i) {
      if (!usedRows[i]) {
        disappeared[i]++;
        if (disappeared[i] > maxDisappeared) {
          deregisterObject(objectIDs[i]);
        }
      }
    }

    for (size_t i = 0; i < usedCols.size(); ++i) {
      if (!usedCols[i]) {
        registerObject(inputCentroids[i]);
      }
    }
  }

  //std::cout << "Number of objects: " << objectIDs.size() << std::endl;
  //Serial.print("Number of objects: ");
  //Serial.println(objectIDs.size());
  
  return objectIDs;
}

std::vector<int> CentroidTracker::getObjectIDs() const {
    return objectIDs;
}

std::vector<std::pair<int, int>> CentroidTracker::getCentroids() const {
    return centroids;
}
