Summary: Return a JSON File for a DAP Data response
Name: fileout_json
Version: 0.9.1
Release: 1
License: LGPLv2+
Group: System Environment/Daemons
URL: http://www.opendap.org/
Source0: http://www.opendap.org/pub/source/%{name}-%{version}.tar.gz
Requires: libdap >= 3.13.0
Requires: bes >= 3.13.0

BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires:   libdap-devel >= 3.13.1
BuildRequires:   bes-devel >= 3.13.0

%description
This is the fileout JSON response handler for Hyrax - the OPeNDAP data
server. With this handler a server can easily be configured to return
data packaged in JSON file.

%prep
%setup -q

%build
%configure --disable-static --disable-dependency-tracking
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install INSTALL="install -p"

rm $RPM_BUILD_ROOT%{_libdir}/bes/libfojson_module.la

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%dir %{_sysconfdir}/bes/
%dir %{_sysconfdir}/bes/modules
%config(noreplace) %{_sysconfdir}/bes/modules/fojson.conf
%{_libdir}/bes/libfonc_module.so
%doc COPYING COPYRIGHT NEWS README


