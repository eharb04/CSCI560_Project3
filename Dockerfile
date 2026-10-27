# CSCI 560 - Project 3: Traceroute
# Matches the course's Ubuntu 20.04 LTS grading environment.
FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    g++ \
    gdb \
    make \
    cmake \
    git \
    curl \
    wget \
    nano \
    net-tools \
    iproute2 \
    iputils-ping \
    traceroute \
    tcpdump \
    netcat-openbsd \
    pkg-config \
    valgrind \
    strace \
    ltrace \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Mount the source directory into /work at runtime so edits persist on the host.
WORKDIR /work

CMD ["/bin/bash"]
