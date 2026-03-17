# Dockerfile for bes_core images
ARG BUILDER_BASE_IMAGE
ARG FINAL_BASE_IMAGE
FROM ${BUILDER_BASE_IMAGE:-"rockylinux:8"} AS builder

ARG BUILDER_BASE_IMAGE
RUN if [ -z "$BUILDER_BASE_IMAGE" ]; then \
        echo "Error: Non-empty BUILDER_BASE_IMAGE must be specified. Exiting."; \
        exit 1; \
    fi

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
ENV PREFIX="/"
ENV DEPS_PREFIX="/root/install"
ENV PATH="$PREFIX/bin:$DEPS_PREFIX/deps/bin:$PATH"

ENV CPPFLAGS="-I/usr/include/tirpc"
ENV LDFLAGS="-ltirpc"
ENV LD_LIBRARY_PATH="${DEPS_PREFIX}/deps/lib"

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

RUN sudo chown -R $USER:$USER $DEPS_PREFIX \
    && sudo chmod o+x /root

# Build the BES
COPY . ./bes
RUN sudo chown -R $USER:$USER bes
WORKDIR bes

RUN autoreconf -fiv
RUN echo "Sanity check: CPPFLAGS=$CPPFLAGS LDFLAGS=$LDFLAGS prefix=$PREFIX" \
    && ./configure --disable-dependency-tracking \
    --with-dependencies="${DEPS_PREFIX}/deps" \
    --prefix="${PREFIX}" \
    $GDAL_OPTION \
    --with-build=$BES_BUILD_NUMBER \
    --enable-developer
RUN make -j$(nproc --ignore=1)
RUN sudo make install

# # Clean up extraneous files; do it in this stage so we don't pull them over
# # at the next stage
RUN sudo rm ${PREFIX}/lib/bes/*.a \
    && sudo rm ${PREFIX}/lib/bes/*.la

# # Update permissions to support user \"${USER}\" running the daemon"
RUN sudo setfacl -R -m u:${USER}:rwx ${PREFIX}/var \
    && sudo setfacl -R -m u:${USER}:rwx ${PREFIX}/run \
    && sudo setfacl -R -m u:${USER}:rwx ${PREFIX}/share

# Test the BES
RUN besctl start && make check -j$(nproc --ignore=1) && besctl stop

RUN cat libdap4-snapshot | cut -d ' ' -f 1 | sed 's/libdap4-//' > libdap_VERSION

# # #####
# # ##### Final layer: libdap + hyrax-dependencies + bes
# # #####
FROM ${FINAL_BASE_IMAGE:-rockylinux:8} AS bes_image

ARG FINAL_BASE_IMAGE
RUN if [ -z "$FINAL_BASE_IMAGE" ]; then \
        echo "Error: Non-empty FINAL_BASE_IMAGE must be specified. Exiting."; \
        exit 1; \
    fi

# Duplicated from installation above, this time on a slimmer base image...
# Install the libdap rpms
ARG LIBDAP_RPM_FILENAME
RUN --mount=from=aws_downloads,target=/tmp_mounted \
    yum update -y \
    && dnf install sudo which procps libicu -y \
    && echo "Installing libdap snapshot rpms: $LIBDAP_RPM_FILENAME" \
    && dnf -y install "/tmp_mounted/$LIBDAP_RPM_FILENAME" \
    && dnf clean all

ENV USER="bes_user"
ENV USER_ID=101
ENV PREFIX="/"
ENV DEPS_PREFIX="/root/install"
ENV PATH="$PREFIX/bin:$DEPS_PREFIX/deps/bin:$PATH"

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

RUN sudo chown -R $USER:$USER $DEPS_PREFIX \
    && sudo chmod o+x /root

USER $USER
WORKDIR "/home/$USER"

COPY --from=builder /home/${USER}/bes/bes_VERSION bes_VERSION
COPY --from=builder /home/${USER}/bes/libdap_VERSION libdap_VERSION
COPY --from=builder $DEPS_PREFIX $DEPS_PREFIX
COPY --from=builder /etc/bes /etc/bes
COPY --from=builder /usr/lib/bes /usr/lib/bes
COPY --from=builder /run/bes /run/bes
COPY --from=builder /share/bes /share/bes
COPY --from=builder /include/bes /include/bes

# Sanity check....
RUN echo "besdaemon is here: "`which besdaemon` \
    && echo "BES_VERSION (from bes_VERSION) is $(cat bes_VERSION)" \
    && echo "LIBDAP_VERSION (from libdap_VERSION) is $(cat libdap_VERSION)" \
    && besctl start \
    && besctl stop

ENTRYPOINT [ "/bin/bash" ]

CMD ["-"]
