
%define bescachedir %{_localstatedir}/cache/%{name}
%define bespkidir %{_sysconfdir}/pki/%{name}
%define beslogdir %{_localstatedir}/log/%{name}
%define bespiddir %{_localstatedir}/run/%{name}
%define besuser %{name}
%define besgroup %{name}
%define beslibdir %{_libdir}/bes
%define bessharedir %{_datadir}/bes
%define hyraxsharedir %{_datadir}/hyrax

Name:           bes
Version:        3.17.1
Release:        1%{?dist}
Summary:        Back-end server software framework for OPeNDAP

Group:          System Environment/Libraries
License:        LGPLv2+
URL:            http://www.opendap.org/download/BES.html
Source0:        http://www.opendap.org/pub/source/bes-%{version}.tar.gz

BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

Requires:       libdap >= 3.18.0
Requires:       readline bzip2 zlib
Requires:	netcdf >= 4.1
Requires:	libicu >= 3.6
Requires:	hdf5 => 1.8
Requires:	hdf >= 4.2
Requires:       libxml2 >= 2.7.0
Requires:       openssl
# gdal >= 1.10
# gridfields >= ?
# fits >= ?

Requires(pre): shadow-utils

BuildRequires:  libdap-devel >= 3.18.0
BuildRequires:  readline-devel
BuildRequires:  bzip2-devel zlib-devel
BuildRequires:  libxml2-devel >= 2.7.0
BuildRequires:	netcdf-devel >= 4.1
BuildRequires:	libicu-devel >= 3.6
BuildRequires:	hdf5-devel => 1.8
BuildRequires:	hdf-devel >= 4.2
BuildRequires:  openssl-devel
BuildRequires:  pkgconfig
# gdal-devel >= 1.10
# gridfields-devel >= ?
# fits-devel >= ?

%description
BES is a high-performance back-end server software framework for 
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
Requires:       libdap-devel >= 3.15.0
# for the /usr/share/aclocal directory ownership
Requires:       automake
Requires:       openssl-devel, bzip2-devel, zlib-devel
Requires:       pkgconfig

%description    devel
The %{name}-devel package contains libraries and header files for
developing applications that use %{name}.

# %package doc
# Summary: Documentation of the OPeNDAP BES
# Group: Documentation

# %description doc
# Documentation of OPeNDAP BES.

%prep
%setup -q
chmod a-x dispatch/BESStreamResponseHandler*

%build
%configure --disable-static --disable-dependency-tracking
make %{?_smp_mflags}

# make docs
# rm -rf __distribution_docs
# cp -pr docs __distribution_docs
# mv __distribution_docs/html __distribution_docs/api-html
# # .map and .md5 files are of dubious use
# rm __distribution_docs/api-html/*.map
# rm __distribution_docs/api-html/*.md5
# chmod a-x __distribution_docs/BES_*.doc

sed -i.dist -e 's:=/tmp:=%{bescachedir}:' \
  -e 's:=.*/bes.log:=%{beslogdir}/bes.log:' \
  -e 's:=.*/lib/bes:=%{beslibdir}:' \
  -e 's:=.*/share/bes:=%{bessharedir}:' \
  -e 's:=.*/share/hyrax:=%{hyraxsharedir}:' \
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
mkdir -p $RPM_BUILD_ROOT%{bescachedir}
chmod g+w $RPM_BUILD_ROOT%{bescachedir}
mkdir -p $RPM_BUILD_ROOT%{bespkidir}/{cacerts,public}
mkdir -p $RPM_BUILD_ROOT%{beslogdir}
chmod g+w $RPM_BUILD_ROOT%{beslogdir}
mkdir -p $RPM_BUILD_ROOT%{bespiddir}
chmod g+w $RPM_BUILD_ROOT%{bespiddir}
mv $RPM_BUILD_ROOT%{_bindir}/bes-config-pkgconfig $RPM_BUILD_ROOT%{_bindir}/bes-config

%clean
rm -rf $RPM_BUILD_ROOT

%pre
getent group %{besgroup} >/dev/null || groupadd -r %{besgroup}
getent passwd %{besuser} >/dev/null || \
useradd -r -g %{besuser} -d %{beslogdir} -s /sbin/nologin \
    -c "BES daemon" %{besuser}
exit 0

%post 
/sbin/chkconfig --add besd
/sbin/ldconfig

%preun
/sbin/chkconfig --del besd

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%doc ChangeLog NEWS README
%dir %{_sysconfdir}/bes/
%dir %{_sysconfdir}/bes/modules

%config(noreplace) %{_sysconfdir}/bes/bes.conf
%config(noreplace) %{_sysconfdir}/bes/modules/*.conf
# %config(noreplace) %{_sysconfdir}/bes/modules/functions.conf
# %config(noreplace) %{_sysconfdir}/bes/modules/csv.conf
# %config(noreplace) %{_sysconfdir}/bes/modules/dap-server.conf
# %config(noreplace) %{_sysconfdir}/bes/modules/dapreader.conf
# %config(noreplace) %{_sysconfdir}/bes/modules/ff.conf
# %config(noreplace) %{_sysconfdir}/bes/modules/fojson.conf
# %config(noreplace) %{_sysconfdir}/bes/modules/fonc.conf
# %config(noreplace) %{_sysconfdir}/bes/modules/gateway.conf
# %config(noreplace) %{_sysconfdir}/bes/modules/h4.conf
# %config(noreplace) %{_sysconfdir}/bes/modules/h5.conf
# %config(noreplace) %{_sysconfdir}/bes/modules/nc.conf
# %config(noreplace) %{_sysconfdir}/bes/modules/ncml.conf
# %config(noreplace) %{_sysconfdir}/bes/modules/w10n.conf
# %config(noreplace) %{_sysconfdir}/bes/modules/xml_data_handler.conf

%dir %{_datadir}/bes/
%{_datadir}/bes/*.html
%{_datadir}/bes/*.txt
%{_datadir}/bes/*.xml

%{_datadir}/hyrax/

%{_bindir}/beslistener
%{_bindir}/besdaemon
# %{_bindir}/besd # moved to /etc/rc.d/init.d; see below.
%{_bindir}/besstandalone
%{_bindir}/besctl
%{_bindir}/hyraxctl
%{_bindir}/bescmdln

%{_libdir}/*.so.*
%{_libdir}/bes/

%{_initddir}/besd

# %{bescachedir}
%{bespkidir}/

%attr (-,%{besuser},%{besgroup}) %{beslogdir}
%attr (-,%{besuser},%{besgroup}) %{bespiddir}
%attr (-,%{besuser},%{besgroup}) %{bescachedir}

%files devel
%defattr(-,root,root,-)
# %doc __distribution_docs/BES_*.doc
%{_bindir}/besCreateModule
%{_bindir}/bes-config
%{_includedir}/bes/
%{_libdir}/*.so
%{_libdir}/pkgconfig/bes_*.pc
%{_datadir}/bes/templates/
%{_datadir}/aclocal/bes.m4

# %files doc
# %defattr(-,root,root,-)
# %doc __distribution_docs/api-html/

%changelog
* Thu Apr  2 2015 EC2 James Gallagher <jgallagher@opendap.org> - 3.13.2-1
- Added install of 'besd' script to /etc/rc.d/init.d 

* Tue Sep 14 2010 Patrick West <westp@rpi.edu> - 3.9.0-1
- Update

* Tue May 04 2010 Patrick West <westp@rpi.edu> - 3.8.3-1
- Update

* Tue Apr 06 2010 Patrick West <westp@rpi.edu> - 3.8.2-1
- Update

* Thu Mar 11 2010 Patrick West <westp@rpi.edu> - 3.8.1-1
- Update

* Tue Feb 02 2010 Patrick West <westp@rpi.edu> - 3.8.0-1
- Update

* Thu Jan 29 2009 James Gallagher <jgallagher@opendap.org> - 3.7.0-1
- Update

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
