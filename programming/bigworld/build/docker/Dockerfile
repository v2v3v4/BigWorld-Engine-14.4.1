# syntax=docker/dockerfile:1
# Assumes the context is the root of the BWT source tree, e.g.,
# docker buildx build . -f programming/bigworld/build/docker/Dockerfile -t <sometag>

# Build the bigworld-devel RPM, which depends on the things we need to build BWT.
FROM centos:7 as devel_rpm_build
RUN --mount=type=cache,target=/var/cache/yum yum -y install make rpm-build
# rpm-build or the BigWorld scripts refuse to run as root. Sensibly.
RUN groupadd bigworld
RUN useradd bigworld -g bigworld
WORKDIR /home/bigworld
USER bigworld
COPY --chown=bigworld:bigworld . ./bwrpm/
WORKDIR ./bwrpm
# RUN make -C game/tools/bigworld/server/rpm bigworld-devel

# Install the things we need to build BWT
FROM centos:7 as builder

# Not needed yet.
# RUN --mount=type=cache,target=/var/cache/yum yum -y install epel-release
# RUN --mount=type=cache,target=/var/cache/yum yum -y install centos-release-scl-rh

# RPM's

# RUN --mount=type=bind,target=/root/bigworld-devel-14.4.1.el7.x86_64.rpm,from=devel_rpm_build,source=/home/bigworld/bwrpm/game/tools/bigworld/server/rpm/binary_rpms/bigworld-devel-14.4.1.el7.x86_64.rpm \
#     --mount=type=cache,target=/var/cache/yum \
#     yum -y install /root/bigworld-devel-14.4.1.el7.x86_64.rpm

# `which` is not depended-upon by bigworld-devel, but is used by the build scripts.
 RUN --mount=type=cache,target=/var/cache/yum yum -y install mc which dos2unix
 RUN --mount=type=cache,target=/var/cache/yum yum -y install redhat-lsb-core
 RUN --mount=type=cache,target=/var/cache/yum yum -y install make gcc gcc-c++
 RUN --mount=type=cache,target=/var/cache/yum yum -y install rpm-build
 RUN --mount=type=cache,target=/var/cache/yum yum -y install mariadb-devel python-devel 
 RUN --mount=type=cache,target=/var/cache/yum yum -y install sqlite-devel readline-devel
 RUN --mount=type=cache,target=/var/cache/yum yum -y install gdbm-devel bzip2-devel
 RUN --mount=type=cache,target=/var/cache/yum yum -y install ncurses-devel binutils-devel

# Expected run command is something like
# $ docker run --rm -it -u $(id -u):$(id -g) --mount type=bind,source=${PWD},target=/home/bigworld/host --mount type=tmpfs,target=/home/bigworld/rpmbuild --env HOME=/home/bigworld -w /home/bigworld/host <sometag>
# All build output from Make should go into game/bin/server/el7/
# However, intermediate files end up in programming/bigworld/build/el7
# And some directories will be created under programming/bigworld/third_party/python_modules

# To build the BWT Server binaries (and any other interesting utilities on the way)
# $ make -s -j64 -rR -C programming
# (May need to run it twice, sometimes it gets confused and stops early due to Python build-system output confusion)
# TODO: This might actually be a patching failure, for the Pympler and oursql patches.

# Run all the unit tests:
# $ make -s -j64 -rR -C programming bw-run-all-unit-tests
# (Use bw-run-unit-tests to stop on first failure)

# Build a couple of infrastructure RPMs in to run-up a cluster.
# Output is in game/tools/bigworld/server/rpm/binary_rpms
# These are consumed by the cluster_node Dockerfile and Vagrantfile
# $ make -s -j64 -rR -C game/tools/bigworld/server/rpm bigworld-bwmachined bigworld-tools bigworld-server
