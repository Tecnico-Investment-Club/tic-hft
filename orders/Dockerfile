FROM ubuntu:22.04 as builder

LABEL maintainer="TIC <tic.strategy.app@gmail.com>"

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libcurl4-openssl-dev \
    libpq-dev \
    libssl-dev \
    postgresql-client \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

# Clone and build project
COPY . /build/

RUN mkdir -p build && cd build && \
    cmake .. && \
    make -j$(nproc) && \
    make install

# Production stage
FROM ubuntu:22.04

LABEL maintainer="TIC <tic.strategy.app@gmail.com>"

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libcurl4-openssl \
    libpq5 \
    libssl3 \
    postgresql-client \
    && rm -rf /var/lib/apt/lists/*

# Copy binary from builder
COPY --from=builder /usr/local/bin/hft_orders /usr/local/bin/hft_orders

WORKDIR /app

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD ps aux | grep hft_orders | grep -v grep || exit 1

EXPOSE 9001

ENTRYPOINT ["hft_orders"]
CMD []
