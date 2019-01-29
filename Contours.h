/*
  Copyright (c) 2018, Digital Media Professionals. All rights reserved.
*/

#pragma once

#include <vector>

#define RESULT_POINT_LEN    5

struct Point
{
    Point(int _x, int _y) : x(_x), y(_y) {}
    Point() : x(0), y(0) {}
    int x;
    int y;
    
    //bool operator!=(const Point& t0, const Point& t1) {
    //    return t0.x != t1.x || t0.y != t1.y;
    //}
    //Point operator+(const Point& t0, const Point& t1) {
    //    return Point(t0.x + t1.x, t0.y + t1.y);
    //}
    //Point operator/(const Point& t0, int val) {
    //    return Point(t0.x / val, t0.y / val);
    //}
    const Point& operator=(const Point& t1) {
        x = t1.x;
        y = t1.y;
        return t1;
    }
};

typedef std::vector<Point> PointArray[3];

/**
* @brief fp16 image to edge points.
* @param[in] image      fp16 image. byte/pixel=2
* @param[in] width      width of image
* @param[in] height     height of image
* @param[out] points    edge points
* @return bool true if success
*/
bool contours(void *image, int width, int height, std::vector<Point> &points);

/**
* @brief contour points to 5 points
* @param[in] points     contour points
* @param[in] ch         channel
* @param[out] left      left side 5 points
* @param[out] right     right side 5 points
* @return int (is_valid_right ? 2 : 0) | (is_valid_left ? 1 : 0)
*/
int get_edge_point(std::vector<Point> &points, Point *left, Point *right);


void fp16x2_transpose(const void *image, void* buffer, int width, int height, int bpp);
