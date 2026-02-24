FROM ubuntu:22.04

WORKDIR /project

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libpqxx-dev \
    libpq-dev \
    libssl-dev \
    libcurl4-openssl-dev \
    curl \
    nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy project files
COPY . /project

# Build
RUN mkdir -p /project/build && \
    cd /project/build && \
    cmake .. && \
    make -j4 && \
    make install

# Set environment
ENV PATH="/usr/local/bin:${PATH}"

# Default command
CMD ["paper-engine-orders"]
