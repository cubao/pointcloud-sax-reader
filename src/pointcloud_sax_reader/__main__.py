from __future__ import annotations

import sys

from . import bbox3d_of_pointcloud

if __name__ == "__main__":
    bbox = bbox3d_of_pointcloud(sys.argv[1])
    print("bbox:", bbox)  # noqa: T201
