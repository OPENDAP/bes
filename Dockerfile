# Dockerfile for bes_rhel images

ARG BASE_IMAGE
FROM ${BASE_IMAGE}
RUN yum update -y \
   && dnf install sudo -y

ENV USER="bes_user"
ENV USER_ID=101

RUN useradd \
    # --system \
    --user-group \
    # --shell /sbin/nologin \
    --comment "BES daemon" \
    --uid ${USER_ID} \
    # --home-dir /var/log/bes \
    $USER \
    && echo $USER ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USER
USER $USER

# Temporary home for pulling in the bes repo; we'll be
# deleting it at the end, so it doesn't really matter where we are
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

COPY . ./bes
RUN sudo chown -R $USER:$USER bes $PREFIX \
    && sudo chmod o+x /root
WORKDIR bes

RUN pwd && ls

RUN autoreconf -fiv

RUN ./configure --disable-dependency-tracking \
    --with-dependencies="${PREFIX}/deps" \
    --prefix="${PREFIX}" \
    $GDAL_OPTION \
    --with-build=$BES_BUILD_NUMBER \
    --enable-developer
RUN make -j16

RUN make install

# # TODO: move into own layer
RUN make check -j16

# RUN besctl start && make check -j16 && besctl stop

# # 5. Add besd service to start at boot
# RUN cp ${PREFIX}/etc/rc.d/init.d/besd /etc/rc.d/init.d/besd \
#     && chkconfig --add besd \
#     && ldconfig

# # Clean up
# WORKDIR ".."
# RUN cp bes/bes_VERSION bes_VERSION 
#     # && rm -rf bes

# # Sanity check....
# RUN echo "besdaemon is here: "`which besdaemon`
# RUN echo "BES_VERSION (from bes_VERSION) is $(cat bes_VERSION)"

# CMD ["-"]
# ENTRYPOINT besctl start