Summary: Return a GeoTiff, JP2k, etc., File for a DAP Data response
Name: fileout_gdal
Version: 0.9.4
Release: 1
License: LGPLv2+
Group: System Environment/Daemons
URL: http://www.opendap.org/
Source0: http://www.opendap.org/pub/source/%{name}-%{version}.tar.gz
Requires:        libdap >= 3.13.0
Requires:        bes >= 3.13.0
Requires:        gdal >= 1.10

BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires:   libdap-devel >= 3.13.0
BuildRequires:   bes-devel >= 3.13.0
BuildRequires:   gdal-devel >= 1.10

%description
This is the fileout GDAL response handler for Hyrax - the OPeNDAP data
server. With this handler a server can easily be configured to return
data packaged in a GeoTiff, JP2, etc., file.

%prep
%setup -q

%build
%configure --disable-static --disable-dependency-tracking
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install INSTALL="install -p"

rm $RPM_BUILD_ROOT%{_libdir}/bes/libfong_module.la

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%dir %{_sysconfdir}/bes/
%dir %{_sysconfdir}/bes/modules
%config(noreplace) %{_sysconfdir}/bes/modules/fong.conf
%{_libdir}/bes/libfong_module.so
%doc COPYING NEWS README

%changelog
* Tue Nov 20 2012 James Gallagher <jgallagher@opendap.org> - 
- Initial build.

