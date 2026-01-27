# Dockerfile for bes_rhel images
ARG BASE_IMAGE
FROM ${BASE_IMAGE} AS base

ENV USER="bes_user"
ENV USER_ID=101

RUN yum update -y \
    && dnf install sudo -y \
    && useradd \
        --user-group \
        --comment "BES daemon" \
        --uid ${USER_ID} \
        $USER \
    && echo $USER ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USER
USER $USER
WORKDIR "/home/$USER"

# Start bes build process
ARG GDAL_OPTION
ARG BES_BUILD_NUMBER
ENV PREFIX="/root/install"
ENV PATH="$PREFIX/bin:$PREFIX/deps/bin:$PATH"

ENV CPPFLAGS="-I/usr/include/tirpc"
ENV LDFLAGS="-ltirpc"
ENV LD_LIBRARY_PATH="${PREFIX}/deps/lib"

# Install the latest hyrax dependencies
ARG HYRAX_DEPENDENCIES_TARBALL
RUN --mount=from=aws_downloads,target=/tmp \
    sudo tar -C "/root" -xzvf "/tmp/$HYRAX_DEPENDENCIES_TARBALL"

# Install the libdap rpms
ARG LIBDAP_RPM_FILENAME
ARG LIBDAP_DEVEL_RPM_FILENAME
RUN --mount=from=aws_downloads,target=/tmp \
    echo "Installing libdap snapshot rpms: $LIBDAP_RPM_FILENAME, $LIBDAP_DEVEL_RPM_FILENAME" \
    && sudo dnf -y install "/tmp/$LIBDAP_RPM_FILENAME" \
    && sudo dnf -y install "/tmp/$LIBDAP_DEVEL_RPM_FILENAME"
# To debug what has been installed, use
# rpm -ql "$PREFIX/rpmbuild/${LIBDAP_RPM_FILENAME}"

RUN sudo chown -R $USER:$USER $PREFIX \
    && sudo chmod o+x /root

#####
##### Build layer: Includes large bes repo that we don't want in final image
#####
FROM base AS builder

# Build the BES
COPY . ./bes
RUN sudo chown -R $USER:$USER bes
WORKDIR bes

RUN autoreconf -fiv
RUN echo "Sanity check: CPPFLAGS=$CPPFLAGS LDFLAGS=$LDFLAGS prefix=$PREFIX" \
    && ./configure --disable-dependency-tracking \
    --with-dependencies="${PREFIX}/deps" \
    --prefix="${PREFIX}" \
    $GDAL_OPTION \
    --with-build=$BES_BUILD_NUMBER \
    --enable-developer
RUN make install -j16

# Test the BES
RUN besctl start && make check -j16 && besctl stop

RUN pwd

#####
##### Final layer: Base + installed bes components
#####
FROM base

COPY --from=builder /home/${USER}/bes/bes_VERSION bes_VERSION
COPY --from=builder $PREFIX $PREFIX

# Sanity check....
RUN echo "besdaemon is here: "`which besdaemon` \
    && echo "BES_VERSION (from bes_VERSION) is $(cat bes_VERSION)" \
    && besctl start \
    && besctl stop

CMD ["-"]
