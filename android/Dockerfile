FROM ubuntu:focal-20220105

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update

RUN apt-get install -y \
            git \
            python \
            curl \
            wget \
            lsb-release \
            sudo \
            openjdk-8-jdk-headless \
            xz-utils

# Add user
ARG USERNAME=webrtc-build
ARG UID=1000
ARG GID=1000
RUN groupadd --gid $GID --non-unique $USERNAME
RUN useradd --create-home --uid $UID --gid $GID --non-unique --shell /bin/bash $USERNAME
RUN usermod --append --groups sudo $USERNAME
RUN echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

RUN git clone --depth 1 https://chromium.googlesource.com/chromium/tools/depot_tools.git /opt/depot_tools && \
    chown -R $UID:$GID /opt/depot_tools
ENV PATH="/opt/depot_tools:${PATH}"

RUN mkdir /webrtc && chown $UID:$GID /webrtc
WORKDIR /webrtc

ADD ./container-script.sh container-script.sh

USER $USERNAME

RUN fetch --help > /dev/null && \
    gclient --help > /dev/null

ENTRYPOINT bash -c "./container-script.sh"
