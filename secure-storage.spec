Name: secure-storage-binding
Version: 2.0.0
Release: 1%{?dist}
Summary: Binding provide a database API with key/value semantics
License:  APL2.0
URL: https://git.ovh.iot/redpesk/redpesk-addons/secure-storage-binding
Source: %{name}-%{version}.tar.gz

%global _afmappdir %{_prefix}/redpesk

BuildRequires:  cmake
BuildRequires:  gcc-c++
BuildRequires:  afb-cmake-modules
BuildRequires:  pkgconfig(json-c)
BuildRequires:  pkgconfig(lua) >= 5.3
BuildRequires:  pkgconfig(afb-binding)
BuildRequires:  pkgconfig(afb-libhelpers)
BuildRequires:  pkgconfig(afb-libcontroller)
BuildRequires:  pkgconfig(libsystemd) >= 222
BuildRequires:  pkgconfig(afb-helpers4)


%if 0%{?suse_version}
BuildRequires:  libdb-4_8-devel
%else
BuildRequires:  libdb-devel
%endif

Requires:       afb-binder

%description
This binding provide a database API with key/value semantics.
The backend is currently a Berkeley DB.

%package redtest
Summary: redtest package
Requires: %{name} = %{version}-%{release}

%description redtest
secure-storage-binding redtest

%prep
%autosetup -p 1

%build
%cmake -DAFM_APP_DIR=%{_afmappdir} .
%cmake_build

%install
%cmake_install

%files
%defattr(-,root,root)
%dir %{_afmappdir}/%{name}
%{_afmappdir}

%files redtest
%{_libexecdir}/redtest/%{name}/run-redtest
%{_libexecdir}/redtest/%{name}/py-test.py

%%changelog







