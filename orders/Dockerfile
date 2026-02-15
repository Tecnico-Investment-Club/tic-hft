FROM ubuntu:22.04 as builder

RUN apt-get update && apt-get install -y \
    build-essential cmake git \
    libcurl4-openssl-dev libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY . /build/

RUN mkdir build && cd build && cmake .. && make -j$(nproc)

FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libcurl4-openssl libssl3 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /build/build/hft_orders /usr/local/bin/hft_orders

WORKDIR /app
ENTRYPOINT ["hft_orders"]
