Name: secure-storage-binding
Version: 2.0.0
Release: 1%{?dist}
Summary: Binding provide a database API with key/value semantics
License:  APL2.0
URL: https://git.ovh.iot/redpesk/redpesk-addons/secure-storage-binding
Source: %{name}-%{version}.tar.gz

BuildRequires:  afm-rpm-macros
BuildRequires:  cmake
BuildRequires:  gcc-c++
BuildRequires:  afb-cmake-modules
BuildRequires:  pkgconfig(json-c)
BuildRequires:  pkgconfig(lua) >= 5.3
BuildRequires:  pkgconfig(afb-binding)
BuildRequires:  pkgconfig(afb-libhelpers)
BuildRequires:  pkgconfig(afb-libcontroller)
BuildRequires:  pkgconfig(libsystemd) >= 222


%if 0%{?suse_version}
BuildRequires:  libdb-4_8-devel
%else
BuildRequires:  libdb-devel
%endif

Requires:       afb-binder

%description
This binding provide a database API with key/value semantics.
The backend is currently a Berkeley DB.

%prep
%autosetup -p 1

%build
%afm_configure_cmake
%afm_build_cmake

%install
export NO_BRP_STRIP_DEBUG="true"
%afm_makeinstall


%check

%clean

%files
%afm_files

%afm_package_test


%%changelog



