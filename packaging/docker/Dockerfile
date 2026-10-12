# syntax=docker/dockerfile:1
# Robo Violence 2 servers (Phase B step 11): one build, two images for the build machine's architecture (amd64 or
# arm64). Build from the repository root:
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
# Dependencies first, in their own layer: it changes only with vcpkg.json (vcpkg at the manifest's pinned baseline)
ENV VCPKG_ROOT=/opt/vcpkg
COPY vcpkg.json ./
RUN baseline=$(sed -n 's/.*"builtin-baseline": *"\([0-9a-f]*\)".*/\1/p' vcpkg.json) \
    && git init --quiet /opt/vcpkg \
    && git -C /opt/vcpkg fetch --quiet --depth 1 https://github.com/microsoft/vcpkg "$baseline" \
    && git -C /opt/vcpkg checkout --quiet FETCH_HEAD \
    && /opt/vcpkg/bootstrap-vcpkg.sh -disableMetrics
# The triplet follows the build machine's architecture
RUN case "$(uname -m)" in aarch64|arm64) echo arm64-linux ;; *) echo x64-linux ;; esac > /opt/triplet
RUN --mount=type=cache,target=/root/.cache/vcpkg \
    /opt/vcpkg/vcpkg install --triplet="$(cat /opt/triplet)" --x-no-default-features --x-install-root=/opt/vcpkg_installed
COPY . .
# Release servers only; main/ gets the placeholder data (the original assets are never in the image)
RUN cmake --preset linux-x64-release -DBV2_BUILD_CLIENT=OFF -DBV2_BUILD_TESTS=OFF \
        -DVCPKG_MANIFEST_INSTALL=OFF -DVCPKG_INSTALLED_DIR=/opt/vcpkg_installed -DVCPKG_TARGET_TRIPLET="$(cat /opt/triplet)" \
    && cmake --build --preset linux-x64-release --target bv2dedicated bv2master \
    && runtime=build/linux-x64-release/runtime && mkdir -p /out/seed \
    && cp "$runtime/bv2dedicated" "$runtime/bv2master" /out/ \
    && cp -rL "$runtime/main" /out/main \
    && cp "$runtime/master.db" "$runtime/web.db" /out/seed/

# Runtime: both servers run as uid 10001 with their state in the /var/lib/bv2 volume
FROM ubuntu:24.04 AS runtime
LABEL org.opencontainers.image.source=https://github.com/Hub-Tansky/RoboViolence2
RUN useradd --system --uid 10001 --create-home --home-dir /var/lib/bv2 bv2
# Saved config, bv2.db, logs and downloaded maps (ADR 0007); the master's databases
ENV BV2_PREF_DIR=/var/lib/bv2
VOLUME /var/lib/bv2
WORKDIR /opt/bv2

FROM runtime AS server
# The headless server still links GL and GLU (PNS-3)
RUN apt-get update && apt-get install -y --no-install-recommends libgl1 libglu1-mesa && rm -rf /var/lib/apt/lists/*
COPY --from=build /out/bv2dedicated /opt/bv2/
COPY --from=build /out/main /opt/bv2/main
USER bv2
EXPOSE 3333/tcp
# A listening socket, not a connection: a connection would join the game as a player
HEALTHCHECK --interval=10s --timeout=3s --start-period=10s CMD grep -q ':0D05 00000000:0000 0A' /proc/net/tcp
# Arguments: a launch script in main/LaunchScript (default CTF); with --config <file>, name the script too
ENTRYPOINT ["/opt/bv2/bv2dedicated"]
CMD ["CTF"]

FROM runtime AS master
COPY --from=build /out/bv2master /opt/bv2/
COPY --from=build /out/seed /opt/bv2/seed
COPY packaging/docker/master-entrypoint.sh /opt/bv2/
USER bv2
EXPOSE 10207/tcp
HEALTHCHECK --interval=10s --timeout=3s --start-period=10s CMD grep -q ':27DF 00000000:0000 0A' /proc/net/tcp
WORKDIR /var/lib/bv2
ENTRYPOINT ["/opt/bv2/master-entrypoint.sh"]
