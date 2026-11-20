FROM alpine:3.24 AS base-image
RUN set -xe \
    && apk update \
    && apk add \
        autoconf \
        automake \
        bash \
        cmake \
        g++ \
        gcc \
        gdb \
        git \
        linux-headers \
        make \
        openblas-dev \
        sudo \
        xz \
    && adduser -D croco \
    && addgroup croco abuild \
    && echo "croco ALL=(ALL:ALL) NOPASSWD:ALL" >> /etc/sudoers

FROM base-image AS llama-image
WORKDIR /opt
RUN set -xe \
    && git clone --recurse-submodules https://github.com/ggml-org/llama.cpp.git \
    && cd llama.cpp \
    && cmake -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -DBUILD_SHARED_LIBS=ON \
        -DLLAMA_BUILD_EXAMPLES=OFF \
        -DLLAMA_BUILD_TOOLS=OFF \
        -DLLAMA_BUILD_TESTS=OFF \
        -DLLAMA_BUILD_SERVER=OFF \
        -DGGML_NATIVE=ON \
        -DGGML_METAL=OFF \
        -DGGML_METAL_EMBED_LIBRARY=OFF \
        -DGGML_BLAS_DEFAULT=ON \
        -DGGML_METAL_USE_BF16=ON \
        -DGGML_OPENMP=OFF \
        -DLLAMA_CURL=OFF \
        -DNDEBUG=ON \
    && cd build \
    && make -j $(nproc) \
    && make install

FROM llama-image AS runtime-image
COPY --chown=croco:croco src/ /home/croco/embedding/
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
WORKDIR /home/croco/embedding
RUN set -xe \
    && cmake -B build -DPHP_CONFIG=php-config85 \
    && cd build \
    && make -j $(nproc) \
    && make install \
    && echo "extension=croco_embedding" > /etc/php85/conf.d/20_embedding.ini
USER croco
CMD [ "bash" ]

