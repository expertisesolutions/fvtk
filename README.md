# FTK

FTK is a 2D toolkit library in development written using Vulkan Graphics API.

## Why Vulkan

Vulkan is the replacement API for OpenGL. It allows more optimization opportunities. Which we make use extensively in this project.

## Getting Started

Download FTK git repositories at https://github.com/expertisesolutions/ftk .

Then, install boost-build (apt-get install libboost1.67-tools-dev, replace version accordingly, for Ubuntu and yay -S boost-build in Arch Linux).

And install conan (https://docs.conan.io/en/1.8/installation.html).

Run conan to bring and build dependencies and run b2 to compile:

```
$ conan install . --build
$ b2 --use-package-manager=conan
```
