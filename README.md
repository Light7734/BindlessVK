# Vulkan-Renderer [![forthebadge](https://forthebadge.com/images/badges/works-on-my-machine.svg)](https://forthebadge.com)
Realtime 3d renderer developed using the Vulkan graphics API and C++, built mostly for learning purposes.

## Setting up the project
Prerequisites:
* git
* conan
* cmake
* vulkan-drivers

```
git clone --recurse-submodules 'git@github.com:light7734/vulkan-renderer.git' vulkan-renderer
mkdir vulkan-renderer/build
cd vulkan-renderer/build
conan install .. --build=missing
cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=DEBUG
cmake --build . -j`nproc`
cd ..
ln -s ./build/compile_commands.json ./compile_commands.json
```

## Development
**Commit syntax: Category(scope): Description**

Branches:
* main: The main development branch
* stable: This branch will most-likely build
* backup: The old vulkan-renderer (re did the project because I forgot everything)

You can checkout the roadmap of this project on [Trello board](https://trello.com/b/idJempUN/vulkan-renderer)

I have a list of learning resoruces on my [homepage](https://mohammad-hm.com/about)

## Contact
Feel free to HMU anytime on discord @Light7734#4652 to chat about programming or whatever.
