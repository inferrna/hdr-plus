# HDR+ Implementation
Please see: http://timothybrooks.com/tech/hdr-plus

To compile, follow these steps:
1. Install Halide from http://halide-lang.org/.
2. Set `HALIDE_ROOT_DIR` in CMakeLists.txt to the Halide directory path.
3. Install dcraw and add it to PATH. E.g. through
```
apt-get install dcraw
```
Source code available at https://www.cybercom.net/~dcoffin/.
4. From the project root directory, run the following commands:
```
mkdir build
cd build
cmake ..
make
```
