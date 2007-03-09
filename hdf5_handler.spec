Summary:         HDF5 data handler for the OPeNDAP Data server
Name:            hdf5_handler
Version:         1.1.0
Release:         1
License:         LGPL
Group:           System Environment/Daemons 
Source0:         ftp://ftp.unidata.ucar.edu/pub/opendap/source/%{name}-%{version}.tar.gz
URL:             http://www.opendap.org/

BuildRoot:       %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
#BuildRequires:   libdap-devel >= 3.7.4 hdf-devel
#Requires:        bes

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

# pre: commands to run before install; post: commnds run after install;
# preun; postun for commands before and after uninstall

# Only try to configure the bes.conf file if the bes can be found.
%post
if bes-config --version >/dev/null 2>&1
then
	bes_prefix=`bes-config --prefix`
	configure-hdf5-data.sh $bes_prefix/etc/bes/bes.conf $bes_prefix/lib/bes
fi

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%{_bindir}/dap_h5_handler
%{_bindir}/configure-hdf5-data.sh
%{_libdir}/
%{_libdir}/bes/
%{_datadir}/hyrax/data/hdf5
%doc COPYING NEWS README

%changelog
* Mon Jun 19 2006 James Gallagher <jimg@zoe.opendap.org> - 
- Initial build.

