# BindlessVK [![forthebadge](https://forthebadge.com/images/badges/works-on-my-machine.svg)](https://forthebadge.com)

<div align="center">
<img src="https://github.com/Light7734/Vulkan-Renderer/raw/main/Branding/bindlessvk.svg"/>

<i> "Nvidia, F*** You!" </i>

-Linus Torvalds
</div>

## What's all this?
BindlessVK is a realtime 3d renderer developed using the Vulkan graphics API and C++, built mostly for learning purposes.

## Development Guideline
**Getting started:**
```
git clone --recurse-submodules 'git@github.com:light7734/vulkan-renderer.git' vulkan-renderer
mkdir vulkan-renderer/build
cd vulkan-renderer/build
cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=DEBUG
cmake --build . -j`nproc`
cd ..
ln -s ./build/compile_commands.json ./compile_commands.json
```

**Commit syntax follows the** [**_conventional commits specification_**](https://www.conventionalcommits.org/en/v1.0.0/):
```
<type>[optional scope]: <description>

[optional body]

[optional footer(s)]
```

**Commit types:**
-   **feat:** Changes that add a new feature.
-   **fix:** Changes that fix a bug.
-   **refactor:** Changes that neither fix a bug nor add a feature.
-   **style:** Changes to code that won't affect the compiled code.
-   **docs:** Changes to the documentations.
-   **perf:** Changes that improve the code performance.
-   **asset:** Changes to the assets.
-   **wip:** Prefix added to aforementioned commit types to split the change in to multiple commits.

**Branches:**
-   **main:** main development branch.

## Contact
Feel free to HMU anytime on matrix @light7734:matrix.org to chat about programming or whatever. ♥️
