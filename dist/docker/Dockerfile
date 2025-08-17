FROM debian:stable-slim
ARG VITTE_VERSION=0.1.0
LABEL org.opencontainers.image.title="Vitte"           org.opencontainers.image.version=$VITTE_VERSION           org.opencontainers.image.source="https://example.org/vitte"           org.opencontainers.image.description="Vitte toolchain container"
RUN apt-get update && apt-get install -y ca-certificates && rm -rf /var/lib/apt/lists/*
COPY vitte /usr/local/bin/vitte
COPY vitte-cli /usr/local/bin/vitte-cli
ENTRYPOINT ["vitte-cli"]
