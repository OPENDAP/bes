Summary:         Basic request handlig for DAP servers 
Name:            dap-server
Version:         3.5.1
Release:         1
License:         GPL
Group:           System Environment/Daemons 
Source0:         http://www.opendap.org/pub/3.5/source/%{name}-%{version}.tar.gz
URL:             http://www.opendap.org/

BuildRoot:       %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires:   perl curl libdap-devel >= 3.5.2
Requires:        curl webserver

%description
This is base software for our workhorse server. Written using the DAP++ C++ 
library and Perl, this handles processing compressed files and arranging for 
the correct server module to process the file. The base software also 
provides support for the ASCII response and HTML data-request form. Use this 
in combination with one or more of the format-specific handlers.

%prep 
%setup -q

%build
%configure

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%{_bindir}/dap_usage
%{_bindir}/dap_asciival
%{_bindir}/dap_www_int
%{_datadir}/dap-server/
%{_datadir}/dap-server-cgi/
%doc COPYING COPYRIGHT_URI EXAMPLE_DODS_STATISTICS  NEWS
%doc README README.ascii README-security README.www_int

%changelog
* Fri Sep  2 2005 Patrice Dumas <dumas@centre-cired.fr> 3.5.1-1
- initial release
