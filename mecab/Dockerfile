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
    # php85 は 8.5.13-r0 以上を要求する（2026-11-19 のセキュリティリリース。#10）。
    # それより古いパッケージしか取れないときはビルドを失敗させ、古い PHP のまま入るのを防ぐ
    && apk add \
        'php85>=8.5.13-r0' \
        php85-common \
        php85-dev \
    # Alpine 3.24 の php85 は /usr/bin/php を自前で持つので、上書きできる形にする
    && cd /usr/bin \
    && ln -sf php85 php \
    # 例外・致命的エラーのバックトレースに引数を載せない（CVE-2026-129474 / CVE-2026-129182 の条件を外す）
    && printf 'zend.exception_ignore_args = 1\nfatal_error_backtraces = 0\n' > /etc/php85/conf.d/99-backtrace.ini
WORKDIR /home/croco/mecab
RUN set -xe \
    && cmake -B build -DPHP_CONFIG=php-config85 \
    && cd build \
    && make -j $(nproc) \
    && make install \
    && echo "extension=croco_mecab" > /etc/php85/conf.d/20_mecab.ini
USER croco
CMD [ "bash" ]

