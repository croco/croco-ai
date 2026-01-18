FROM alpine:3.22 AS base-image
RUN set -xe \
    && apk update \
    && apk add \
        autoconf \
        automake \
        bash \
        cmake \
        curl \
        g++ \
        gcc \
        gdb \
        git \
        lapack-dev \
        linux-headers \
        make \
        openblas-dev \
        sudo \
        xz \
    && adduser -D croco \
    && addgroup croco abuild \
    && echo "croco ALL=(ALL:ALL) NOPASSWD:ALL" >> /etc/sudoers

FROM base-image AS faiss-image
WORKDIR /opt
RUN set -xe \
    && curl -LO https://github.com/facebookresearch/faiss/archive/refs/tags/v1.13.2.tar.gz \
    && tar -xzf v1.13.2.tar.gz \
    && mv faiss-1.13.2 faiss \
    && rm v1.13.2.tar.gz \
    && cd faiss \
    && cmake -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -DFAISS_ENABLE_GPU=OFF \
        -DFAISS_ENABLE_CUVS=OFF \
        -DFAISS_ENABLE_PYTHON=OFF \
        -DBUILD_TESTING=OFF \
        -DBUILD_SHARED_LIBS=ON \
        -DFAISS_ENABLE_C_API=OFF \
        -DFAISS_OPT_LEVEL=generic \
    && cd build \
    && make -j $(nproc) \
    && make install

FROM faiss-image AS runtime-image
COPY --chown=croco:croco src/ /home/croco/faiss/
RUN set -xe \
    && apk update \
    && apk add \
        php84 \
        php84-common \
        php84-dev \
    && cd /usr/bin \
    && ln -s php84 php
WORKDIR /home/croco/faiss
RUN set -xe \
    && cmake -B build \
    && cd build \
    && make -j $(nproc) \
    && make install \
    && echo "extension=croco_faiss" > /etc/php84/conf.d/20_faiss.ini
USER croco
CMD [ "bash" ]
