README for Debian build of BES
==============================

# Using make:
To generate a debian package using make type the command:

```
    make deb
```

# Using the command line:
To  generate the debian package using command line commands:

```
    make dist
    mv bes-<PACKAGE-PACKAGE_VERSION>.tar.gz ..
    cd ..
    mv bes-<PACKAGE-PACKAGE_VERSION>.tar.gz bes_<PACKAGE-PACKAGE_VERSION>.orig.tar.gz
    tar xzf bes_<PACKAGE-PACKAGE_VERSION>.orig.tar.gz
    cd bes-<PACKAGE-PACKAGE_VERSION>/
    cp -r ../bes/debian/ .
    debuild -us -uc
```
