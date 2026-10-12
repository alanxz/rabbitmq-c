#!/bin/bash
# SessionStart hook for Claude Code cloud sessions: installs the toolchain,
# sanitizer runtimes and a RabbitMQ broker needed to build and test rabbitmq-c.
set -euo pipefail

if [ "${CLAUDE_CODE_REMOTE:-}" != "true" ]; then
  exit 0
fi

export DEBIAN_FRONTEND=noninteractive
SUDO=""
if [ "$(id -u)" -ne 0 ]; then SUDO="sudo"; fi

CODENAME="$(. /etc/os-release && echo "${VERSION_CODENAME}")"
LLVM_VER="$(clang --version 2>/dev/null | sed -n 's/.*version \([0-9]*\)\..*/\1/p' | head -1 || true)"

# --- Build toolchain, sanitizers, and dev utilities ------------------------
PKGS=(
  build-essential cmake ninja-build pkg-config git curl ca-certificates gnupg
  libssl-dev libpopt-dev xmlto doxygen        # optional rabbitmq-c deps (tools, docs)
  clang clang-format clang-tidy clang-tools   # clang-tools provides scan-build
  llvm                                        # llvm-symbolizer for readable sanitizer traces
  gdb valgrind lcov gcovr                     # debugging and coverage
)
if [ -n "${LLVM_VER}" ]; then
  PKGS+=("libclang-rt-${LLVM_VER}-dev")       # asan/ubsan/tsan/msan runtimes
else
  PKGS+=(libclang-rt-dev)
fi

MISSING=()
for p in "${PKGS[@]}"; do
  dpkg -s "$p" >/dev/null 2>&1 || MISSING+=("$p")
done
if [ "${#MISSING[@]}" -gt 0 ]; then
  $SUDO apt-get update -qq
  $SUDO apt-get install -y -qq --no-install-recommends "${MISSING[@]}"
fi

# --- RabbitMQ broker (latest, from the Team RabbitMQ apt repos) ------------
install_latest_rabbitmq() {
  local keyring=/usr/share/keyrings
  curl -fsSL https://keys.openpgp.org/vks/v1/by-fingerprint/0A9AF2115F4687BD29803A206B73A36E6026DFCA \
    | $SUDO gpg --dearmor -o "$keyring/com.rabbitmq.team.gpg"
  curl -fsSL https://github.com/rabbitmq/signing-keys/releases/download/3.0/cloudsmith.rabbitmq-erlang.E495BB49CC4BBE5B.key \
    | $SUDO gpg --dearmor -o "$keyring/rabbitmq.E495BB49CC4BBE5B.gpg"
  curl -fsSL https://github.com/rabbitmq/signing-keys/releases/download/3.0/cloudsmith.rabbitmq-server.9F4587F226208342.key \
    | $SUDO gpg --dearmor -o "$keyring/rabbitmq.9F4587F226208342.gpg"

  $SUDO tee /etc/apt/sources.list.d/rabbitmq.list >/dev/null <<LIST
deb [arch=amd64 signed-by=$keyring/rabbitmq.E495BB49CC4BBE5B.gpg] https://ppa1.rabbitmq.com/rabbitmq/rabbitmq-erlang/deb/ubuntu $CODENAME main
deb [arch=amd64 signed-by=$keyring/rabbitmq.9F4587F226208342.gpg] https://ppa1.rabbitmq.com/rabbitmq/rabbitmq-server/deb/ubuntu $CODENAME main
LIST
  $SUDO apt-get update -qq
  $SUDO apt-get install -y -qq --no-install-recommends \
    erlang-base erlang-asn1 erlang-crypto erlang-eldap erlang-ftp erlang-inets \
    erlang-mnesia erlang-os-mon erlang-parsetools erlang-public-key \
    erlang-runtime-tools erlang-snmp erlang-ssl erlang-syntax-tools \
    erlang-tftp erlang-tools erlang-xmerl rabbitmq-server
}

if ! command -v rabbitmq-server >/dev/null 2>&1; then
  if ! install_latest_rabbitmq; then
    echo "WARNING: Team RabbitMQ repos unreachable (check the environment's network allowlist);" >&2
    echo "         falling back to the distro rabbitmq-server package." >&2
    $SUDO rm -f /etc/apt/sources.list.d/rabbitmq.list
    $SUDO apt-get update -qq
    $SUDO apt-get install -y -qq rabbitmq-server
  fi
fi

# --- Start the broker (no systemd in the container) ------------------------
if ! rabbitmqctl status >/dev/null 2>&1; then
  $SUDO rabbitmq-server -detached
  # The node registers with epmd a moment after -detached returns, so retry.
  for _ in $(seq 1 60); do
    $SUDO rabbitmqctl await_startup --timeout 5 >/dev/null 2>&1 && break
    sleep 2
  done
  $SUDO rabbitmqctl status >/dev/null
fi
echo "RabbitMQ $($SUDO rabbitmqctl version 2>/dev/null || true) running on localhost:5672"

# --- Convenience env for builds --------------------------------------------
if [ -n "${CLAUDE_ENV_FILE:-}" ]; then
  echo 'export ASAN_OPTIONS=detect_leaks=1:abort_on_error=0' >> "$CLAUDE_ENV_FILE"
  echo 'export UBSAN_OPTIONS=print_stacktrace=1' >> "$CLAUDE_ENV_FILE"
fi
