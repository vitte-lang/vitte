FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    ca-certificates curl git build-essential bash \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

# Expected: mount repo into /workspace/vitte
CMD ["bash"]
