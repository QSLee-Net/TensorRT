# voxelGenerator Plugin [DEPRECATED]

**This plugin is deprecated since TensorRT 10.12 and will be removed in a future release. No alternatives are planned to be provided.**

**Table Of Contents**
- [Description](#description)
    * [Structure](#structure)
- [Parameters](#parameters)
- [Additional resources](#additional-resources)
- [License](#license)
- [Changelog](#changelog)
- [Known issues](#known-issues)

## Description

The `voxelGeneratorPlugin` performs the generation of voxels(pillars) from raw points in a point cloud frame. This operation essentially quantize the 3D points in spacial dimensions(x, y, z) with a certain granularity. The output of this plugin will be a group of pillars.

`voxelGeneratorPlugin` implements a quantization of 3D points in point cloud data and produces a groups of voxels. Each voxel is either empty or contains several points that are close to each other.

This plugin is optimized for the above steps and it allows you to do PointPillars inference in TensorRT.


### Structure

The `voxelGeneratorPlugin` takes 2 inputs; `points`, and `num_points`.

`points`
The input raw points from a point cloud. The shape of this tensor is `[N, M, C]`, where `N` is batch size, `M` is the maximum number of points in a point cloud frame, and `C` is the number of channels for each point.
Since point cloud data is sparse in nature, each frame will generally have different number of valid points(no more than `M`). Zero-padding should be applied properly to construct a dense tensor from a batch of point cloud frames.


`num_points`
The number of valid points in each frame. The valid number of points should be no more than `M`. The shape of this tensor is `[N]`.


The `voxelGeneratorPlugin` generates the following 3 outputs:

`voxels`
The voxels generated by this plugin. The shape of this tensor is `[N, V, P, C']`, where `N` is batch size, `V` is the maximum number of voxels(pillars) per frame, `P` is the maximum number of points per voxel, and `C'` is the number of channels(features) per point in voxels.


`voxel_coords`
The coordinates of each voxel in `voxels`. This coordinates tensor will be used to compute a dense feature map indirectly from the `voxels`(after some reduction operations are applied to `voxels`). The shape of this tensor is `[N, V, 4]`, where `N, V` are as above and 4 is just the length of coordinates encoded as `(frame_id, z, y, x)`.


`num_pillar`
The number of valid voxels(pillars) in `voxels` for each frame. This will be used to generate the dense feature map. The shape of this tensor is `[N]`.

## Parameters

`voxelGeneratorPlugin` has plugin creator class `voxelGeneratorPluginCreator` and plugin class `voxelGeneratorPlugin`.

The parameters are defined below and consists of the following attributes:

| Type     | Parameter                | Description
|----------|--------------------------|--------------------------------------------------------
| `int`    | `max_num_points_per_voxel` | Maximum number of points per voxel.
| `int` | `max_voxels`        | Maximum number of voxels to be generated per frame.
| `list of floats` | `point_cloud_range` | The range of the point cloud coordinates.
| `int`    | `voxel_feature_num` | The number of channels of the generated voxels.
| `list of floats` | `voxel_size` | The size of the voxels.

## Additional resources

The following resources provide a deeper understanding of the `voxelGeneratorPlugin` plugin:

**Networks:**
-   [PointPillars](https://arxiv.org/pdf/1812.05784)

**Documentation:**
-   [PointPillars](https://arxiv.org/pdf/1812.05784)

## License

For terms and conditions for use, reproduction, and distribution, see the [TensorRT Software License Agreement](https://docs.nvidia.com/deeplearning/sdk/tensorrt-sla/index.html)
documentation.


## Changelog

May 2025
Add deprecation note.

Dec 2021
This is the first release of this `README.md` file.


## Known issues

There are no known issues in this plugin.
