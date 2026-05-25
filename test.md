电量控制场景以 BAT0 的 charging state 为准，而不是以 AC/USB-PD 是否插入为准。
当前策略使用 80/75 滞回：充电到 80 及以上后进入 inhibit-charge，之后保持该状态，直到电量降到 75 及以下才恢复 auto。

需要覆盖的场景：

1. BAT0 正在充电，电量达到 80 的时候，自动切换到 inhibit-charge。
2. BAT0 正在充电，电量已经在 80 以上时，保持 inhibit-charge。
3. 已经进入 inhibit-charge 后，即使 BAT0 变为 not charging/discharging，80 以下到 75 以上仍保持 inhibit-charge。
4. 电量降到 75 及以下时，切换到 auto。
5. 服务启动时，使用当前 BAT0 charging state 和当前电量应用初始模式：
   - charging 且 80 以上：inhibit-charge
   - charging 且 80 以下：auto
   - not charging/discharging：auto，后续等 BAT0 状态更新为 charging 时再重新评估

测试方式：

1. 将充电策略抽成纯 C++ `ChargePolicy`。
2. 用 mock 输入模拟电量和 BAT0 charging state。
3. 不依赖真实 UPower、D-Bus、systemd 或 `/sys/class/power_supply/BAT0/charge_behaviour`。
