# BindlessVK 

<div align="center">
<img src="https://github.com/Light7734/bindlessvk/raw/main/Branding/bindlessvk.svg"/>
<br/>

<i>
"Perfection is achieved, not when there is nothing left to add but when there is nothing left to take away..."

-Antoine de St. Exupery, Wind, Sand, and Stars, 1939
</i>
</div>

## What's all this?
BindlessVK is a physically based renderer(PBR) written in modern C++ using Vulkan API, it utilizes a bindless design to minimize the state changes.

## Development Guideline
**Getting started:**
```
git clone --recurse-submodules 'git@github.com:light7734/bindlessvk.git' bindlessvk
mkdir bindlessvk/build
cd bindlessvk/build
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
