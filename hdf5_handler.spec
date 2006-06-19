Summary:         HDF5 data handler for the OPeNDAP Data server
Name:            hdf5_handler
Version:         1.1.0
Release:         1
License:         LGPL
Group:           System Environment/Daemons 
Source0:         ftp://ftp.unidata.ucar.edu/pub/opendap/source/%{name}-%{version}.tar.gz
URL:             http://www.opendap.org/

BuildRoot:       %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
#BuildRequires:   libdap-devel >= 3.7.0 hdf-devel

%description
This is the hdf5 data handler for our data server. It reads HDF5
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
%{_bindir}/dap_h5_handler
%doc COPYING NEWS README

%changelog
* Mon Jun 19 2006 James Gallagher <jimg@zoe.opendap.org> - 
- Initial build.

