Summary:         Basic request handling for OPeNDAP servers 
Name:            dap-server
Version:         4.1.6
Release:         1
License:         LGPL
Group:           System Environment/Daemons 
Source0:         http://www.opendap.org/pub/source/%{name}-%{version}.tar.gz
URL:             http://www.opendap.org/
Requires:        libdap >= 3.13.3
Requires:        bes >= 3.13.2

BuildRoot:       %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:   libdap-devel >= 3.13.3
BuildRequires:   bes-devel >= 3.13.2

%description
This package contains general purpose handlers for use with the new
Hyrax data server. These are the Usage, ASCII and HTML form handlers.
Each takes input from a 'data handler' and returns a HTML or plain text
response --- something other than a DAP response object.

%prep 
%setup -q

%build
%configure --disable-dependency-tracking --disable-static
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install INSTALL='install -p'

rm $RPM_BUILD_ROOT%{_libdir}/bes/libascii_module.la
rm $RPM_BUILD_ROOT%{_libdir}/bes/libusage_module.la
rm $RPM_BUILD_ROOT%{_libdir}/bes/libwww_module.la

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%dir %{_sysconfdir}/bes
%dir %{_sysconfdir}/bes/modules
%config(noreplace) %{_sysconfdir}/bes/modules/dap-server.conf
%{_datadir}/bes/
%{_libdir}/bes/libascii_module.so
%{_libdir}/bes/libusage_module.so
%{_libdir}/bes/libwww_module.so
%doc COPYING COPYRIGHT_URI NEWS README

%changelog
* Thu Jan 29 2009 James Gallagher <jimg@zoe.opendap.org> - 3.9.0-1
- Updated

* Mon Mar 3 2008 Patrick West <patrick@ucar.edu> 3.8.5-1
- Update for 3.8.5; General fixes

* Wed Dec 3 2007 James Gallagher <jgallagher@opendap.org> 3.7.3-1
- Update for 3.8.4; General fixes

* Wed Nov 14 2007 James Gallagher <jgallagher@opendap.org> 3.7.3-1
- Update for 3.8.3; General fixes

* Wed Feb 14 2007 James Gallagher <jgallagher@opendap.org> 3.7.3-1
- Update for 3.7.3; includes adding BES modules

* Wed Sep 20 2006 Patrice Dumas <dumas@centre-cired.fr> 3.7.1-1
- update to 3.7.1

* Fri Mar  3 2006 Patrice Dumas <dumas@centre-cired.fr> 3.6.0-1
- new release

* Wed Feb 22 2006 Ignacio Vazquez-Abrams <ivazquez@ivazquez.net> 3.5.3-1.2
- Small fix for Perl provides

* Thu Feb  2 2006 Patrice Dumas <dumas@centre-cired.fr> 3.5.3-1
- add a cgi subpackage

* Fri Sep  2 2005 Patrice Dumas <dumas@centre-cired.fr> 3.5.1-1
- initial release
