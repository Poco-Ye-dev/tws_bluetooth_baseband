# TWS Bluetooth Baseband (src_ai)

面向学习用途的蓝牙/TWS 重写工程：从 HCI、L2CAP、经典 Profile，到 LE/EDR 基带骨架，以及 TWS v3 主副耳协议与双向音频同步仿真。

> 说明：仓库里的代码是按原厂 TWS 架构做的自研重写，寄存器地址、部分算法为模拟实现，**不能直接烧录或与真机互通**。真机还需要芯片 RF/模拟校准、硬件 SBC 解码、I2S/DAC 音频输出和真实 L2CAP/中断/时隙接线。

## 目录结构

```
apps/        产品应用框架（earphone/soundbox/dongle/iot）
audio/       音频框架、DSP、ANC/CVP/KWS、CPU 音频 port
bt/
  le/        LE: LL/CSA/ISO/扩展广播/8 实例 HW/定时器/IRQ/双 ring
  edr/       EDR: LC/ARQ/SCO/LMP/SSP/E0/包型/AFH/8 frame HW
  common/    packet/hopping/HCI bridge
  rf/        radio 抽象 + 双模 RF 共存
  hci/ l2cap/ att/ gatt/ gap/ smp/ sdp/ rfcomm/ profiles/
drivers/     gpio/uart/i2c/spi/adc/pwm/flash/dma/i2s/rtc/watchdog/irq/regmap
kernel/      lib(字符串/堆/日志/CRC/数学/E0) + os(任务/同步/定时器/事件/workqueue)
media/       codec / pipeline / container / frame pool / ADPCM / SBC 硬解接口
interface/   构建工具脚本与 utils(fs/ui/lzss)
include/     全部公共头文件
cpu/         7 款芯片 port/clock/regs/ble5/bt_baseband
tests/       host 自测与仿真（TWS 单播、双向、同步、SBC 播放）
update/      OTA
tools/       compiler/platform/utils + 工程脚本
```

## 构建/检查

```bash
make -n app                  # 依赖与编译单元检查
gcc -std=c11 -Wall -Wextra -I include -I cpu/tcl76484 -c <file.c>
```

当前：全量 `.c` 编译 **0 error / 0 warning**，`make -n app` 约 100+ 编译单元。

## TWS v3 实现

文件：`include/bt/tws.h`、`bt/profiles/tws.c`、`bt/profiles/a2dp.c`（relay 挂点）。

角色：`primary` 持手机 A2DP 链路，`secondary` 通过私有 L2CAP PSM `0x0080` 跟随。

消息：`HELLO / SYNC / MEDIA / MEDIA_UP / KEY / BATTERY / ROLE_REQ / ROLE_ACK / PING`，帧格式：

```
+--------+--------+--------+--------+--------+------------------+
| type 1 | seq 2  | len 2  |          payload (len B)           |
+--------+--------+--------+--------+--------+------------------+

HELLO    : role, features, phone[6]
SYNC     : clock_offset(4), anchor(4)        小端
MEDIA    : media_seq(2), timestamp(4), audio…   主 → 副
MEDIA_UP : up_seq(2),    timestamp(4), audio…   副 → 主
KEY      : key, state
BATTERY  : percent
ROLE_REQ : target        ROLE_ACK : role
```

### TWS 排帧：定时器 → 开窗 → RX 中断 → 再定时

TWS 的 MEDIA/KEY/SYNC 消息最终都承载在 L2CAP/ACL 上，所以排帧分两层：

1. **基带排帧**（`bt/le/le_link.c`、`bt/edr/edr_link.c`、`le_timer.c`、`le_irq.c`）
   - 每条 TWS 链路一个 anchor，时间单位 625µs
   - 定时器在 `anchor - 2 slots` 提前醒来，到 anchor 打开 RX 窗口（winsize + widening）
   - 收到包触发 `RX_DONE` 中断，ISR 只做：读 HW buffer → `rx_ring` → ACK
   - 之后重新 `arm(next_anchor - 2)`，等下一个连接事件
2. **TWS 应用排帧**（`bt/profiles/tws.c`）
   - `tws_relay_media()` / `tws_send_key()` 等把消息写入 `tx_ring`
   - 窗口到来时从 `tx_ring` 取帧发送；窗口内可连续发多帧（受 winsize/缓冲限制）
   - 接收侧 `rx_ring` 由 `tws_rx()` 消费，按 `type/seq/len` 分发

时序（slot = 625µs）：

```
slot        N-2              N-1            N(anchor)        N+1          N+2
timer       |── arm(anchor-2) ────────────>| IRQ wake
radio                                      [window open: winsize + widen]
TX 主耳                                    [TWS MEDIA #k / KEY / SYNC]
RX 副耳                                    [window]──RX_DONE IRQ──> rx_ring ─> tws_rx()
re-arm      next_anchor = N + interval_slots；timer = next_anchor - 2
```

伪代码：

```c
/* 连接建立后 */
anchor = now + interval_slots;
le_timer_arm(anchor - 2);          /* 提前 2 slot 醒来 */

/* 定时器到点 */
window_open():
    radio_tune(csa2(aa, event_counter, ch_map));
    le_hw_set_window(winsize, widen);
    le_hw_enable(true);            /* 开接收窗口 */

/* RX 中断 */
rx_done_irq():
    pdu = le_hw_rx_read();
    le_ring_push(&rx_ring, pdu);
    tws_rx(tws, bt_buf);           /* 或 le_ll_rx() */
    if (!le_ring_empty(&tx_ring))
        send_next_from_tx_ring();
    le_hw_enable(false);
    anchor += interval_slots;
    le_timer_arm(anchor - 2);      /* 收下一个包 */
```

要点：
- **提前 2 slot 醒来**是为了给 RF 稳定、时钟漂移和窗口 widening 留余量
- 窗口内没收到包则超时，直接 re-arm 下一个 anchor；不阻塞
- TWS 的上下行（MEDIA / MEDIA_UP）共用同一个 anchor 调度，方向由发送侧决定
- 当前 host 自测用 stub 替代了定时器/中断；目标端由 `le_link/edr_link` 的真实 tick + IRQ 驱动

### 时序 1：建链与 HELLO

```
PHONE        A2DP(P)        TWS(P)          L2CAP          TWS(S)         APP(S)
  │              │              │               │              │              │
  │  A2DP 建链   │              │               │              │              │
  │─────────────>│              │               │              │              │
  │              │              │ tws_connect(peer,handle)     │              │
  │              │              │──────────────>│              │              │
  │              │              │  state=WAIT_HELLO            │              │
  │              │              │ tws_hello(phone,feat)        │              │
  │              │              │──HELLO(role,feat,phone6)─────>│              │
  │              │              │               │  sync=READY  │              │
  │              │              │               │  phone 共享  │ on_link(true)│
  │              │              │               │              │─────────────>│
```

### 时序 2：时钟/锚点同步（双向）

```
TWS(P)                                       TWS(S)
  │── SYNC[clock_off, anchor] ──────────────>│  clock_offset/anchor 保存
  │<─ SYNC[clock_off, anchor] ───────────────│  副耳也可同步给主耳
```

当前只保存 offset/anchor 供上层对齐；没有抖动缓冲/播放时钟锁相。

### 时序 3：下行音频（手机 → 主耳 → 副耳）

```
A2DP(P)              TWS(P)                         TWS(S)              APP(S)/DAC
  │ a2dp_send_media() │                               │                     │
  │───── media ──────>│ a2dp->relay                    │                     │
  │                   │ tws_relay_media(data,len,ts)   │                     │
  │                   │── MEDIA[seq, ts, audio] ──────>│ on_media(ts)        │
  │                   │ media_seq++                    │ 按 ts+offset 入缓冲 │
  │                   │                                │────────────────────>│ 播放
```

### 时序 4：上行音频（副耳麦克风 → 主耳）

```
TWS(S)                                             TWS(P)
  │ tws_relay_media_up(data,len,ts)                   │
  │── MEDIA_UP[seq, ts, audio] ──────────────────────>│ on_media_up(ts)
  │ up_seq++                                          │ 按 ts+offset 入缓冲
```

### 时序 5：按键 / 电量 / 心跳

```
TWS(P)                                     TWS(S)
  │── KEY[key,state] ─────────────────────>│ on_key()
  │── BATTERY[percent] ───────────────────>│ on_battery()
  │── PING ───────────────────────────────>│ 回 PING
```

### 时序 6：角色切换（primary ↔ secondary）

```
TWS(P)                    LMP                 TWS(S)
  │ tws_request_role_switch()                │
  │ target = 相反角色, pending_role = target │
  │──── lmp_role_switch(link) ──>│           │
  │──── ROLE_REQ[target] ───────────────────>│
  │                                         │ 接收方转为 target 的反角色
  │<──── ROLE_ACK[role] ────────────────────│
  │ role = pending_role; on_role()           │ on_role()
```

### TWS 状态机

```
tws_connect() → WAIT_HELLO ──HELLO──> READY
                                   ├── SYNC
                                   ├── MEDIA / MEDIA_UP
                                   ├── KEY / BATTERY / PING
                                   └── ROLE_REQ / ROLE_ACK
```

## SBC 播放仿真

`media/codec/sbc_hw.c` 提供 SBC 头解析（真实帧长计算）和模拟硬件解码接口 `sbc_hw_decode()`。目标板把该函数绑定芯片 SBC 硬件即可；host 仿真用参考 PCM 驱动。

流程：

```
SBC 文件 → sbc_parse_header() 逐帧切分 → sbc_hw_decode()（模拟硬件）
        → tws_relay_media() → 副耳按 offset 对齐 → 两侧 WAV
```

## 自测（host，无需硬件）

```bash
./tests/run_tws_selftest.sh        # 协议状态机        12/12 PASS
./tests/run_tws_sync_demo.sh       # 单向下行同步      40/40, 0 mismatch
./tests/run_tws_bidir_demo.sh      # 双向音频同步      32/32 双向, 0 mismatch
./tests/run_tws_sbc_demo.sh        # SBC 歌 → 双耳 WAV
./tests/run_tws_sbc_hw_demo.sh     # SBC + 模拟硬解     1033 帧, 0 mismatch
```

生成的 WAV 位于 `tmp/`，两侧文件逐字节一致。

## 当前边界

- host 仿真中 L2CAP/LMP/A2DP 为 stub；目标端仍需 PSM 监听、data 回调、625µs tick、radio/transport、app 初始化接线
- SBC 解码为模拟（参考 PCM）；真机需要芯片硬解或自研软件 SBC codec
- 无 I2S/DAC 输出、无 ACK/重传、无抖动缓冲、无 TWS 鉴权
- LE/EDR 寄存器为模拟地址；E0/SSP 为简化实现
- 这是学习/架构参考工程，不是量产 SDK
