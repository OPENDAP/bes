Summary:         HDF4 File Content Map Writer
Name:            h4mapwriter
Version:         1.0.7
Release:         1
License:         GPL
Group:           Development/Tools
Source0:         http://www.hdfgroup.org/ftp/pub/outgoing/h4map/src/%{name}-%{version}.tar.gz
URL:             http://www.hdfgroup.org/

BuildRoot:       %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

%description
This is an XML map file generation tool for HDF4 files. With the offset and length information provided in the  XML map file, users can use generic POSIX file fread() function to retreive data from the HDF4 file.

%prep 
%setup -q

%build
%configure CC=h4cc
make 

%install
rm -rf $RPM_BUILD_ROOT
make -i DESTDIR=$RPM_BUILD_ROOT install INSTALL="install -p"


%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%doc %{_mandir}/man1/h4mapwriter.1.gz

%changelog
* Thu Nov 01 2014 Hyo-Kyung Joe Lee <hyoklee@hdfgroup.org> - 1.0.7-1
- Maintenance release for testing with HDF4 version 4.2.10.

* Thu Nov 01 2013 Hyo-Kyung Joe Lee <hyoklee@hdfgroup.org> - 1.0.6-1
- Maintenance release with improved error handling.

* Thu May 01 2013 Hyo-Kyung Joe Lee <hyoklee@hdfgroup.org> - 1.0.5-1
- Maintenance release for testing with HDF4 version 4.2.9.

* Thu Nov 01 2012 Hyo-Kyung Joe Lee <hyoklee@hdfgroup.org> - 1.0.4-1
- Bug fix release for handling palettes in old OCTS Level 3 products.

* Tue May 01 2012 Hyo-Kyung Joe Lee <hyoklee@hdfgroup.org> - 1.0.3-1
- Bug fix release

* Tue Nov 01 2011 Hyo-Kyung Joe Lee <hyoklee@hdfgroup.org> - 1.0.2-1
- Bug fix release

* Tue Sep 01 2011 Hyo-Kyung Joe Lee <hyoklee@hdfgroup.org> - 1.0.1-1
- Bug fix release

* Sun Jul 01 2011 Hyo-Kyung Joe Lee <hyoklee@hdfgroup.org> - 1.0.0-1
- Official release

* Sun May 22 2011 Hyo-Kyung Joe Lee <hyoklee@hdfgroup.org> - 0.9.1-1
- Beta release

* Fri Oct 29 2009 Hyo-Kyung Joe Lee <hyoklee@hdfgroup.org> - 0.0.1-1
- Initial release

