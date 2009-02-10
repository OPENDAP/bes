%define pptcapicachedir %{_localstatedir}/cache/%{name}
%define pptcapipkidir %{_sysconfdir}/pki/%{name}
%define pptcapilogdir %{_localstatedir}/log/%{name}
%define pptcapiuser %{name}
%define pptcapigroup %{name}

Name:           pptcapi
Version:        1.0.3
Release:        1%{?dist}
Summary:        OPeNDAP Back-end server PPT C API

Group:          System Environment/Libraries
License:        LGPLv2+
URL:            http://www.opendap.org/download/PPTCAPI.html
Source0:        http://www.opendap.org/pub/source/pptcapi-%{version}.tar.gz

BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  pkgconfig
BuildRequires:  doxygen graphviz
Requires(pre): shadow-utils

%description
The BES PPT C API is a C API for communicating with an OPeNDAP Back-End
Server (BES). PPT stands for Point to Point Transport.

%package        devel
Summary:        Development files for %{name}
Group:          Development/Libraries
Requires:       %{name} = %{version}-%{release}
# for the /usr/share/aclocal directory ownership
Requires:       automake
Requires:       pkgconfig

%description    devel
The %{name}-devel package contains libraries and header files for
developing applications that use %{name}.

%package doc
Summary: Documentation of the OPeNDAP BES PPT C API
Group: Documentation

%description doc
Documentation of OPeNDAP BES PPT C API.


%prep
%setup -q

%build
%configure --disable-static --disable-dependency-tracking
make %{?_smp_mflags}

make docs
rm -rf __distribution_docs
cp -pr docs __distribution_docs
mv __distribution_docs/html __distribution_docs/api-html
# .map and .md5 files are of dubious use
rm __distribution_docs/api-html/*.map
rm __distribution_docs/api-html/*.md5

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
find $RPM_BUILD_ROOT -name '*.la' -exec rm -f {} ';'
mkdir -p $RPM_BUILD_ROOT%{pptcapicachedir}
mkdir -p $RPM_BUILD_ROOT%{pptcapipkidir}/{cacerts,public}
mkdir -p $RPM_BUILD_ROOT%{pptcapilogdir}
mv $RPM_BUILD_ROOT%{_bindir}/pptcapi-config-pkgconfig $RPM_BUILD_ROOT%{_bindir}/pptcapi-config

%clean
rm -rf $RPM_BUILD_ROOT


%pre
exit 0


%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%defattr(-,root,root,-)
%doc ChangeLog NEWS README
%{_libdir}/*.so.*
%{bescachedir}
%{bespkidir}/

%files devel
%defattr(-,root,root,-)
%{_bindir}/pptcapi-config
%{_includedir}/pptcapi/
%{_libdir}/*.so
%{_libdir}/pkgconfig/bes_*.pc
%{_datadir}/aclocal/pptcapi.m4

%files doc
%defattr(-,root,root,-)
%doc __distribution_docs/api-html/

%changelog
* Tue Feb 10 2009 Patrick West <westp@rpi.edu> - 1.0.3-1
- Initial packaging

