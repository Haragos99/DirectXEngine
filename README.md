# DirectX Engine

A small DirectX 11 editor for experimenting with 3D models, Gaussian splats, skeletal skinning and marching-cubes meshing. You load models, arrange them in a scene, build a skeleton, bind a mesh to it and pose it, and turn point clouds or Gaussian splats into meshes. Everything is driven from a docked control panel.

<!-- IMAGE: overview screenshot of the editor (scene + control panel), e.g. Results/overview.png -->

## Features

- **Model import**: OBJ meshes, PLY point clouds and PLY Gaussian splats (standard 3D Gaussian Splatting files and SuperSplat compressed files).
- **Scene editing**: a hierarchy of objects with drag-and-drop parenting, a properties panel for position, rotation and scale, and Move/Scale gizmos in the viewport.
- **Gaussian splat rendering**: depth-sorted splats, with adjustable splat size.
- **Skeletons**: create a skeleton, add joints and pose them with the gizmo.
- **Skinning**: bind a mesh to a skeleton and choose how it deforms:
  - weights: *Default (closest bone)* or *Bone Heat* (diffuses weights over the mesh surface);
  - deformation: *Linear Blend* or *Dual Quaternion* skinning, computed on the GPU.
- **Weight inspection**: colour the mesh by its weights (all bones blended, or one bone from blue 0 to red 1), and open a sortable per-vertex weight table.
- **Marching cubes**: generate a mesh from a scalar field (a sphere), or turn a selected splat cloud into a mesh.
- **Render modes**: solid, wireframe, and solid with wireframe overlay.

<!-- IMAGE: Gaussian splat scene, e.g. Results/splats.png -->
<!-- IMAGE: skinned mesh with the weight view, e.g. Results/skinning.png -->
<!-- IMAGE: mesh created with marching cubes, e.g. Results/marching-cubes.png -->

## Controls

### Camera

| Input | Action |
|---|---|
| `W` / `S` | Move forward / back |
| `A` / `D` | Move left / right |
| `Space` / `Shift` | Move up / down |
| `Ctrl` + drag with the left mouse button | Rotate the camera |
| Mouse wheel | Zoom |

### Selecting and editing

| Input | Action |
|---|---|
| Double-click an object in the viewport | Select it |
| Click an object in the **Hierarchy** panel | Select it |
| Drag a gizmo handle (left mouse button) | Move or scale the selected object along that axis |
| `Delete` | Delete the selected object (works from the viewport and from the panels) |
| `R` | Cycle render mode: solid → wireframe → solid + wireframe |

Keys typed into a text field in the panel go to that field, not to the camera.

## The control panel

The panel is split into sections. Some only appear when they apply to the current selection.

| Section | What it does |
|---|---|
| **Model** | *Import model* opens a file dialog for `.obj` and `.ply` files. |
| **Transform mode** | Chooses whether the gizmo moves or scales (*Move* / *Scale*). |
| **Hierarchy** | Lists every object. Click a row to select it, and drag a row onto another to make it a child. *Unparent* moves the selection back to the top level; *Clear* deselects. |
| **Object Properties** (own window) | Name, position, rotation and scale of the selection. Drag a field, or Ctrl+click it to type a value. *Uniform scale* keeps the axes together; *Reset transform* sets position and rotation to zero and scale to one. |
| **Skeleton** | *Create skeleton* adds a skeleton with one root joint. With a skeleton or joint selected: *Add child joint* and *Reset pose*. |
| **Skin Binding** | With a mesh selected: pick the skeleton, the *Technique* (Linear Blend or Dual Quaternion) and the *Weights* method, then *Bind*. Once bound, *Weight view* colours the mesh, *Weight table* opens the per-vertex table, and *Unbind* detaches it. |
| **Skin Weights** (own window) | A table with each vertex's weight per bone. Sort by any column, filter to blended vertices, weld vertices split by UV seams, and jump to a vertex by number. Click a weight to show that bone in the viewport. |
| **Marching Cubes** | Set *Radius*, *Resolution* and *Iso level*, then *Generate sphere mesh*. |
| **Splat size** | With a splat cloud selected: scales the size of every splat. |
| **Splat meshing** | With a splat cloud selected: *Grid resolution*, *Density threshold* and *Splat reach*, then *Create mesh from splats*. Meshing runs in the background with a progress window; the mesh appears when it finishes. |
| **Viewport** | Frame time and FPS, *Cycle render mode*, background *Clear color*, and the ImGui demo window. |

## Typical workflows

**Pose a skinned mesh**
1. *Import model* → choose an `.obj` file.
2. *Create skeleton*, then use *Add child joint* to build a chain. Select joints and move them into the mesh with the gizmo.
3. Select the mesh. In **Skin Binding** pick the skeleton, a technique and a weights method, then press *Bind*.
4. Select a joint and move it with the gizmo: the mesh follows. *Reset pose* returns the skeleton to its bind pose.

**Turn Gaussian splats into a mesh**
1. *Import model* → choose a splat `.ply` file.
2. Select the cloud. In **Splat meshing** adjust the settings and press *Create mesh from splats*.
3. The new mesh is added to the scene at the cloud's position.

## Building

### Requirements

- Windows 10 or later and a DirectX 11 GPU (feature level 11_0)
- Visual Studio 2022 with the **Desktop development with C++** workload (toolset v143, C++20)
- [vcpkg](https://vcpkg.io) for Eigen

### Steps

1. Clone the repository and fetch Dear ImGui (a git submodule):
   ```bash
   git submodule update --init --recursive
   ```
2. Install Eigen, used by Bone Heat skinning, and enable vcpkg's Visual Studio integration:
   ```bash
   vcpkg install eigen3:x64-windows
   vcpkg integrate install
   ```
3. Open `DirectXEngine.sln`, choose **Debug** or **Release** with **x64**, and build. `DirectXEngine` is the startup project.
4. Run it. The exe is written to `x64\<Config>\DirectXEngine.exe`, with its compiled shaders in `x64\<Config>\shaders\`. You can also start it outside Visual Studio from any folder: it finds the `Resources` folder by searching upwards from the exe, so keep the exe inside the repository.

Shaders are compiled during the build, so a shader error appears as a build error. After editing a `.hlsl` file, rebuild before running.

## Project structure

The solution is split into static libraries. Each library may only use the ones below it, and the build enforces this through the include paths.

```
Core        math types, camera, transform, half-edge mesh
Rendering   D3D11 device and swap chain, shaders, textures, splat rendering
ModelIO     OBJ and PLY parsing
Scene       scene objects, World, gizmos, importers, environment
Animation   skeletons and skinning
Voxel       scalar fields and marching cubes
UI          ImGui control panel
DirectXEngine (exe)   window, input, editor
```

Source is in `src/<Project>/`, third-party code in `External/`, models and textures in `Resources/`. Shared build settings live in `src/Engine.props`.

## License

This project is licensed under the MIT License.
