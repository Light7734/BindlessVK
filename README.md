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

**Commit syntax (almost) follows the** [**_conventional commits specification_**](https://www.conventionalcommits.org/en/v1.0.0/):
```
<type>[optional scope]: <description>

[optional body]

[optional footer(s)]
```

**Commit types:**
-   **beh:** Changes to the behavior.
    - beh(bvk): add multi sample anti-aliasing
    - beh(demo): randomize objects' transformation
    - beh(asset-loader): add progress logging
    - beh(profiler): output will be prettified

-   **fix:** Changes that fix a malfunction in behavior.
    - fix(asset-loader): file handle left open
    - fix(demo): incorrect lights' parameters
    - fix(profiler): missing closing brackets

-   **ref:** Changes that improve the internal structure of the code without altering its external behavior.
    - ref(bvk): optimize render loop
    - ref(bvk): rename methods & variables
    - ref(profiler): remove unused variables
    - ref(asset-loader): split large class 'a' into 'a', 'b' & 'c'

-   **sty:** Aesthetic changes that won't affect the structure or the behavior of the code.
    - sty: update .clang-format
    - sty: tidy up CMakeLists
    - sty: change naming convention for 'some type'
    - sty(bvk): remove extra whitespaces
    - sty(profiler): re-order methods

-   **doc:** Changes to the documentations.
    - doc: update repository readme
    - doc(bvk): fix spelling errors
    - doc(profiler): add docs for class 'a'
    - doc(asset-loader): remove comments

-   **ass:** Changes to the assets.
    - ass(branding): add bindlessvk logo
    - ass(demo): fix player weight paintings
    - ass(demo): remove unused files 

**Branches:**
-   **main:** the main development branch.

## Contact
Feel free to HMU anytime on matrix @light7734:matrix.org to chat about programming or whatever. ♥️
