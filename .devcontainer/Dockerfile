# Multi-stage Dockerfile for Vitte development environment
FROM mcr.microsoft.com/vscode/devcontainers/rust:1-bullseye as base

# Install additional build tools
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    libssl-dev \
    zlib1g-dev \
    git \
    curl \
    wget \
    ccache \
    gdb \
    lldb \
    clang \
    clang-format \
    clang-tools \
    cppcheck \
    valgrind \
    && rm -rf /var/lib/apt/lists/*

# Install Rust components for development
RUN rustup component add rustfmt clippy && \
    cargo install cargo-audit cargo-expand cargo-tree

# Set up ccache for faster C/C++ builds
ENV CC="ccache gcc" \
    CXX="ccache g++" \
    CCACHE_SLOPPINESS="time_macros,file_stat_stat_broken_inode_time"

# Create workspace directory
WORKDIR /workspace

# Copy the project
COPY . .

# Build stage: Compile Vitte
FROM base as builder

RUN chmod +x scripts/bootstrap_stage0.sh scripts/self_host_stage1.sh && \
    ./scripts/bootstrap_stage0.sh && \
    ./scripts/self_host_stage1.sh

# Final stage: Development environment
FROM base as development

# Copy built artifacts from builder
COPY --from=builder /workspace/target /workspace/target

# Set environment variables
ENV PATH="/workspace/target/debug:${PATH}" \
    VITTE_DEV=1

# Install Vitte into PATH
RUN ln -sf /workspace/target/debug/vittec /usr/local/bin/vittec

# Setup git hooks and development environment
RUN chmod +x /workspace/scripts/dev-setup.sh && \
    git init . 2>/dev/null || true

# Set the default shell to bash
SHELL ["/bin/bash", "-c"]

# Entrypoint
CMD ["/bin/bash"]
