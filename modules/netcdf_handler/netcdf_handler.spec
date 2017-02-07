Summary:         NetCDF 3 data handler for the OPeNDAP Data server
Name:            netcdf_handler
Version:         3.10.4
Release:         2
License:         LGPLv2+
Group:           System Environment/Daemons 
Source0:         http://www.opendap.org/pub/source/%{name}-%{version}.tar.gz
URL:             http://www.opendap.org/
Requires:        libdap >= 3.13.3
Requires:        netcdf >= 4.1
Requires:        bes >= 3.13.2

BuildRoot:       %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires:   libdap-devel >= 3.13.3
BuildRequires:   netcdf-devel >= 4.1
BuildRequires:   bes-devel >= 3.13.2

%description
This is the netcdf data handler for our data server. It reads netcdf 3
files and returns DAP responses that are compatible with DAP2 and the
dap-server software.

%prep 
%setup -q

%build
%configure --disable-static --disable-dependency-tracking
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install INSTALL="install -p"

rm $RPM_BUILD_ROOT%{_libdir}/bes/libnc_module.la

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%dir %{_sysconfdir}/bes/
%dir %{_sysconfdir}/bes/modules
%config(noreplace) %{_sysconfdir}/bes/modules/nc.conf
%{_libdir}/bes/libnc_module.so
%{_datadir}/hyrax/
%doc COPYING COPYRIGHT NEWS README

%changelog
* Thu Jan 29 2009 James Gallagher <jimg@zoe.opendap.org> - 3.8.0-1
- Updated

* Tue Mar  4 2008 Patrice Dumas <pertusus at free.fr> - 3.7.9-1
- Update to 3.7.9

* Thu Sep  7 2006 Patrice Dumas <pertusus at free.fr> - 3.7.2-1
- Update to 3.7.2

* Fri Mar  3 2006 Patrice Dumas <pertusus at free.fr> - 3.6.0-1
- Update to 3.6.0

* Wed Feb  1 2006 Patrice Dumas <pertusus at free.fr> - 3.5.2-1
- re-add netcdf-devel

* Wed Nov 16 2005 James Gallagher <jgallagher@opendap.org> 3.5.1-1
- Removed netcdf-devel from BuildRequires. it does, unless you install 
- netcdf some other way.

* Thu Sep 21 2005 James Gallagher <jgallagher@opendap.org> 3.5.0-1
- initial release
