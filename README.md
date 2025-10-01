# DirectX 11 Raytracing Project

This project demonstrates **classic ray tracing** and a **mesh-based rendering mode** using DirectX 11.  
It is designed to showcase real-time raytracing techniques alongside mesh rendering approaches for visual comparison and performance analysis.

## Features

- **Classic Raytracing Mode**  
  Implements ray tracing at a pixel level, simulating realistic light interactions such as reflections and shadows.

- **Mesh Mode**  
  Utilizes mesh geometry with standard rasterization for rendering, allowing performance comparisons with raytracing.

## How to Run

1. Clone the repository.
2. Open the project in **Visual Studio**.
3. Build the project with the `/std:c++latest` flag enabled.
4. Run the executable — use the mode selection interface to switch between **Classic Raytracing** and **Mesh Mode**.

## Results

Below is a sample rendering result from the project in **Classic Raytracing Mode**:

![Result](pic.png)

## Requirements

- Windows 10 or later
- Visual Studio 2019 or later
- DirectX 11 compatible GPU
- C++17 or later

## License

This project is licensed under the MIT License. See the LICENSE file for details.
