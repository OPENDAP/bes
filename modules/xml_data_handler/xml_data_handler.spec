Summary:         Basic request handling for OPeNDAP servers 
Name:            xml_data_handler
Version:         1.0.5
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
This package contains a general purpose handler for use with the Hyrax
data server. This handler takes input from a 'data handler' and
returns XML document that encodes both dataset metadata and values. It
is intended to be used for small data requests and web systems that 
need data in XML documents..

%prep 
%setup -q

%build
%configure --disable-dependency-tracking --disable-static
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install INSTALL='install -p'

rm $RPM_BUILD_ROOT%{_libdir}/bes/libxml_data_module.la

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%dir %{_sysconfdir}/bes
%dir %{_sysconfdir}/bes/modules
%config(noreplace) %{_sysconfdir}/bes/modules/xml_data_handler.conf
%{_libdir}/bes/libxml_data_module.so
%doc COPYING NEWS README

%changelog

