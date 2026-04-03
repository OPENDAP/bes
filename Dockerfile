# Dockerfile for bes_core images

# This Dockerfile is intended to build a base image that will be used to build
# subsequent images for our production BES/Hyrax images. The build process is
# split into two stages, with the first stage building the BES and the second
# stage copying over the built BES and its dependencies to a slimmer base image.

ARG BUILDER_BASE_IMAGE
ARG FINAL_BASE_IMAGE
FROM ${BUILDER_BASE_IMAGE:-"rockylinux:8"} AS builder

# Sanity check that the required build argument is provided and non-empty, evn
# though a default value is provided above. We want to enforce that the value is
# always specified.
ARG BUILDER_BASE_IMAGE
RUN if [ -z "$BUILDER_BASE_IMAGE" ]; then \
        echo "Error: Non-empty BUILDER_BASE_IMAGE must be specified. Exiting."; \
        exit 1; \
    fi

ENV BES_USER="bes_user"
ENV USER_ID=101

RUN yum update -y \
    && dnf install sudo -y \
    && dnf clean all

RUN useradd \
        --user-group \
        --comment "BES daemon" \
        --uid ${USER_ID} \
        $BES_USER \
    && echo $BES_USER ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$BES_USER
USER $BES_USER
WORKDIR "/home/$BES_USER"

# Start bes build process
ARG GDAL_OPTION
ARG BES_BUILD_NUMBER
ENV PREFIX="/"
ENV DEPS_PREFIX="/root/install"
ENV PATH="$PREFIX/bin:$DEPS_PREFIX/deps/bin:$PATH"

ENV CPPFLAGS="-I/usr/include/tirpc"
ENV LDFLAGS="-ltirpc"
ENV LD_LIBRARY_PATH="$DEPS_PREFIX/deps/lib"

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

RUN sudo chown -R $BES_USER:$BES_USER $DEPS_PREFIX \
    && sudo chmod o+x /root

# Build the BES
COPY . ./bes
RUN sudo chown -R $BES_USER:$BES_USER bes
WORKDIR bes

RUN autoreconf -fiv
RUN echo "Sanity check: CPPFLAGS=$CPPFLAGS LDFLAGS=$LDFLAGS prefix=$PREFIX" \
    && ./configure --disable-dependency-tracking \
    --with-dependencies="$DEPS_PREFIX/deps" \
    --prefix="$PREFIX" \
    $GDAL_OPTION \
    --with-build=$BES_BUILD_NUMBER
RUN make -j$(nproc --ignore=1)
RUN sudo make install

# Clean up extraneous files; do it in this stage so we don't pull them over
# at the next stage
RUN sudo rm $PREFIX/lib/bes/*.a \
    && sudo rm $PREFIX/lib/bes/*.la

# Test time! We need the besdaemon to be running while we do this, so that
# we hit all the tests. In order to run the daemon, we need to update some
# permissions.
# First, support user $BES_USER running the daemon...
RUN sudo setfacl -R -m u:$BES_USER:rwx $PREFIX/var \
    && sudo setfacl -R -m u:$BES_USER:rwx $PREFIX/run \
    && sudo chown -R $BES_USER:$BES_USER $PREFIX/share/mds \
    && sudo sed -i.dist \
    -e 's:=user_name:='"$BES_USER"':' \
    -e 's:=group_name:='"$BES_USER"':' \
    /etc/bes/bes.conf \
    && sudo touch "/var/bes.log" \
    && sudo chown -R $BES_USER:$BES_USER "/var/bes.log" \
    && echo "okay, ready to run tests"

# ...next, the daemon has to be started as root.
RUN sudo -s --preserve-env=PATH besctl start

# ...now run the tests.
ARG DIST
ENV DIST=${DIST:-el8}
RUN if [ "$DIST" == "el9" ]; then \
        echo "# Warning: Skipping make check because of undiagnosed el9 errors; ref TODO-ISSUE-LINK"; \
    else \
        make check -j$(nproc --ignore=1) \
    fi

# ...and turn off the besdaemon. We want to turn this on/off regardless of
# whether we run the tests
RUN sudo -s --preserve-env=PATH besctl stop

RUN cat libdap4-snapshot | cut -d ' ' -f 1 | sed 's/libdap4-//' > libdap_VERSION

#####
##### Final layer: libdap + hyrax-dependencies + bes
#####
FROM ${FINAL_BASE_IMAGE:-rockylinux:8} AS bes_core

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
    && dnf install sudo which procps libicu acl chkconfig -y \
    && echo "Installing libdap snapshot rpms: $LIBDAP_RPM_FILENAME" \
    && dnf -y install "/tmp_mounted/$LIBDAP_RPM_FILENAME" \
    && dnf clean all

ENV BES_USER="bes_user"
ENV USER_ID=101
ENV PREFIX="/"
ENV DEPS_PREFIX="/root/install"
ENV PATH="$PREFIX/bin:$DEPS_PREFIX/deps/bin:$PATH"

RUN useradd \
        --user-group \
        --comment "BES daemon" \
        --uid ${USER_ID} \
        $BES_USER \
    && echo $BES_USER ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$BES_USER

# Install the latest hyrax dependencies
ARG HYRAX_DEPENDENCIES_TARBALL
RUN --mount=from=aws_downloads,target=/tmp_mounted \
    sudo tar -C "/root" -xzvf "/tmp_mounted/$HYRAX_DEPENDENCIES_TARBALL"

RUN sudo chown -R $BES_USER:$BES_USER $DEPS_PREFIX \
    && sudo chmod o+x /root

USER $BES_USER
WORKDIR "/home/$BES_USER"

COPY --from=builder /home/$BES_USER/bes/bes_VERSION bes_VERSION
COPY --from=builder /home/$BES_USER/bes/libdap_VERSION libdap_VERSION
COPY --from=builder $DEPS_PREFIX $DEPS_PREFIX

# Copy over everything installed in the builder image.
# This is a little ham-fisted, but seems to be at least sufficient
# (if not particularly elegant!).
COPY --from=builder /etc/bes /etc/bes
COPY --from=builder /usr/lib /usr/lib
COPY --from=builder /run/bes /run/bes
COPY --from=builder /share/bes /usr/share/bes
COPY --from=builder /share/hyrax /usr/share/hyrax
COPY --from=builder /include/bes /include/bes
COPY --from=builder /etc/rc.d/init.d/besd /etc/rc.d/init.d/besd
COPY --from=builder /bin/bes* /bin
# NB: Last line of multi-file docker copy is destination
COPY --from=builder \
    /usr/bin/bes* \
    /usr/bin/*dmrpp* \
    /usr/bin/ingest* \
    /usr/bin/dap-config \
    /usr/bin/dmr_memory_cache \
    /usr/bin/get_hdf_side_car \
    /usr/bin/getdap \
    /usr/bin/getdap4 \
    /usr/bin/hyraxctl \
    /usr/bin/localBesGetDap \
    /usr/bin/populateMDS \
    /usr/bin/reduce_mdf \
    /usr/bin/

RUN sudo setfacl -R -m u:$BES_USER:rwx /var/run \
    && sudo setfacl -R -m u:$BES_USER:rwx /run \
    && sudo setfacl -R -m u:$BES_USER:rwx /usr/share

################################################################
# Set up besdaemon

USER root

# Adapted from bes/spec.all_static.in in RPM creation.
# The four *.pem substitutions may be unnecessary, as those *.pem files may be
# vestigial substitutions for a build process past. See HYRAX-2075.
RUN sed -i.dist \
    -e 's:=.*/bes.log:=/var/log/bes/bes.log:' \
    -e 's:=.*/lib/bes:=/usr/lib/bes:' \
    -e 's:=.*/share/bes:=/usr/share/bes:' \
    -e 's:=.*/share/hyrax:=/usr/share/hyrax:' \
    -e 's:=/full/path/to/serverside/certificate/file.pem:=/etc/pki/bes/cacerts/file.pem:' \
    -e 's:=/full/path/to/serverside/key/file.pem:=/etc/pki/bes/public/file.pem:' \
    -e 's:=/full/path/to/clientside/certificate/file.pem:=/etc/pki/bes/cacerts/file.pem:' \
    -e 's:=/full/path/to/clientside/key/file.pem:=/etc/pki/bes/public/file.pem:' \
    -e 's:=user_name:='"$BES_USER"':' \
    -e 's:=group_name:='"$BES_USER"':' \
    /etc/bes/bes.conf \
    && mkdir -p "/var/log/bes/" \
    && touch "/var/log/bes/bes.log" \
    && chown -R $BES_USER:$BES_USER "/var/log/bes/"

# Start besd service at boot
RUN chkconfig --add besd \
    && ldconfig \
    && chkconfig --list | grep besd

# Confirm that the besd service starts at boot
RUN echo "besdaemon is here: $(which besdaemon)" \
    && echo "whoami: $(whoami)" \
    && BESD_COUNT=$(chkconfig --list | grep besd) \
    && if [ -z "$BESD_COUNT" ]; then \
        echo "Error: besd service not configured to run on startup. Exiting."; \
        exit 1; \
    fi

# Sanity-check versions, and that the besctl can be started and stopped without failing
RUN echo "BES_VERSION (from bes_VERSION) is $(cat bes_VERSION)" \
    && echo "LIBDAP_VERSION (from libdap_VERSION) is $(cat libdap_VERSION)" \
    && besctl start \
    && besctl stop

ENTRYPOINT [ "/bin/bash" ]

CMD ["-"]
