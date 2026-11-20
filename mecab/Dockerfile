FROM alpine:3.24 AS base-image
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

FROM base-image AS mecab-image
WORKDIR /opt
RUN set -xe \
    && curl -LO https://github.com/taku910/mecab/archive/refs/heads/master.tar.gz \
    && tar -xzf master.tar.gz \
    && mv mecab-master mecab \
    && rm master.tar.gz \
    && cd mecab/mecab \
    && ./configure \
		--prefix=/usr \
		--enable-utf8-only \
    && make -j $(nproc) \
    && make install

FROM mecab-image AS ipadic-image
WORKDIR /opt/mecab/mecab-ipadic
RUN set -xe \
    && ./configure --with-charset=utf8 \
    && make -j $(nproc) \
    && make install

FROM ipadic-image AS runtime-image
COPY --chown=croco:croco src/ /home/croco/mecab/
RUN set -xe \
    && apk update \
    # php84 は 8.4.28-r0 以上を要求する（2026-11-19 のセキュリティリリース。#10）。
    # それより古いパッケージしか取れないときはビルドを失敗させ、古い PHP のまま入るのを防ぐ
    && apk add \
        'php84>=8.4.28-r0' \
        php84-common \
        php84-dev \
    && cd /usr/bin \
    && ln -s php84 php
WORKDIR /home/croco/mecab
RUN set -xe \
    && cmake -B build \
    && cd build \
    && make -j $(nproc) \
    && make install \
    && echo "extension=croco_mecab" > /etc/php84/conf.d/20_mecab.ini
USER croco
CMD [ "bash" ]

