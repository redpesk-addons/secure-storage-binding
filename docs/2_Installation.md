# Installation

## From package

You can run the same command on a target runing a redpesk OS or in the [SDK container]({% chapter_link sdk-container-doc.overview %}) (development mode).

```bash
dnf install secure-storage-binding
```

## From sources

When developing inside the SDK container, to install the build dependencies, run the following command:

```bash
dnf builddep secure-storage-binding
```

Then clone and build from sources.

```bash
git clone https://github.com/redpesk-addons/secure-storage-binding
cd secure-storage-binding
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=COVERAGE -DSECSTOREADMIN=ON ..
make 
```

> Note: To rebuild all (including application framework) from sources, please refer to this [chapter]({% chapter_link host-build-doc.build-framework-on-your-computer %}).
