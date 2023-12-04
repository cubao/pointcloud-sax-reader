from __future__ import annotations

import sys
import time

from . import bbox3d_of_pointcloud

if __name__ == "__main__":
    assert len(sys.argv) > 1, f"usage:\n\t{sys.argv[0]} path/to/file.pcd"
    path = sys.argv[1]
    tic = time.time()
    bbox = bbox3d_of_pointcloud(path)
    secs = time.time() - tic
    print(f"bbox: {bbox} (ours, time: {secs:.6f})")  # noqa: T201

    try:
        import open3d as o3d

        tic = time.time()
        pcd = o3d.io.read_point_cloud(path)
        aabb = pcd.get_axis_aligned_bounding_box()
        secs = time.time() - tic
        bbox = [*aabb.min_bound.tolist(), *aabb.max_bound.tolist()]
        print(f"bbox: {bbox} (open3d, time: {secs:.6f})")  # noqa: T201
    except ImportError:
        pass
