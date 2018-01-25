

# Panther

Panther is an open-source, highly efficient text editor written from scratch in C++. It supports both OSX and Windows currently, and has future plans for Linux support. It uses the Skia library for rendering, a modified version of the RE library for efficient regular expression matching and ICU for unicode support.

![Screenshot of Panther](https://raw.githubusercontent.com/Mytherin/Panther/master/Images/screenshot.png)

# Building

Panther can be build using CMake. The below example creates debug build of Panther inside the "build" directory.

```bash
mkdir build
cd build
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug ..
make
```

Different generator options can be used depending on which build environment you are using. If you wish to use Panther as an actual editor (i.e. compile for usage instead of for development), remember to use the Release option.

