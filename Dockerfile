###############################################################################################
#
# Dockerfile for bes image
#
#
# Some shell state reference:
# set -f # "set -o noglob"  Disable file name generation using metacharacters (globbing).
# set -v # "set -o verbose" Prints shell input lines as they are read.
# set -x # "set -o xtrace"  Print command traces before executing command.
# set -e #  Exit on error.
#
# In general use "set -e" when running commands that matter and don't use
# it for debugging stuff.
#

#####
##### Stage 1. All dependencies
#####
FROM opendap/rocky8_hyrax_builder:latest AS deps-base
RUN yum update -y

ARG PREFIX
ENV PREFIX="${PREFIX:-"/root/install"}"

ENV PATH="$PREFIX/bin:$PREFIX/deps/bin:$PATH"

# ###############################################################
# # Install the latest hyrax dependencies
ARG HYRAX_DEPENDENCIES_TARBALL
RUN --mount=from=dependencies,target=/tmp \
    tar -C "$HOME" -xzvf "/tmp/$HYRAX_DEPENDENCIES_TARBALL"

# ###############################################################
# # Install the libdap rpms
ARG LIBDAP_RPM_FILENAME
ARG LIBDAP_DEVEL_RPM_FILENAME
RUN --mount=from=dependencies,target=/tmp \
    echo "Installing libdap snapshot rpms: $LIBDAP_RPM_FILENAME, $LIBDAP_DEVEL_RPM_FILENAME" \
    && dnf -y install "/tmp/$LIBDAP_RPM_FILENAME" \
    && dnf -y install "/tmp/$LIBDAP_DEVEL_RPM_FILENAME"

# To debug what has been installed, use    
# rpm -ql "$PREFIX/rpmbuild/${LIBDAP_RPM_FILENAME}"

#####
##### Stage 2. Make and install the bes
#####
# FROM deps-base AS bes-builder

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

RUN make -j16

RUN make install --dry-run



# #####
# ##### Stage 3: Only keep the built dependencies
# #####
# FROM deps-base
# COPY --from=bes-builder ${PREFIX} ${PREFIX}

# RUN echo "besdaemon is here: "`which besdaemon`

# CMD ["-"]

