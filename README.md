# Secure Storage Service

The Redpesk secure storage is compliant with the [legato](https://legato.io/) secure storage API
[secStore](https://github.com/legatoproject/legato-af/blob/master/interfaces/le_secStore.api) and
[secStoreAdmin](https://github.com/legatoproject/legato-af/blob/master/interfaces/secureStorage/secStoreAdmin.api).

## Pre-requisites

## Setup

## Build for 'native' Linux distribution (Fedora, openSUSE, Debian, Ubuntu, ...)

```bash
cd rp-service-secure-storage
mkdir -p build;
cd build;
cmake -DBUILD_TEST_WGT=TRUE -DCMAKE_BUILD_TYPE=COVERAGE ..;
make;
make widget;
```

## TEST

### TEST on AGL

### Native Linux

```bash
cd rp-service-secure-storage
cd build;
export AFB_PASSWD_FILE=$(pwd)/package-test/var
afm-test package package-test -c;
```

## Activate authentication security
