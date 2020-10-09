# From the QBDL root dir:
# docker build -t quarkslab/qbdl --target runner -f ./docker/qbdl.dockerfile .
FROM ubuntu:20.04 as builder

RUN apt-get update &&                         \
    DEBIAN_FRONTEND=noninteractive apt-get install -y \
      ninja-build                             \
      unzip                                   \
      wget                                    \
      apt-transport-https                     \
      software-properties-common              \
      build-essential                         \
      libffi-dev                              \
      libssl-dev openssl                      \
      zlib1g-dev                              \
      cmake                                   \
      python3                                 \
      python3-pip                             \
      git \
    && wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key|apt-key add - \
    && echo "deb http://apt.llvm.org/focal/ llvm-toolchain-focal-11 main" >>/etc/apt/sources.list \
    && apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y clang-11 \
    && rm -rf /var/lib/apt/lists/*            \
    && apt-get clean

RUN python3 -m pip install --no-cache-dir --upgrade pip setuptools

ENV CC  clang-11
ENV CXX clang++-11

# Install a specific version of miasm
RUN cd /tmp && \
    git clone https://github.com/cea-sec/miasm/ && cd miasm && \
		git checkout 381d370b73b980190e9a24753a595b043193bec1 && \
		pip install -r requirements.txt && python3 ./setup.py install && \
		rm -rf /tmp/miasm

# TODO: installation of the LIEF python binding is very dirty, but properly
# fixing this requires some re-engineering on the LIEF part.
# Indeed, we need both QBDL and LIEF python bindings to link on the very same
# LIEF static library.
RUN cd /tmp && \
    wget https://github.com/lief-project/LIEF/archive/0.11.3.tar.gz && \
    tar xf 0.11.3.tar.gz && rm 0.11.3.tar.gz && cd LIEF-0.11.3 && \
    mkdir build && cd build && \
    cmake -DLIEF_C_API=NO -DLIEF_LOGGING=OFF -DLIEF_LOGGING_DEBUG=OFF -DLIEF_ENABLE_JSON=OFF -DLIEF_OAT=OFF -DLIEF_DEX=OFF -DLIEF_VDEX=OFF -DLIEF_ART=OFF -G Ninja .. && \
    ninja install && cp api/python/lief.so /usr/local/lib/python3.8/dist-packages && \
    rm -rf /tmp/LIEF-0.11.3

COPY . /src

RUN cd /src && mkdir -p build_docker && cd build_docker && \
    cmake -GNinja -DCMAKE_BUILD_TYPE=Release .. && \
    ninja

RUN cd /src/bindings/python && \
    python3 ./setup.py --ninja install

FROM ubuntu:focal as runner

RUN apt-get update &&                         \
    DEBIAN_FRONTEND=noninteractive apt-get install -y \
      python3                                 \
      python3-distutils                       \
      libpython3.8                            \
    && rm -rf /var/lib/apt/lists/*            \
    && apt-get clean

COPY --from=builder /usr/local/lib/python3.8 /usr/local/lib/python3.8/

RUN mkdir -p /qbdl

# TODO(romain): Use a "make install"
COPY --from=builder /src/build_docker/tools/macho_run/macho_run /qbdl/
COPY --from=builder /src/build_docker/tools/elf_run/elf_run /qbdl/
COPY --from=builder /src/bindings/python/examples /qbdl/
COPY --from=builder /src/examples /qbdl/

WORKDIR /qbdl
