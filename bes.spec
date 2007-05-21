Name:           bes
Version:        3.5.1
Release:        1%{?dist}
Summary:        Back-end server software framework for OPeNDAP

Group:          System Environment/Libraries
License:        BSD-like
URL:            http://www.opendap.org/download/BES.html
Source0:        ftp://ftp.unidata.ucar.edu/pub/opendap/source/bes-%{version}.tar.gz

BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  libdap-devel >= 3.7.4
BuildRequires:  doxygen

%description
BES is a new, high-performance back-end server software framework for 
OPeNDAP that allows data providers more flexibility in providing end 
users views of their data. The current OPeNDAP data objects (DAS, DDS, 
and DataDDS) are still supported, but now data providers can add new data 
views, provide new functionality, and new features to their end users 
through the BES modular design. Providers can add new data handlers, new 
data objects/views, the ability to define views with constraints and 
aggregation, the ability to add reporting mechanisms, initialization 
hooks, and more.


%package        devel
Summary:        Development files for %{name}
Group:          Development/Libraries
Requires:       %{name} = %{version}-%{release}
Requires:       libdap-devel >= 3.7.4

%description    devel
The %{name}-devel package contains libraries and header files for
developing applications that use %{name}.


%prep
%setup -q

%build
%configure --disable-static --disable-dependency-tracking
make %{?_smp_mflags}
make docs
cp -pr docs/html api-html

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
find $RPM_BUILD_ROOT -name '*.la' -exec rm -f {} ';'


%clean
rm -rf $RPM_BUILD_ROOT


%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%defattr(-,root,root,-)
%doc ChangeLog NEWS README
%dir %{_sysconfdir}/bes/
%config(noreplace) %{_sysconfdir}/bes/bes.conf
%{_datadir}/bes/
%{_bindir}/beslistener
%{_bindir}/besdaemon
%{_bindir}/besctl
%{_bindir}/bescmdln
%{_bindir}/besCreateModule
%{_libdir}/*.so.*
%{_libdir}/bes/

%files devel
%defattr(-,root,root,-)
%doc api-html/ docs/BES_PPT.doc docs/BES_Creating_Module.doc
%doc docs/BES_Configuration.doc docs/BES_Server_Architecture.doc
%{_includedir}/bes/
%{_libdir}/*.so
%{_bindir}/bes-config


%changelog
* Tue Feb 13 2007 James Gallagher <jgallagher@opendap.org> 3.4.0-1
- Update.
* Sat Jul 22 2006 Patrice Dumas <pertusus@free.fr> 3.2.0-1
- initial packaging
