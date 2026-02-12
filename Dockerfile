# Dockerfile for bes_rhel images
ARG BUILDER_BASE_IMAGE
ARG FINAL_BASE_IMAGE
FROM ${BUILDER_BASE_IMAGE} AS builder

ENV USER="bes_user"
ENV USER_ID=101

RUN yum update -y \
    && dnf install sudo -y \
    && dnf clean all

RUN useradd \
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
RUN --mount=from=aws_downloads,target=/tmp_mounted \
    sudo tar -C "/root" -xzvf "/tmp_mounted/$HYRAX_DEPENDENCIES_TARBALL"

# Install the libdap rpms
ARG LIBDAP_RPM_FILENAME
ARG LIBDAP_DEVEL_RPM_FILENAME
RUN --mount=from=aws_downloads,target=/tmp_mounted \
    echo "Installing libdap snapshot rpms: $LIBDAP_RPM_FILENAME, $LIBDAP_DEVEL_RPM_FILENAME" \
    && sudo dnf -y install "/tmp_mounted/$LIBDAP_RPM_FILENAME" \
    && sudo dnf -y install "/tmp_mounted/$LIBDAP_DEVEL_RPM_FILENAME"
# To debug what has been installed, use
# rpm -ql "$PREFIX/rpmbuild/${LIBDAP_RPM_FILENAME}"

RUN sudo chown -R $USER:$USER $PREFIX \
    && sudo chmod o+x /root

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
RUN make install -j$(nproc --ignore=1)

# Test the BES
RUN besctl start && make check -j$(nproc --ignore=1) && besctl stop

#####
##### Final layer: libdap + hyrax-dependencies + bes
#####
FROM ${FINAL_BASE_IMAGE} AS bes_image

# Duplicated from installation above, this time on a slimmer base image...
# Install the libdap rpms
ARG LIBDAP_RPM_FILENAME
ARG LIBDAP_DEVEL_RPM_FILENAME
RUN --mount=from=aws_downloads,target=/tmp_mounted \
    yum update -y \
    && dnf install sudo which procps libicu -y \
    && echo "Installing libdap snapshot rpms: $LIBDAP_RPM_FILENAME, $LIBDAP_DEVEL_RPM_FILENAME" \
    && dnf -y install "/tmp_mounted/$LIBDAP_RPM_FILENAME" \
    && dnf clean all

ENV USER="bes_user"
ENV USER_ID=101
ENV PREFIX="/root/install"
ENV PATH="$PREFIX/bin:$PREFIX/deps/bin:$PATH"

RUN useradd \
        --user-group \
        --comment "BES daemon" \
        --uid ${USER_ID} \
        $USER \
    && echo $USER ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USER

# Install the latest hyrax dependencies
ARG HYRAX_DEPENDENCIES_TARBALL
RUN --mount=from=aws_downloads,target=/tmp_mounted \
    sudo tar -C "/root" -xzvf "/tmp_mounted/$HYRAX_DEPENDENCIES_TARBALL"

RUN sudo chown -R $USER:$USER $PREFIX \
    && sudo chmod o+x /root

USER $USER
WORKDIR "/home/$USER"

COPY --from=builder /home/${USER}/bes/bes_VERSION bes_VERSION
COPY --from=builder $PREFIX $PREFIX

# Sanity check....
RUN echo "besdaemon is here: "`which besdaemon` \
    && echo "BES_VERSION (from bes_VERSION) is $(cat bes_VERSION)" \
    && besctl start \
    && besctl stop

ENTRYPOINT [ "/bin/bash" ]

CMD ["-"]
