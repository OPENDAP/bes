Name:           bes
Version:        3.5.1
Release:        1%{?dist}
Summary:        Example Data Handler for BES

Group:          System Environment/Libraries
License:        LGPL
URL:            http://www.opendap.org/download/BES.html
#Source0:        ftp://ftp.unidata.ucar.edu/pub/opendap/source/bes-%{version}.tar.gz
Source0:        http://www.opendap.org/pub/source/bes-%{version}.tar.gz

BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  libdap-devel >= 3.7.7

%description
This CSV Data Handler is an example data handler module for the BES.
BES is a new, high-performance back-end server software framework for 
OPeNDAP that allows data providers more flexibility in providing end 
users views of their data. The current OPeNDAP data objects (DAS, DDS, 
and DataDDS) are still supported, but now data providers can add new data 
views, provide new functionality, and new features to their end users 
through the BES modular design. Providers can add new data handlers, new 
data objects/views, the ability to define views with constraints and 
aggregation, the ability to add reporting mechanisms, initialization 
hooks, and more.


%package        devel
Summary:        Development files for %{name}
Group:          Development/Libraries
Requires:       %{name} = %{version}-%{release}
Requires:       libdap-devel >= 3.7.7

%description    devel
The %{name}-devel package contains libraries and header files for
developing applications that use %{name}.


%package doc
Summary: Documentation of the OPeNDAP BES CSV Handler Example
Group: Documentation

%description doc
Documentation of OPeNDAP BES CSV Handler Example.


%prep

%build
%configure --disable-static --disable-dependency-tracking
make %{?_smp_mflags}
make docs
rm -rf __fedora_docs
cp -pr docs __fedora_docs
mv __fedora_docs/html __fedora_docs/api-html
# .map and .md5 files are of dubious use
rm __fedora_docs/api-html/*.map
rm __fedora_docs/api-html/*.md5
chmod a-x __fedora_docs/BES_*.doc

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
find $RPM_BUILD_ROOT -name '*.la' -exec rm -f {} ';'
for file in config.guess depcomp missing config.sub install-sh ltmain.sh mkinstalldirs; do
  chmod a+x $RPM_BUILD_ROOT%{_datadir}/bes/templates/conf/$file
done

%clean
rm -rf $RPM_BUILD_ROOT


%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%defattr(-,root,root,-)
%doc ChangeLog NEWS README
%{_libdir}/bes/libnc_module.so
%{_datadir}/hyrax/
%doc COPYING COPYRIGHT NEWS README

%changelog
