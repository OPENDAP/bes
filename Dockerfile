# Dockerfile for bes_rhel8 image

FROM opendap/rocky8_hyrax_builder:latest
RUN yum update -y

ARG PREFIX
ENV PREFIX="${PREFIX:-"/root/install"}"

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
RUN ./configure --disable-dependency-tracking --prefix=${PREFIX} \
    --with-dependencies=${PREFIX}/deps \
    $GDAL_OPTION \
    --with-build=$BES_BUILD_NUMBER

ARG NJOBS_OPTION
ENV NJOBS_OPTION="${NJOBS_OPTION:-""}"
RUN echo "NJOBS_OPTION is '$NJOBS_OPTION'"

RUN make $NJOBS_OPTION

RUN make install
RUN echo "besdaemon is here: "`which besdaemon`

RUN echo `ls`
ENV BES_VERSION="$(`cat bes_VERSION`)"
RUN echo "BES_VERSION is ${BES_VERSION}"

# Clean up
WORKDIR ".."
RUN rm -rf bes

# Sanity check....
RUN echo "besdaemon is here: "`which besdaemon`

CMD ["-"]
