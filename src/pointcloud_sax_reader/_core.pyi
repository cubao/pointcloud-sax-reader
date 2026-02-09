"""

        Pybind11 example plugin
        -----------------------

        .. currentmodule:: pointcloud_sax_reader

        .. autosummary::
           :toctree: _generate

           add
           subtract
    
"""
from __future__ import annotations
__all__: list[str] = ['add', 'bbox2d_of_pointcloud', 'bbox3d_of_pointcloud', 'subtract']
def add(arg0: int, arg1: int) -> int:
    """
            Add two numbers
    
            Some other explanation about the add function.
    """
def bbox2d_of_pointcloud(path: str) -> list[float]:
    ...
def bbox3d_of_pointcloud(path: str) -> list[float]:
    ...
def subtract(arg0: int, arg1: int) -> int:
    """
            Subtract two numbers
    
            Some other explanation about the subtract function.
    """
__version__: str = '0.1.1'
