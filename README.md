[![Storm upstream](https://img.shields.io/badge/storm--upstream-e0ff44df-blue)](https://github.com/stormchecker/storm/commit/e0ff44df1da4b4607dce3ec18404c76fe26a2fd1)

# Tempest

Tempest is a shield synthesis tool built as a fork of [Storm](https://www.stormchecker.org/), a probabilistic model checker. It extends Storm's model checking with the prerequisites for synthesizing shields from probabilistic models.

## Dependencies

Tempest inherits Storm's dependencies. Install them by following the official Storm documentation:

[https://www.stormchecker.org/documentation/obtain-storm/dependencies.html](https://www.stormchecker.org/documentation/obtain-storm/dependencies.html)


## Installation

The recommended build uses [Ninja](https://ninja-build.org/) for faster incremental compilation. If Ninja is not available, plain CMake (Make backend) works as a drop-in alternative.

**With Ninja (recommended):**

```bash
# Clone the repository
git clone <tempest-repo-url>
cd tempest

# Configure
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -GNinja -DSTORM_BUILD_TESTS=OFF

# Build
ninja -C build -j$(nproc)
```

**With CMake / Make:**

```bash
git clone <tempest-repo-url>
cd tempest

# Configure (omit -GNinja to use the default Make generator)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DSTORM_BUILD_TESTS=OFF

# Build
cmake --build build -j$(nproc)
```

The compiled binary is located at `build/bin/tempest`.

## TempestPy

Tempest serves as the computational backend for **TempestPy**, a Python interface for shield synthesis. See the TempestPy repository for Python bindings and high-level shield synthesis workflows:

[TempestPy repository](#) <!-- TODO: replace with actual link -->
