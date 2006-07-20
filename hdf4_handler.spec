Summary:         HDF4 data handler for the OPeNDAP Data server
Name:            hdf4_handler
Version:         3.7.1
Release:         1
License:         LGPL
Group:           System Environment/Daemons 
Source0:         ftp://ftp.unidata.ucar.edu/pub/opendap/source/%{name}-%{version}.tar.gz
URL:             http://www.opendap.org/

BuildRoot:       %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires:   libdap-devel >= 3.7.0 hdf-devel
#Requires:        dap-server

%description
This is the hdf4 data handler for our data server. It reads HDF4 and HDF-EOS
files and returns DAP responses that are compatible with DAP2 and the
dap-server software.

%prep 
%setup -q

%build
%configure
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%{_bindir}/dap_hdf4_handler
%doc COPYING COPYRIGHT_URI NEWS
%doc README

%changelog
* Fri Mar  3 2006 Patrice Dumas <pertusus at free.fr> - 3.6.0-1
- Update to 3.6.0

* Thu Sep 21 2005 James Gallagher <jgallagher@opendap.org> 3.5.0-1
- initial release
