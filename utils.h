#ifndef UTILS_H
#define UTILS_H

// use Arduino.h when you are using Arduino functions in .cpp
#include <Arduino.h> 

using namespace std;
#include <string>

extern String ip_address;
extern String index_html;
extern String current_found_objects_JSONData;

void set_index_html();
void append_found_objects_to_JSON(int bounding_box_X, int bounding_box_Y, int bounding_box_Width, int bounding_box_Height);
void append_tracked_objects_to_JSOData(int arr[][3], int number_of_tracked_objects);
void create_JSONData_for_noBBs();
void end_current_bounding_boxes_JSONData();
void clear_current_found_objects_JSONData();

#endif