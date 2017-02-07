Summary: Return a NetCDF File for a DAP Data response
Name: fileout_netcdf
Version: 1.3.0
Release: 1
License: LGPLv2+
Group: System Environment/Daemons
URL: http://www.opendap.org/
Source0: http://www.opendap.org/pub/source/%{name}-%{version}.tar.gz
Requires: libdap >= 3.14.0
Requires: netcdf >= 4.1.0
Requires: bes >= 3.14.0

BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires:   libdap-devel >= 3.14.0
BuildRequires:   netcdf-devel >= 4.1.0
BuildRequires:   bes-devel >= 3.14.0

%description
This is the fileout netCDF response handler for Hyrax - the OPeNDAP data
server. With this handler a server can easily be configured to return
data packaged in a netCDF 3 file.

%prep
%setup -q

%build
%configure --disable-static --disable-dependency-tracking
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install INSTALL="install -p"

rm $RPM_BUILD_ROOT%{_libdir}/bes/libfonc_module.la

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%dir %{_sysconfdir}/bes/
%dir %{_sysconfdir}/bes/modules
%config(noreplace) %{_sysconfdir}/bes/modules/fonc.conf
%{_libdir}/bes/libfonc_module.so
%doc COPYING COPYRIGHT NEWS README

%changelog
* Sat May  1 2010 Patrick West <westp@rpi.edu> - 1.0.1-1
- Update to 1.0.1

* Tue Feb  2 2010 Patrick West <westp@rpi.edu> - 1.0.0-1
- Update to 1.0.0

* Mon Mar 16 2009 James Gallagher <jgallagher@opendap.org> - 
- Initial build.

