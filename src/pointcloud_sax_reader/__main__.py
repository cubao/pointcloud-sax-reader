from __future__ import annotations

import sys

from . import bbox3d_of_pointcloud

if __name__ == "__main__":
    assert len(sys.argv) > 1, f"usage:\n\t{sys.argv[0]} path/to/file.pcd"
    bbox = bbox3d_of_pointcloud(sys.argv[1])
    print("bbox:", bbox)  # noqa: T201
