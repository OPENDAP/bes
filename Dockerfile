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

# ###############################################################
# # Install the latest hyrax dependencies
ARG HYRAX_DEPENDENCIES_TARBALL
RUN --mount=from=dependencies,target=/tmp \
    tar -xzvf /tmp/${HYRAX_DEPENDENCIES_TARBALL}

# ###############################################################
# # Install the libdap rpm
ARG LIBDAP_RPM_FILENAME 
RUN --mount=from=dependencies,target=/tmp \
    echo "Installing libdap snapshot rpm. ${LIBDAP_RPM_FILENAME}" \
    && dnf -y install /tmp/${LIBDAP_RPM_FILENAME}

#####
##### Stage 2. Install the bes
#####
FROM deps-base AS bes-builder

ARG CPPFLAGS
ARG LDFLAGS
ARG prefix 

# RUN autoreconf -fiv
COPY . bes/
WORKDIR /bes 

RUN autoreconf -fiv

# TODO: configure 
# TODO: make install

# RUN echo "besdaemon is here: "`which besdaemon`

CMD ["-"]

