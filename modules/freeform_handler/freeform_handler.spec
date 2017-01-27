Summary:         FreeForm data handler for the OPeNDAP Data server
Name:            freeform_handler
Version:         3.8.8
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
This is the freeform data handler for our data server. It reads ASCII,
binary and DB4 files which have been described using FreeForm and returns DAP
responses that are compatible with DAP2 and the dap-server software. This
package contains the OPeNDAP 4 Data Server (aka Hyrax) modules.

%prep 
%setup -q

%build
%configure --disable-static --disable-dependency-tracking
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install INSTALL="install -p"
rm $RPM_BUILD_ROOT%{_libdir}/bes/libff_module.la

# I made this a convenience library. jhrg 3/25/10
# rm $RPM_BUILD_ROOT%{_libdir}/libfreeform.la
# rm $RPM_BUILD_ROOT%{_libdir}/libfreeform.so

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%dir %{_sysconfdir}/bes/modules
%config(noreplace) %{_sysconfdir}/bes/modules/ff.conf
%{_libdir}/bes/libff_module.so
%{_datadir}/hyrax/
%doc COPYING COPYRIGHT NEWS README

# %{_libdir}/libfreeform.so.*
# I made this a 'convenience' library, in part, to solve problems with 
# distcheck on OS/X. jhrg 3/24/10

%changelog
* Thu Jan 29 2009 James Gallagher <jimg@zoe.opendap.org> - 3.7.10-1
- Updated

* Thu Jun 26 2008 Patrick West <patrick@ucar.edu> 3.7.9-1
- Update to 3.7.9

* Mon Mar 03 2008 Patrick West <patrick@ucar.edu> 3.7.8-1
- Update to 3.7.8

* Mon Mar 26 2006 James Gallagher <jgallagher@opendap.org> 3.6.1-1
- Update to 3.6.1: Mac OS/X fixes.

* Mon Feb 27 2006 James Gallagher <jgallagher@opendap.org> 3.6.0-1
- Update to 3.6.0

* Thu Sep 21 2005 James Gallagher <jgallagher@opendap.org> 3.5.0-1
- initial release
