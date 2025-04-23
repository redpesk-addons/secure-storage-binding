# Installation

## Rebuilding from source

### Install Dependencies

* Add the redpesk repository to your system. For detailed instructions, refer to the [redpesk documentation](https://docs.redpesk.bzh/).

#### For example, if you are using Fedora:

```bash
sudo dnf in cmake gcc g++ afb-cmake-modules json-c-devel afb-binding-devel libdb-devel findutils procps-ng
```

### Build for Linux distribution

```bash
cd secure-storage-binding
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=COVERAGE -DSECSTOREADMIN=ON ..
make 
```

After running make, the .so file will be generated in the build/secstorage directory :

```bash
~/secure-storage-binding/build$ tree -L 2
...
├── secstorage
│   ├── afb-service-secure-storage.so
│   ├── CMakeFiles
│   ├── cmake_install.cmake
│   └── Makefile
...

42 directories, 23 files

```