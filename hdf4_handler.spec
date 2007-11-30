Summary:         HDF4 data handler for the OPeNDAP Data server
Name:            hdf4_handler
Version:         3.7.6
Release:         2
License:         LGPL
Group:           System Environment/Daemons 
Source0:         http://www.opendap.org/pub/source/%{name}-%{version}.tar.gz
URL:             http://www.opendap.org/

BuildRoot:       %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires:   libdap-devel >= 3.7.9 hdf-devel
BuildRequires:   bes-devel

%description
This is the hdf4 data handler for our data server. It reads HDF4 and HDF-EOS
files and returns DAP responses that are compatible with DAP2 and the
dap-server software.

%prep 
%setup -q

%build
%configure --disable-dependency-tracking --disable-static
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install INSTALL="install -p"

rm $RPM_BUILD_ROOT%{_libdir}/bes/libhdf4_module.la

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%{_bindir}/dap_hdf4_handler
%{_bindir}/bes-hdf4-data.sh
%{_libdir}/bes/libhdf4_module.so
%{_datadir}/hyrax/
%doc COPYING COPYRIGHT_URI NEWS README

%changelog
* Fri Mar  3 2006 Patrice Dumas <pertusus at free.fr> - 3.6.0-1
- Update to 3.6.0

* Thu Sep 21 2005 James Gallagher <jgallagher@opendap.org> 3.5.0-1
- initial release
