# Installation

## Rebuilding from source

### Install Dependencies

* Declare redpesk repository: (see [doc](http://redpesk-doc-internal.lorient.iot/docs/en/master/developer-guides/host-configuration/docs/1-Setup-your-build-host.html))

#### Fedora 33

```bash
sudo dnf in cmake gcc g++ afb-cmake-modules json-c-devel afb-binding-devel libdb-devel findutils procps-ng
```

### Build for Linux distribution

```bash
cd secure-storage-binding
mkdir -p build;
cd build;
cmake -DBUILD_TEST_WGT=TRUE -DCMAKE_BUILD_TYPE=COVERAGE ..;
make SECSTOREADMIN=1 ;
make SECSTOREADMIN=1 widget;
```

## TEST

If you want to run the test and the code coverage just execute code:

```bash
cd service-secure-storage
cd build;
export AFB_PASSWD_DIR=$(pwd)/package-test/var
mkdir -p ${AFB_PASSWD_DIR}
echo "test_encryption_key" > ${AFB_PASSWD_DIR}/test.passwd
afm-test package package-test -c;
```
