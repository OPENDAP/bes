Summary:         GDAL handler for the OPeNDAP Data server
Name:            gdal_handler
Version:         1.0.5
Release:         1
License:         LGPLv2+
Group:           System Environment/Daemons 
Source0:         http://www.opendap.org/pub/source/%{name}-%{version}.tar.gz
URL:             http://www.opendap.org/
Requires:        libdap >= 3.18.0
Requires:        bes >= 3.17.1
Requires:        gdal >= 1.10

BuildRoot:       %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires:   libdap-devel >= 3.18.0
BuildRequires:   bes-devel >= 3.17.1
BuildRequires:   gdal-devel >= 1.10

%description
This is the GDAL handler for our data server. We hope it will serve any
file that can be read using the GDAL library.

%prep 
%setup -q

%build
%configure --disable-static --disable-dependency-tracking
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install INSTALL="install -p"

rm -f $RPM_BUILD_ROOT%{_libdir}/bes/libgdal_module.la

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%dir %{_sysconfdir}/bes/
%dir %{_sysconfdir}/bes/modules
%config(noreplace) %{_sysconfdir}/bes/modules/gdal.conf
%{_libdir}/bes/libgdal_module.so
%{_datadir}/hyrax/
%doc COPYING NEWS README

%changelog
