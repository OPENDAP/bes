Summary:         ugrid functions handler for the OPeNDAP Data server
Name:            ugrid_functions
Version:         1.0.1
Release:         1
License:         LGPLv2+
Group:           System Environment/Daemons 
Source0:         http://www.opendap.org/pub/source/%{name}-%{version}.tar.gz
URL:             http://www.opendap.org/
Requires:        libdap >= 3.11.0
Requires:        bes >= 3.9.0

BuildRoot:       %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires:   libdap-devel >= 3.11.0
BuildRequires:   bes-devel >= 3.9.0

%description
This is the ugrid (Unstructured Grid or irregular mesh) subsetting 
function handler for Hyrax. This Hyrax server function will subset a
compliant ugrid mesh and return a mesh that is also compliant. The
subset can be specified using latitude, longitude and time. 

%prep 
%setup -q

%build
%configure --disable-static --disable-dependency-tracking
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install INSTALL="install -p"

rm $RPM_BUILD_ROOT%{_libdir}/bes/libugrid_functions.la

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%dir %{_sysconfdir}/bes/
%dir %{_sysconfdir}/bes/modules
%config(noreplace) %{_sysconfdir}/bes/modules/ugrid_functions.conf
%{_libdir}/bes/libugrid_functions.so
%{_datadir}/hyrax/
%doc COPYING NEWS README

%changelog
