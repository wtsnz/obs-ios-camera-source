%{!?python_sitearch: %global python_sitearch %(%{__python} -c "from distutils.sysconfig import get_python_lib; print(get_python_lib(1))")}

Name:          libusbmuxd
Version:       1.1.0.build
Release:       0%{?dist}
Summary:       Library for the USB multiplexor daemon for iPhone and iPod Touch devices

Group:         System Environment/Libraries
License:       LGPLv2+
URL:           http://www.libimobiledevice.org/
Source0:       http://www.libimobiledevice.org/downloads/%{name}-%{version}.tar.gz

BuildRequires: swig
BuildRequires: gcc-c++
BuildRequires: libplist-devel

%description
libusbmuxd is a library for the USB multiplexor daemon for iPhone and iPod Touch

%package devel
Summary: Development package for libusbmuxd
Group: Development/Libraries
Requires: libusbmuxd = %{version}-%{release}
Requires: pkgconfig

%description devel
%{name}, development headers and libraries.

%prep
%setup -q

%build
./autogen.sh --prefix=/usr --without-cython --libdir=/usr/lib64 --enable-static=no --enable-shared=yes
make

%install
make DESTDIR=$RPM_BUILD_ROOT install
rm $RPM_BUILD_ROOT/usr/lib64/*.la

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%doc AUTHORS COPYING README
%{_bindir}/iproxy
%{_libdir}/libusbmuxd.so.*

%files devel
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/libusbmuxd.pc
%{_libdir}/libusbmuxd.so
%{_includedir}/*.h

%changelog
* Fri Apr 21 2017 Frederik Carlier <frederik.carlier@quamotion.mobi> - 1.1.0-4
- Initial package

