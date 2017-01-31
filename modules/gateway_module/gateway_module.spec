Summary:         Gateway module for the OPeNDAP Data server
Name:            gateway_module
Version:         1.1.2
Release:         2
License:         LGPLv2+
Group:           System Environment/Daemons 
Source0:         http://www.opendap.org/pub/source/%{name}-%{version}.tar.gz
URL:             http://www.opendap.org/
Requires:        libdap >= 3.13.3
Requires:        bes >= 3.13.2

BuildRoot:       %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires:   libdap-devel >= 3.13.3
BuildRequires:   bes-devel >= 3.13.2

%description
This is the Gateway module for our data server that allows a remote URL
to be passed as a container to the BES, have that remote URL accessed,
and served locally.

%prep 
%setup -q

%build
%configure --disable-static --disable-dependency-tracking
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install INSTALL="install -p"

rm $RPM_BUILD_ROOT%{_libdir}/bes/libgateway_module.la

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%dir %{_sysconfdir}/bes/
%dir %{_sysconfdir}/bes/modules
%config(noreplace) %{_sysconfdir}/bes/modules/gateway.conf
%{_libdir}/bes/libgateway_module.so
%doc COPYING COPYRIGHT NEWS README

%changelog
* Sat Jan 29 2011 Patrick West <westp@rpi.edu> 0.0.1-1
- initial release
