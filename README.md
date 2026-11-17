我有很多公司的tws实际量产的基带代码，但是我不能公开，这个是我用ai重写的，而且像这种小单片机技术壁垒并不高，网上很多，仅供学习。

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

### 经典蓝牙 TWS 排帧（BR/EDR）

TWS 不是 BLE 的连接事件模型。TWS 在经典蓝牙里是两条 BR/EDR 链路：

- 手机 ↔ 主耳：一条 ACL（音频 A2DP、控制 AVRCP、通话 SCO/eSCO）
- 主耳 ↔ 副耳：另一条 TWS ACL（TWS 私有协议 + 媒体/同步/控制）

两条链路共享同一个 625µs slot 时钟，由控制器分时调度。

#### 1. 时隙与主从

```
slot:      2k            2k+1          2k+2          2k+3
手机 piconet:  M(phone) TX   S(主耳) RX   M(phone) TX   S(主耳) RX
TWS  piconet:                              M(主耳) TX    S(副耳) RX
```

- master 在偶数 slot 发，slave 在奇数 slot 回；包可占 1/3/5 个 slot（DM1/DH1 … 3-DH5）
- 主耳对手机通常是 slave，对副耳是 master（或通过角色切换交换）
- 没有 BLE 的 connection interval；靠 master 轮询 + slot 分配

#### 2. 8 个硬件 frame 上下文

`BREDR_FRAME_NUM = 8`（`classic_new/bredr_regs.h`），每个 frame 是一个 LT_ADDR/链路调度上下文：

```
bredr_bd_get_frame()
    ├─ 分配空闲 frame，设置 frame->id
    ├─ __write_reg_bdaddr()   对端地址
    ├─ __write_reg_format()   包类型（DM/DH/2-DH/3-DH…）
    ├─ __write_reg_clkoffset() 时钟偏移
    ├─ windset(frame, winsize, wide, widen)  RX 窗口
    ├─ bdhw_set_afh(frame, …)  AFH 频率表
    └─ bd_set_prio(frame, prio)

set_slot_triger()  → 硬件在指定 slot 发/收
RX IRQ → bredr_rx_irq_handler() → 分发 TWS/L2CAP → bredr_bd_put_frame()
```

优先级（高到低）：**eSCO/SCO 语音 > TWS 控制/同步 > A2DP ACL > 空闲 POLL**。

#### 3. 双链路 slot 调度

主耳同时维护手机链路和 TWS 链路：

- 手机 master 决定的 slot 用于收手机包/回 ACK
- 剩余 slot 分配给 TWS 链路：TWS 媒体转发、tlcap 控制、AFH/同步
- 多 slot 包（DH3/DH5）会占用连续 slot，调度时先预留
- SCO/eSCO 先占时隙，TWS 与 A2DP 让路；Sniff 时合并窗口
- 空闲发 POLL/NULL 维持链路，不做无意义重传

#### 4. RX 中断与 frame 回收

```
TX: 填 TX buffer → set_slot_triger → 下一个 master slot 发出
RX: windset 打开接收窗口 → 包到 → BREDR_INT0 中断
    → bredr_rx_irq_handler()
        ├─ 按 frame id / LT_ADDR 找 frame
        ├─ 校验 CRC/HEC，处理 ARQ（SEQN/ARQN）
        ├─ 分发：TWS 数据 → tws 协议；L2CAP → 上层
        └─ bredr_bd_put_frame() 或复用到下一包
```

#### 5. TWS 同步：AFH 与 slot_end

`tws_afh.c` 用 slot 时钟对齐两耳的 AFH 切换点：

```c
u32 slot_end = bredr_get_master_slot_clk(tws_link);
slot_end += 400;
if (lmp_get_esco_conn_statu())      slot_end += 800;
if (tws_link_get_sniff_status(tws_link)) slot_end += 6000;

memcpy(&tx_buf[10], &slot_end, 4);
tlcap_send_cmd(TWS_AFH_CHANNEL, MASTER_CHANNEL_MAP, "c14", tx_buf);
tws_link_set_tws_afh(tws_link, slot_end, tx_buf);
```

即：**在消息里带“生效 slot”，两耳在同一个 slot_end 切换信道图**；eSCO 和 Sniff 分别留更大余量。

其他同步：
- `tws_lmp_sync.c`：slot 时钟/偏移同步
- `tws_media_sync.c` / `local_media_sync.c`：媒体时间戳/播放对齐
- `tws_esco.c` / `tws_esco_role_sw.c`：语音时隙与角色切换

#### 6. TWS 媒体转发（经典蓝牙）

```
手机 --A2DP(SBC/AAC)--> 主耳 sink --重打包--> TWS ACL --slot 调度--> 副耳
                                                                      ↓
                                             TWS 媒体同步(timestamp) → 解码 → 播放
```

主耳把 A2DP 媒体重新封成 TWS 媒体包，放入 TWS 链路的 ctrl_frame 队列；副耳收到后按 `tws_media_sync` 的时间戳对齐播放。语音走 eSCO 预留 slot，优先于媒体 ACL。

#### 7. 与 src_ai 的对应/差距

`src_ai` 的 EDR 侧是对上述流程的简化：

- 有：`bt/edr/edr_frame.c`（优先级帧队列）、`edr_hw.c`（模拟寄存器：slot trigger/window/AFH/IRQ）、`edr_irq.c`（8 实例 RX/TX/SLOT 中断）、`edr_link.c`（8 frame + 定时器 + 双 ring）、`tws.c`（TWS 消息）
- 缺：原厂的 `set_slot_triger` 精确时隙、`windset` 窗口 widening、`bd_set_prio` 全优先级、AFH 的 `slot_end` 同步、eSCO 重传窗口、Sniff 实际窗口

所以文档里的“slot 级排帧”是经典蓝牙的正确原理；`src_ai` 目前用 `edr_link_tick()` 轮询 + 8 frame 队列近似，尚未做到逐 slot 的硬件级调度。

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
