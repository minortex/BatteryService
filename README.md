# Battery Service

## Introduction

A simple program for control charging. This tool using QT-DBUS as fundamental, connect to system bus and get result from UPower.

Everytime the battery percentage changes, the tool will check, and decide whether to stop charging.

## Usage

1. Install `cmake` `QT` for dependencies, and compile.
2. Place the `BatteryService.service` into `/etc/systemd/system`, and the `BatteryService` binary into `/usr/bin`.
3. run `sudo systemctl daemon-reload`.
4. run `sudo systemctl enable --now BatteryService`.
5. enjoy.