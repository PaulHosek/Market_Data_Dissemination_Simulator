# Market Data Dissemination Simulator

[![CMake CI](https://github.com/PaulHosek/Market_Data_Dissemination_Simulator/actions/workflows/ci.yml/badge.svg)](https://github.com/PaulHosek/Market_Data_Dissemination_Simulator/actions/workflows/ci.yml)

A high-performance C++23 app to simulate low-latency market data feeds for high-frequency trading.

## Build Instructions
### Prerequisites
- CMake 3.22 or higher
- MinGW-w64 (g++ with C++23 support)
- vcpkg for managing dependencies
- Libraries: ZeroMQ, Boost.Lockfree, spdlog, Google Test

### Setup
1. **Clone the repo**:
   ```bash
   git clone https://github.com/PaulHosek/Market_Data_Dissemination_Simulator.git
   cd Market_Data_Dissemination_Simulator
   ```

2. **Install CMake**:
    - Grab CMake from [cmake.org/download](https://cmake.org/download) (e.g., Windows installer).
    - Add it to your PATH during install.
    - Check it works:
      ```bash
      cmake --version
      ```

3. **Install MinGW**:
    - Install MinGW-w64 via Chocolatey:
      ```bash
      choco install mingw -y
      ```
    - Or download from [sourceforge.net/projects/mingw-w64](https://sourceforge.net/projects/mingw-w64/).
    - Ensure `g++` and `mingw32-make` are in your PATH:
      ```bash
      g++ --version
      mingw32-make --version
      ```

4. **Set up vcpkg**:
    - Clone and bootstrap vcpkg:
      ```bash
      git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
      cd C:\vcpkg
      git checkout 2025.02.14
      .\bootstrap-vcpkg.bat
      cd ..
      ```

5. **Install dependencies**:
    - Use vcpkg to get the libraries:
      ```bash
      C:\vcpkg\vcpkg install zeromq:x64-mingw-dynamic boost-lockfree:x64-mingw-dynamic spdlog:x64-mingw-dynamic gtest:x64-mingw-dynamic
      ```

6. **Build the project**:
    - Configure with CMake:
      ```bash
      cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic -G "MinGW Makefiles"
      ```
    - Build:
      ```bash
      mingw32-make -C build
      ```

7. **Run tests**:
    - Run the Google Test suite:
      ```bash
      cd build
      ctest -C Release
      ```

## Project Structure
- `src/`: Main code (e.g., `main.cpp`).
- `include/`: Header files.
- `tests/`: Unit tests (`test_basic.cpp`).
- `docs/`: Design docs (coming soon).
