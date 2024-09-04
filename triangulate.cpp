#include <vector>
#include <algorithm>
#include "triangulate.h"

namespace
{
    struct Event {
        enum Type { START, END } type;
        int index;
        Point point;
    };

    // Function to determine if a point is above or below a line segment
    bool is_above(const Point& p, const Point& p1, const Point& p2) {
        return (p2.y - p1.y) * (p.x - p1.x) - (p2.x - p1.x) * (p.y - p1.y) > 0;
    }

}

// Function to triangulate a polygon using the Sweep Line algorithm
std::vector<Triangle> triangulate_polygon(const std::vector<Point>& points)
{
    std::vector<Event> events;
    for (int i = 0; i < points.size(); ++i) {
        events.push_back({ Event::START, i, points[i] });
        events.push_back({ Event::END, i, points[i] });
    }

    std::sort(events.begin(), events.end(), [](const Event& e1, const Event& e2) {
        return e1.point.x < e2.point.x || (e1.point.x == e2.point.x && e1.type < e2.type);
        });

    std::vector<int> activePoints;
    std::vector<Triangle> triangles;

    for (const Event& e : events) {
        if (e.type == Event::START) {
            activePoints.push_back(e.index);
            int i = (int)(activePoints.size() - 2);
            while (i >= 0 && is_above(points[e.index], points[activePoints[i]], points[activePoints[i + 1]])) {
                triangles.push_back({ activePoints[i], activePoints[i + 1], e.index });
                activePoints.erase(activePoints.begin() + i + 1);
                --i;
            }
        }
        else {
            int i = 0;
            while (i < activePoints.size() - 1 && is_above(points[e.index], points[activePoints[i]], points[activePoints[i + 1]])) {
                triangles.push_back({ activePoints[i], activePoints[i + 1], e.index });
                activePoints.erase(activePoints.begin() + i + 1);
                ++i;
            }
            activePoints.erase(std::find(activePoints.begin(), activePoints.end(), e.index));
        }
    }

    return triangles;
}