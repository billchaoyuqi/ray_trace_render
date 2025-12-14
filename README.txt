Please locate the Makefile within the build folder and execute the make command from the terminal within the build directory.
To accelerate operational efficiency, I have employed the OpenMP external library for parallel computing acceleration. Please ensure the availability of this library before running the program.

Implementation Guide：
Usage: D:\c++project\graphic_cw\build\graphic_cw.exe [options]
Options:
  --bvh / --no-bvh     Enable/disable BVH acceleration
  --motion-blur        Enable motion blur effects
  --distributed        Enable distributed rendering with soft shadows
  --shadow-samples N   Number of shadow samples (default: 4)
  --help               Show this help message

As the project was developed on a Windows platform, I was unable to identify a direct method for transitioning from CMake to a Makefile structure.
Alternatively, if feasible, please execute the code within Clion, which should resolve the issue, and the corresponding CMakeLists file will be submitted alongside.