"""

        Pybind11 example plugin
        -----------------------

        .. currentmodule:: pointcloud_sax_reader

        .. autosummary::
           :toctree: _generate

           bbox2d_of_pointcloud
           bbox3d_of_pointcloud

"""
from __future__ import annotations

__all__: list[str] = ["bbox2d_of_pointcloud", "bbox3d_of_pointcloud"]

def bbox2d_of_pointcloud(path: str) -> list[float]: ...
def bbox3d_of_pointcloud(path: str) -> list[float]: ...

__version__: str = "0.1.2"
