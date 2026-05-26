# batteryctl

`batteryctl` controls BAT0 charging behavior on systems that expose
`/sys/class/power_supply/BAT0/charge_behaviour`.

The daemon listens to UPower on the system D-Bus. When BAT0 is charging and the
battery reaches 80%, it writes `inhibit-charge`. It keeps that mode until the
battery drops to 75% or below, then writes `auto`.

## Commands

```sh
batteryctl daemon          # run the policy daemon
batteryctl monitor         # run the daemon in the foreground
batteryctl once            # apply the policy once
batteryctl status          # show concise battery and daemon status
batteryctl auto            # set auto for the current charge session
batteryctl inhibit         # set inhibit-charge for the current charge session
batteryctl service status  # show full BatteryService.service status
batteryctl service restart # restart BatteryService.service
batteryctl help            # show help
```

Running `batteryctl` without arguments is the same as `batteryctl daemon`.

`auto` and `inhibit` are manual override commands for the current charge
session. They are only accepted while external power is online. The daemon
remains running, but pauses automatic policy while the override file exists.
After the current charge session ends, the daemon clears the override and
resumes automatic policy on the next charge session.

## Service

The packaged systemd unit remains `BatteryService.service` for compatibility.
It starts the daemon with:

```ini
ExecStart=/usr/bin/batteryctl daemon
```

Enable it with:

```sh
sudo systemctl enable --now BatteryService.service
```

## Build

```sh
cmake --preset linux-gcc-ninja-release
cmake --build --preset linux-gcc-ninja-release-build
ctest --test-dir build/linux-gcc-ninja-release --output-on-failure
```

## Package

On Arch Linux:

```sh
makepkg --cleanbuild
```

If building from this source tree, prefer using a separate `BUILDDIR` or a
copy of `PKGBUILD`, because makepkg uses `src/` as a build directory.
