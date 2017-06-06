Summary: Provides w10n navigation and data retrieval for Hyrax
Name: w10n_handler
Version: 0.0.1
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
This is the w10n module for Hyrax - the OPeNDAP data
server. With this handler a server can easily be configured to 
provide w10n catalog navigation and data retrieval.

%prep
%setup -q

%build
%configure --disable-static --disable-dependency-tracking
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install INSTALL="install -p"

rm $RPM_BUILD_ROOT%{_libdir}/bes/libw10n_handler.la

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%dir %{_sysconfdir}/bes/
%dir %{_sysconfdir}/bes/modules
%config(noreplace) %{_sysconfdir}/bes/modules/w10n.conf
%{_libdir}/bes/libfonc_module.so
%doc COPYING COPYRIGHT NEWS README


