Summary:         FITS data handler for the OPeNDAP Data server
Name:            fits_handler
Version:         1.0.11
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
This is the fits data handler for our data server. It reads fits
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

rm $RPM_BUILD_ROOT%{_libdir}/bes/libfits_module.la

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%dir %{_sysconfdir}/bes/
%dir %{_sysconfdir}/bes/modules
%config(noreplace) %{_sysconfdir}/bes/modules/fits.conf
%{_libdir}/bes/libfits_module.so
%{_datadir}/hyrax/

%doc COPYING COPYRIGHT NEWS README

%changelog
* Mon Jan 09 2012 Patrick West <westp@rpi.edu> 1.0.6-2
- Release 1.0.6

* Mon Jun 30 2011 Patrick West <pwest@ucar.edu> 1.0.6-1
- Release 1.0.6

* Mon Mar 03 2008 Patrick West <pwest@ucar.edu> 1.0.3-1
- Release 1.0.3

* Wed Jan 10 2007 Patrick West <pwest@ucar.edu> 1.0.1-1
- initial release

