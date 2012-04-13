Summary:         HDF4 data handler for the OPeNDAP Data server
Name:            hdf4_handler
Version:         3.9.4
Release:         1
License:         LGPLv2+
Group:           System Environment/Daemons 
Source0:         http://www.opendap.org/pub/source/%{name}-%{version}.tar.gz
URL:             http://www.opendap.org/
Requires:        libdap >= 3.11.0
Requires:        bes >= 3.9.0
# If the handler is built without requiring HDF4 RPM, the following line can be 
# commented out. It won't affect the functionality of hdf4_handler whether
# the older version of HDF4 RPM is installed or not in the system.
# We leave it here to remind users to install the latest HDF4 library
# to make the current hdf4_handler work since we always test it with the 
# latest HDF4 library only.
Requires:      hdf >= 4.2.6

BuildRoot:       %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires:   libdap-devel >= 3.11.0
BuildRequires:   bes-devel >= 3.9.0
# Building the custom handler with HDF-EOS2 option doesn't require HDF4 RPM.
# Instead, we recommend you to install the latest HDF4 library and use
# configure script to specify the path that points to the latest HDF4 library.
#
# Please read the "%configure" line carefully below the "%build" line.
#
# BuildRequires:   hdf-devel >= 4.2.6
# BuildRequires:   hdf >= 4.2.6


%description
This is the hdf4 data handler for our data server. It reads HDF4 and HDF-EOS2
files and returns DAP responses that are compatible with DAP2 and the
dap-server software.

%prep 
%setup -q

%build
# Replace the paths with ones that have your own HDF4 library and HDF-EOS2 
# library installation.
# If you have HDF4 library RPM installed, you may skip specifying the
# "--with-hdf4=/path/to/your/hdf4_library" part.
%configure --disable-dependency-tracking --disable-static --with-hdfeos2=/hdfdap/hyrax-1.8.0 --with-hdf4=/hdfdap/hyrax-1.8.0
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install INSTALL="install -p"

rm $RPM_BUILD_ROOT%{_libdir}/bes/libhdf4_module.la

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%dir %{_sysconfdir}/bes/
%dir %{_sysconfdir}/bes/modules
%config(noreplace) %{_sysconfdir}/bes/modules/h4.conf
%{_libdir}/bes/libhdf4_module.so
%{_datadir}/hyrax/
%doc COPYING COPYRIGHT_URI NEWS README

%changelog
* Fri Jan 13 2011 James Gallagher <jimg@zoe.opendap.org> - 3.9.4-1
- Updated for building custom hdf4_handler with HDF-EOS2 option.

* Thu Jan 29 2009 James Gallagher <jimg@zoe.opendap.org> - 3.7.9-1
- Updated

* Fri Mar  3 2006 Patrice Dumas <pertusus at free.fr> - 3.6.0-1
- Update to 3.6.0

* Thu Sep 21 2005 James Gallagher <jgallagher@opendap.org> 3.5.0-1
- initial release
