# syntax=docker/dockerfile:1
# Robo Violence 2 servers (Phase B step 11): one build, two images. Build from the repository root:
#   docker build -f packaging/docker/Dockerfile --target server -t roboviolence2-server .
#   docker build -f packaging/docker/Dockerfile --target master -t roboviolence2-master .
# Running them: docs/ops/runbook.md.

FROM ubuntu:24.04 AS build
ARG DEBIAN_FRONTEND=noninteractive
# The server-only build: compiler, Ninja, CMake, autotools (vcpkg ports), Python (placeholder data), GL and GLU headers
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential ninja-build cmake git curl ca-certificates zip unzip tar pkg-config \
        autoconf autoconf-archive automake libtool libltdl-dev python3 libgl1-mesa-dev libglu1-mesa-dev \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY vcpkg.json ./
# vcpkg at the manifest's pinned baseline
RUN baseline=$(sed -n 's/.*"builtin-baseline": *"\([0-9a-f]*\)".*/\1/p' vcpkg.json) \
    && git clone --quiet --filter=blob:none https://github.com/microsoft/vcpkg /opt/vcpkg \
    && git -C /opt/vcpkg checkout --quiet "$baseline" \
    && /opt/vcpkg/bootstrap-vcpkg.sh -disableMetrics
ENV VCPKG_ROOT=/opt/vcpkg
COPY . .
# Release servers only; main/ gets the placeholder data (the original assets are never in the image)
RUN --mount=type=cache,target=/root/.cache/vcpkg \
    cmake --preset linux-x64-release -DBV2_BUILD_CLIENT=OFF -DBV2_BUILD_TESTS=OFF \
    && cmake --build --preset linux-x64-release --target bv2dedicated bv2master \
    && runtime=build/linux-x64-release/runtime && mkdir -p /out/seed \
    && cp "$runtime/bv2dedicated" "$runtime/bv2master" /out/ \
    && cp -rL "$runtime/main" /out/main \
    && cp "$runtime/master.db" "$runtime/web.db" /out/seed/

# Runtime: GL and GLU are still linked by the headless server (PNS-3). The servers run as uid 10001.
FROM ubuntu:24.04 AS runtime
RUN apt-get update && apt-get install -y --no-install-recommends libgl1 libglu1-mesa \
    && rm -rf /var/lib/apt/lists/* \
    && useradd --system --uid 10001 --create-home --home-dir /var/lib/bv2 bv2
# Saved config, bv2.db, logs and downloaded maps (ADR 0007); the master's databases
ENV BV2_PREF_DIR=/var/lib/bv2
VOLUME /var/lib/bv2
WORKDIR /opt/bv2

FROM runtime AS server
COPY --from=build /out/bv2dedicated /opt/bv2/
COPY --from=build /out/main /opt/bv2/main
USER bv2
EXPOSE 3333/tcp
HEALTHCHECK --interval=10s --timeout=3s --start-period=10s CMD bash -c '</dev/tcp/127.0.0.1/3333' || exit 1
# Arguments: a launch script in main/LaunchScript (default CTF); with --config <file>, name the script too
ENTRYPOINT ["/opt/bv2/bv2dedicated"]
CMD ["CTF"]

FROM runtime AS master
COPY --from=build /out/bv2master /opt/bv2/
COPY --from=build /out/seed /opt/bv2/seed
USER bv2
EXPOSE 10207/tcp
HEALTHCHECK --interval=10s --timeout=3s --start-period=10s CMD bash -c '</dev/tcp/127.0.0.1/10207' || exit 1
# The master reads master.db and web.db from its working directory: the volume, seeded on first start
WORKDIR /var/lib/bv2
ENTRYPOINT ["sh", "-c", "for db in master.db web.db; do [ -f \"$db\" ] || cp /opt/bv2/seed/\"$db\" .; done; exec /opt/bv2/bv2master"]
