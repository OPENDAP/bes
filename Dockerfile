# Dockerfile for bes_rhel images

ARG BASE_IMAGE
# TODO-check/fail if no base_image provided
FROM ${BASE_IMAGE:-"rockylinux:8"}
RUN yum update -y

ENV PREFIX="/root/install"
ENV PATH="$PREFIX/bin:$PREFIX/deps/bin:$PATH"

# Install the latest hyrax dependencies
ARG HYRAX_DEPENDENCIES_TARBALL
RUN --mount=from=aws_downloads,target=/tmp \
    tar -C "$HOME" -xzvf "/tmp/$HYRAX_DEPENDENCIES_TARBALL"

# Install the libdap rpms
ARG LIBDAP_RPM_FILENAME
ARG LIBDAP_DEVEL_RPM_FILENAME
RUN --mount=from=aws_downloads,target=/tmp \
    echo "Installing libdap snapshot rpms: $LIBDAP_RPM_FILENAME, $LIBDAP_DEVEL_RPM_FILENAME" \
    && dnf -y install "/tmp/$LIBDAP_RPM_FILENAME" \
    && dnf -y install "/tmp/$LIBDAP_DEVEL_RPM_FILENAME"

# To debug what has been installed, use    
# rpm -ql "$PREFIX/rpmbuild/${LIBDAP_RPM_FILENAME}"

COPY . bes/
WORKDIR /bes

ARG CPPFLAGS
ARG LDFLAGS
RUN autoreconf -fiv

ARG GDAL_OPTION
ARG BES_BUILD_NUMBER
# Note that we are NOT setting a special prefix when we run configuration, 
# so when it comes to the installation step we'll be using
# the default system-based installation sites
RUN ./configure --disable-dependency-tracking \
    --with-dependencies="${PREFIX}/deps" \
    --prefix="${PREFIX}" \
    $GDAL_OPTION \
    --with-build=$BES_BUILD_NUMBER

ARG NJOBS_OPTION
ENV NJOBS_OPTION="${NJOBS_OPTION:-""}"
RUN echo "NJOBS_OPTION is '$NJOBS_OPTION'"

RUN make $NJOBS_OPTION

# Do basic installation
RUN make install
RUN echo "besdaemon is here: "`which besdaemon`

# Mimick RPM install configuration, adapted from bes/spec.all_static.in:

# 0. Define rpm macro variables 
ENV bescachedir="/var/cache/bes"
ENV bespkidir="/etc/pki/bes"
ENV beslogdir="/var/log/bes"
ENV bespiddir="/var/run"
ENV _bindir="$PREFIX/bin"
ENV _tmpfilesdir="/usr/lib/tmpfiles.d"
ENV _systconfdir="$PREFIX/etc"
ENV _datadir="/usr/share"
ENV _libdir="$PREFIX/lib"
ENV beslibdir="${_libdir}/bes"

# 1. %build: Update the default macros: https://docs.fedoraproject.org/en-US/packaging-guidelines/RPMMacros/
RUN sed -i.dist -e 's:=/tmp:='"$bescachedir"':' \
    -e 's:=.*/bes.log:='"$beslogdir"'/bes.log:' \
    -e 's:=.*/lib/bes:='"$beslibdir"':' \
    -e 's:=.*/share/bes:='"$_datadir"'/bes:' \
    -e 's:=.*/share/hyrax:='"$_datadir"'/hyrax:' \
    -e 's:=/full/path/to/serverside/certificate/file.pem:='"$bespkidir"'/cacerts/file.pem:' \
    -e 's:=/full/path/to/serverside/key/file.pem:='"$bespkidir"'/public/file.pem:' \
    -e 's:=/full/path/to/clientside/certificate/file.pem:='"$bespkidir"'/cacerts/file.pem:' \
    -e 's:=/full/path/to/clientside/key/file.pem:='"$bespkidir"'/public/file.pem:' \
    -e 's:=user_name:=bes:' \
    -e 's:=group_name:=bes:' \
    "${PREFIX}/etc/bes/bes.conf"

# 2. "%pre"-install: Add bes group and user
RUN getent group bes >/dev/null || groupadd -r bes
RUN getent passwd bes >/dev/null || \
    useradd -r -g bes -d ${beslogdir} -s /sbin/nologin -c "BES daemon" bes

# 3. Create directories (from "%install" section)
RUN mkdir -p ${bescachedir} \
    && chmod g+w ${bescachedir} \
    && mkdir -p ${bespkidir}/{cacerts,public} \
    && mkdir -p ${beslogdir} \
    && chmod g+w ${beslogdir} \
    && mkdir -p ${bespiddir} \
    && chmod g+w ${bespiddir} \
    && mv ${_bindir}/bes-config-pkgconfig ${_bindir}/bes-config \
    && mkdir -p ${_tmpfilesdir} \
    && mv ${_bindir}/bes-tmpfiles-conf ${_tmpfilesdir}/bes.conf \
    && echo "ok"

# # 4. "%files" installed: Add and update owndership for files handled differently in rpm install than make install
RUN mkdir "${_datadir}/hyrax/" "${_datadir}/mds/" \
    && chown -R bes:bes ${beslogdir} ${bescachedir} "${_datadir}/mds/" "${PREFIX}/etc/bes" ${beslibdir} ${_bindir}

# ENV LD_LIBRARY_PATH="${PREFIX}/deps/lib"

# 5. Add besd service to start at boot

RUN cp ${PREFIX}/etc/rc.d/init.d/besd /etc/rc.d/init.d/besd \
    && chkconfig --add besd \
    && ldconfig

# # Clean up
# WORKDIR ".."
# RUN cp bes/bes_VERSION bes_VERSION 
#     # && rm -rf bes

# # Sanity check....
# RUN echo "besdaemon is here: "`which besdaemon`
# RUN echo "BES_VERSION (from bes_VERSION) is $(cat bes_VERSION)"

# CMD ["-"]
# ENTRYPOINT besctl start