#include <pybind11/pybind11.h>

#include "pointcloud_sax_reader.hpp"

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

int add(int i, int j) {
  return i + j;
}

namespace py = pybind11;

PYBIND11_MODULE(_core, m) {
  m.doc() = R"pbdoc(
        Pybind11 example plugin
        -----------------------

        .. currentmodule:: pointcloud_sax_reader

        .. autosummary::
           :toctree: _generate

           add
           subtract
    )pbdoc";

  m.def("add", &add, R"pbdoc(
        Add two numbers

        Some other explanation about the add function.
    )pbdoc");

  m.def(
      "subtract", [](int i, int j) { return i - j; }, R"pbdoc(
        Subtract two numbers

        Some other explanation about the subtract function.
    )pbdoc");

  m.def("bbox2d_of_pointcloud", &cubao::bbox2d_of_pointcloud, "path"_a);
  m.def("bbox3d_of_pointcloud", &cubao::bbox3d_of_pointcloud, "path"_a);

#ifdef VERSION_INFO
  m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
  m.attr("__version__") = "dev";
#endif
}
