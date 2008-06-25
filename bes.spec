%define bescachedir %{_localstatedir}/cache/%{name}
%define bespkidir %{_sysconfdir}/pki/%{name}
%define beslogdir %{_localstatedir}/log/%{name}
%define besuser %{name}
%define besgroup %{name}

Name:           bes
Version:        3.6.2
Release:        1%{?dist}
Summary:        Back-end server software framework for OPeNDAP

Group:          System Environment/Libraries
License:        LGPLv2+
URL:            http://www.opendap.org/download/BES.html
Source0:        http://www.opendap.org/pub/source/bes-%{version}.tar.gz

BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  libdap-devel >= 3.8.0
BuildRequires:  readline-devel
BuildRequires:  bzip2-devel zlib-devel
# needed by ppt
BuildRequires:  openssl-devel
BuildRequires:  pkgconfig
BuildRequires:  doxygen graphviz
Requires(pre): shadow-utils

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
Requires:       libdap-devel >= 3.8.0
# for the /usr/share/aclocal directory ownership
Requires:       automake
Requires:       openssl-devel, bzip2-devel, zlib-devel
Requires:       pkgconfig

%description    devel
The %{name}-devel package contains libraries and header files for
developing applications that use %{name}.


%package doc
Summary: Documentation of the OPeNDAP BES
Group: Documentation

%description doc
Documentation of OPeNDAP BES.


%prep
%setup -q
chmod a-x dispatch/BESStreamResponseHandler*

%build
%configure --disable-static --disable-dependency-tracking
make %{?_smp_mflags}

make docs
rm -rf __distribution_docs
cp -pr docs __distribution_docs
mv __distribution_docs/html __distribution_docs/api-html
# .map and .md5 files are of dubious use
rm __distribution_docs/api-html/*.map
rm __distribution_docs/api-html/*.md5
chmod a-x __distribution_docs/BES_*.doc

sed -i.dist -e 's:=/tmp:=%{bescachedir}:' \
  -e 's:=.*/bes.log:=%{beslogdir}/bes.log:' \
  -e 's:=/full/path/to/serverside/certificate/file.pem:=%{bespkidir}/cacerts/file.pem:' \
  -e 's:=/full/path/to/serverside/key/file.pem:=%{bespkidir}/public/file.pem:' \
  -e 's:=/full/path/to/clientside/certificate/file.pem:=%{bespkidir}/cacerts/file.pem:' \
  -e 's:=/full/path/to/clientside/key/file.pem:=%{bespkidir}/public/file.pem:' \
  -e 's:=user_name:=%{besuser}:' \
  -e 's:=group_name:=%{besgroup}:' \
  dispatch/bes/bes.conf

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
find $RPM_BUILD_ROOT -name '*.la' -exec rm -f {} ';'
for file in config.guess depcomp missing config.sub install-sh ltmain.sh mkinstalldirs; do
  chmod a+x $RPM_BUILD_ROOT%{_datadir}/bes/templates/conf/$file
done
mkdir -p $RPM_BUILD_ROOT%{bescachedir}
mkdir -p $RPM_BUILD_ROOT%{bespkidir}/{cacerts,public}
mkdir -p $RPM_BUILD_ROOT%{beslogdir}
mv $RPM_BUILD_ROOT%{_bindir}/bes-config-pkgconfig $RPM_BUILD_ROOT%{_bindir}/bes-config

%clean
rm -rf $RPM_BUILD_ROOT


%pre
getent group %{besgroup} >/dev/null || groupadd -r %{besgroup}
getent passwd %{besuser} >/dev/null || \
useradd -r -g %{besuser} -d %{beslogdir} -s /sbin/nologin \
    -c "BES daemon" %{besuser}
exit 0


%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%defattr(-,root,root,-)
%doc ChangeLog NEWS README
%dir %{_sysconfdir}/bes/
%config(noreplace) %{_sysconfdir}/bes/bes.conf
%dir %{_datadir}/bes/
%{_datadir}/bes/*.html
%{_datadir}/bes/*.txt
%{_bindir}/beslistener
%{_bindir}/besdaemon
%{_bindir}/besstandalone
%{_bindir}/besctl
%{_bindir}/hyraxctl
%{_bindir}/besregtest
%{_bindir}/bescmdln
%{_libdir}/*.so.*
%{_libdir}/bes/
%{bescachedir}
%{bespkidir}/
%attr (-,%{besuser},%{besgroup}) %{beslogdir}

%files devel
%defattr(-,root,root,-)
%doc __distribution_docs/BES_*.doc
%{_bindir}/besCreateModule
%{_bindir}/bes-config
%{_includedir}/bes/
%{_libdir}/*.so
%{_libdir}/pkgconfig/bes_*.pc
%{_datadir}/bes/templates/
%{_datadir}/aclocal/bes.m4

%files doc
%defattr(-,root,root,-)
%doc __distribution_docs/api-html/

%changelog
* Wed Jun 25 2008 Patrick West <pwest@ucar.edu> 3.6.2-1
- Update.

* Fri Apr 11 2008 Patrick West <pwest@ucar.edu> 3.6.1-1
- Update.

* Fri Feb 29 2008 Patrick West <pwest@ucar.edu> 3.6.0-1
- Update.

* Tue Feb 13 2007 James Gallagher <jgallagher@opendap.org> 3.4.0-1
- Update.

* Sat Jul 22 2006 Patrice Dumas <pertusus@free.fr> 3.2.0-1
- initial packaging
