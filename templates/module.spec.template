Summary:         OPENDAP_CLASS module for the OPeNDAP Data server
Name:            OPENDAP_TYPE_module
Version:         3.7.9
Release:         2
License:         LGPLv2+
Group:           System Environment/Daemons 
Source0:         http://www.opendap.org/pub/source/%{name}-%{version}.tar.gz
URL:             http://www.opendap.org/

BuildRoot:       %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires:   libdap-devel >= 3.8.0
BuildRequires:   bes-devel >= 3.6.0

%description
This is the OPENDAP_CLASS module for our data server.

%prep 
%setup -q

%build
%configure --disable-static --disable-dependency-tracking
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install INSTALL="install -p"

rm $RPM_BUILD_ROOT%{_libdir}/*.la
rm $RPM_BUILD_ROOT%{_libdir}/*.so
rm $RPM_BUILD_ROOT%{_libdir}/bes/*.la

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_bindir}/bes-OPENDAP_TYPE-data.sh
%{_libdir}/bes/libOPENDAP_TYPE_module.so
%{_datadir}/hyrax/
%doc COPYING COPYRIGHT NEWS README

%changelog
