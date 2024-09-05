#pragma once

struct vec2d {
    double x, y;
};

struct triangle {
    int v1, v2, v3;
};

std::vector<triangle> triangulate_polygon(const std::vector<vec2d>& points);
