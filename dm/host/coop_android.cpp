// SPDX-License-Identifier: GPL-3.0-or-later
//
// Stage 1 of the Swap2-Native OoT co-op port: an Android protocol-11 client.
// It proves that the phone can join the v0.10.1 server before guest-memory
// synchronization is enabled. Configure it with the per-game environment
// lines documented in docs/oot-coop-android.md.
#include <android/log.h>

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <limits>
#include <string>
#include <thread>

#include "coop/net/client.hpp"

namespace {

constexpr const char* kTag = "Swap2Coop";
std::atomic<bool> g_stop{false};
std::thread g_worker;

void Log(const char* text) {
  __android_log_print(ANDROID_LOG_INFO, kTag, "%s", text);
}

std::uint16_t PortFromEnv() {
  const char* value = std::getenv("DM_COOP_PORT");
  if (value == nullptr || *value == '\0') return coop::protocol::kDefaultPort;
  errno = 0;
  char* end = nullptr;
  const unsigned long port = std::strtoul(value, &end, 10);
  if (errno != 0 || end == value || *end != '\0' || port == 0 ||
      port > std::numeric_limits<std::uint16_t>::max()) {
    __android_log_print(ANDROID_LOG_ERROR, kTag,
                        "invalid DM_COOP_PORT=%s; using %u", value,
                        coop::protocol::kDefaultPort);
    return coop::protocol::kDefaultPort;
  }
  return static_cast<std::uint16_t>(port);
}

std::string EnvOr(const char* key, const char* fallback) {
  const char* value = std::getenv(key);
  return value != nullptr && *value != '\0' ? value : fallback;
}

void RunClient(std::string server) {
  coop::net::ClientConfig config;
  config.host = std::move(server);
  config.port = PortFromEnv();
  config.hello.build_id = "Swap2-Native Android v0.10.1";
  config.hello.game_build_id = "e0f3ccfaf50065ae";
  config.hello.game_version = "1.1.1";
  config.hello.nickname = EnvOr("DM_COOP_NAME", "Android");
  config.join_password = EnvOr("DM_COOP_PASSWORD", "");

  __android_log_print(ANDROID_LOG_INFO, kTag,
                      "protocol client starting: %s:%u as %s",
                      config.host.c_str(), config.port,
                      config.hello.nickname.c_str());

  coop::net::Client client(std::move(config));
  if (!client.start()) {
    Log("protocol client failed to initialize");
    return;
  }

  while (!g_stop.load(std::memory_order_relaxed)) {
    client.service(20);
    while (auto event = client.next_event()) {
      using coop::net::EventKind;
      switch (event->kind) {
        case EventKind::welcomed:
        case EventKind::reconnected:
          __android_log_print(
              ANDROID_LOG_INFO, kTag,
              "CONNECTED: protocol 11, player %u, %zu other player(s)",
              event->welcome.player_number, event->welcome.players.size());
          break;
        case EventKind::refused:
          __android_log_print(
              ANDROID_LOG_ERROR, kTag, "REFUSED: %s",
              std::string(coop::net::refuse_reason_text(
                              event->refuse_reason))
                  .c_str());
          break;
        case EventKind::connect_failed:
          __android_log_print(ANDROID_LOG_WARN, kTag,
                              "connection attempt failed; retrying");
          break;
        case EventKind::disconnected:
          __android_log_print(ANDROID_LOG_WARN, kTag,
                              "connection lost; retrying");
          break;
        case EventKind::player_joined:
          __android_log_print(ANDROID_LOG_INFO, kTag,
                              "player %u joined: %s", event->player,
                              event->nickname.c_str());
          break;
        case EventKind::player_left:
          __android_log_print(ANDROID_LOG_INFO, kTag,
                              "player %u left", event->player);
          break;
        default:
          break;
      }
    }
  }
  client.stop();
  Log("protocol client stopped");
}

__attribute__((constructor)) void StartCoopProtocolClient() {
  const char* server = std::getenv("DM_COOP_SERVER");
  if (server == nullptr || *server == '\0') {
    Log("protocol client disabled (DM_COOP_SERVER is not set)");
    return;
  }
  g_worker = std::thread(RunClient, std::string(server));
}

__attribute__((destructor)) void StopCoopProtocolClient() {
  g_stop.store(true, std::memory_order_relaxed);
  if (g_worker.joinable()) g_worker.join();
}

}  // namespace
