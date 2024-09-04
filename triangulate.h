#pragma once

struct Point {
    double x, y;
};

struct Triangle {
    int v1, v2, v3;
};

std::vector<Triangle> triangulate_polygon(const std::vector<Point>& points);
