#pragma once

#include <functional>
#include <limits>
#include <string>
#include <vector>

namespace cubao {
bool pointcloud_sax_read(const std::string& path,
                         const std::function<void(double, double, double)>& point_handler);
inline std::vector<double> bbox2d_of_pointcloud(const std::string& path) {
  double xmin = std::numeric_limits<double>::infinity();
  double ymin = std::numeric_limits<double>::infinity();
  double xmax = -std::numeric_limits<double>::infinity();
  double ymax = -std::numeric_limits<double>::infinity();
  if (pointcloud_sax_read(path, [&](double x, double y, double) {
        if (x < xmin) {
          xmin = x;
        }
        if (x > xmax) {
          xmax = x;
        }
        if (y < ymin) {
          ymin = y;
        }
        if (y > ymax) {
          ymax = y;
        }
      })) {
    return {xmin, ymin, xmax, ymax};
  }
  return {};
}
inline std::vector<double> bbox3d_of_pointcloud(const std::string& path) {
  double xmin = std::numeric_limits<double>::infinity();
  double ymin = std::numeric_limits<double>::infinity();
  double zmin = std::numeric_limits<double>::infinity();
  double xmax = -std::numeric_limits<double>::infinity();
  double ymax = -std::numeric_limits<double>::infinity();
  double zmax = -std::numeric_limits<double>::infinity();
  if (pointcloud_sax_read(path, [&](double x, double y, double z) {
        if (x < xmin) {
          xmin = x;
        }
        if (x > xmax) {
          xmax = x;
        }
        if (y < ymin) {
          ymin = y;
        }
        if (y > ymax) {
          ymax = y;
        }
        if (z < zmin) {
          zmin = z;
        }
        if (z > zmax) {
          zmax = z;
        }
      })) {
    return {xmin, ymin, zmin, xmax, ymax, zmax};
  }
  return {};
}
}  // namespace cubao
