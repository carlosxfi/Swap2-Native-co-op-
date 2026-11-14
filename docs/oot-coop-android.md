# OoT co-op on Android

This branch carries the first Android integration milestone for
[oot-remake-multiplayer v0.10.1](https://github.com/daurosk/oot-remake-multiplayer/releases/tag/v0.10.1).

## Current milestone

The APK builds the upstream encrypted protocol-11 client and can join a
v0.10.1 server. It does **not** read or write game memory yet, so this stage
does not display another player or synchronize the world. Its purpose is to
prove Android networking, ENet, password encryption, and release compatibility
before the guest bridge is enabled.

The upstream co-op repository is pinned as a Git submodule. Clone recursively:

```sh
git clone --recurse-submodules -b oot-coop-android \
  https://github.com/carlosxfi/Swap2-Native-co-op-.git
```

If the repository was already cloned:

```sh
git submodule update --init --recursive
```

Build the APK with the normal Swap2-Native Android instructions.

## Connection settings

In Zelda's game settings, add these environment lines:

```text
DM_COOP_SERVER=192.168.1.100
DM_COOP_PORT=27510
DM_COOP_NAME=Android
DM_COOP_PASSWORD=the-host-password
```

Use the Windows host PC's LAN address for `DM_COOP_SERVER`. The first
milestone uses a direct LAN/VPN address, not an invite code. The password must
match the v0.10.1 host session.

Start the Windows host first, then launch Zelda on Android. Export the game's
log and look for the `Swap2Coop` tag. Success is:

```text
CONNECTED: protocol 11, player N, M other player(s)
```

A refusal names the cause (wrong protocol, wrong game build, full server,
duplicate/bad name, or wrong password).

## Next milestones

1. Bind Swap2's mapped `main` module to the co-op guest-memory interface.
2. Pump the co-op session from the game's once-per-frame point.
3. Send local status, scene and position.
4. Add remote player presence, then Link/Navi puppets.
5. Add save, item, flag, dungeon and object synchronization.
6. Add an Android connection UI and invite-code support.

Voice chat and hosting on Android are intentionally deferred.
