# Build Instructions on cmake 

If you just want to compile this project using cmake for your native platform
use cmake as normal. However if you want to crosscompile using something like 
mingw create a toolchain file. 

For linux -> windows crosscompilation using mingw 2 toolchain files are supplied
one for 32 bit and one for 64 bit binaries.

You can use them like this : `cmake -DCMAKE_TOOLCHAIN_FILE=<file of your choosing>` 
etc.etc. but you get the picture.

# Build options 

To aid in finding issues 3 options can be turned on/off

* ENABLE_PEDANTIC enable -pedantic
* ENABLE_WALL     enable -Wall
* ENABLE_WERROR   enable -Werror

By default these are all turned off.

