我有很多公司的tws实际量产的基带代码，但是我不能公开，这个是我用ai重写的，而且像这种小单片机基本也是没有什么技术壁垒，网上很多，仅供学习。

```
TWS v3 实现（对应 bt/profiles/tws.c）

对象：
  PHONE      手机
  A2DP(P)    主耳 A2DP/AVDTP
  TWS(P)     主耳 TWS 状态机
  L2CAP      私有 PSM 0x0080（TWS_MSG_*）
  TWS(S)     副耳 TWS 状态机
  APP(S)     副耳播放/按键

════════════════ 1. 建链与 HELLO ════════════════

PHONE        A2DP(P)        TWS(P)          L2CAP          TWS(S)         APP(S)
  │              │              │               │              │              │
  │  A2DP 建链   │              │               │              │              │
  │─────────────>│              │               │              │              │
  │              │              │ tws_connect(peer,handle)     │              │
  │              │              │──────────────>│              │              │
  │              │              │  state=WAIT_HELLO            │              │
  │              │              │               │              │              │
  │              │              │ tws_hello(phone,feat)        │              │
  │              │              │──HELLO(role,feat,phone6)─────>│              │
  │              │              │               │  handle_hello│              │
  │              │              │               │  sync=READY  │              │
  │              │              │               │  phone=共享  │              │
  │              │              │               │              │ on_link(true)│
  │              │              │               │              │─────────────>│

════════════════ 2. 时钟/锚点同步（周期） ════════════════

TWS(P)                                   TWS(S)
  │ tws_sync(clock_offset, anchor)          │
  │────── SYNC[clock_off(4) anchor(4)] ────>│
  │                                         │ clock_offset=...
  │                                         │ anchor=...
  │                                         │ resyncs++

  注：offset/anchor 只是传递并保存；当前没有抖动缓冲/播放对齐。

════════════════ 3. 媒体转发（每帧） ════════════════

A2DP(P)              TWS(P)                        TWS(S)            APP(S)/DAC
  │ a2dp_send_media() │                              │                    │
  │───── media ──────>│ a2dp->relay                  │                    │
  │                   │ tws_relay_media(data,len,ts) │                    │
  │                   │── MEDIA[seq(2) ts(4) data] ─>│ on_media(data,len,ts)
  │                   │                              │───────────────────>│ 播放
  │                   │ media_seq++  stats.relayed++ │                    │

  当前：ts 由调用方传入（A2DP 转发路径固定传 0）；无 ACK/重传、无丢包补偿。

════════════════ 4. 按键 / 电量 ════════════════

TWS(P)                                   TWS(S)
  │── KEY[key,state] ─────────────────────>│ on_key(key,state)
  │── BATTERY[percent] ───────────────────>│ on_battery(percent)
  │── PING ───────────────────────────────>│ 回 PING

════════════════ 5. 角色切换（primary ↔ secondary） ════════════════

TWS(P)                    LMP                 TWS(S)
  │ tws_request_role_switch()                │
  │ target = 相反角色                        │
  │ pending_role = target                    │
  │──── lmp_role_switch(link) ──>│           │
  │──── ROLE_REQ[target] ───────────────────>│
  │                                         │ 接收方转为 target 的反角色
  │                                         │ 发 ROLE_ACK[自己的角色]
  │<──── ROLE_ACK[role] ────────────────────│
  │ role = pending_role                     │
  │ on_role(role)                           │ on_role(role)
  │                                         │

  结果：请求方转成目标角色，接收方转成相反角色（保证只有一个 primary）。

════════════════ 消息格式 ════════════════

  +--------+--------+--------+--------+--------+-----------------+
  | type 1B| seq 2B | len 2B |        payload (len B)            |
  +--------+--------+--------+--------+--------+-----------------+
       │        │        │
       │        │        └─ 小端长度
       │        └─ 小端消息序号（tws->seq 每条 +1）
       └─ 1 HELLO / 2 SYNC / 3 MEDIA / 4 KEY / 5 BATTERY /
          6 ROLE_REQ / 7 ROLE_ACK / 8 PING

  HELLO : [role 1][features 1][phone 6]
  SYNC  : [clock_offset 4][anchor 4]        (小端)
  MEDIA : [media_seq 2][timestamp 4][data…]
  KEY   : [key 1][state 1]
  BATTERY:[percent 1]
  ROLE_REQ:[target 1]     ROLE_ACK:[role 1]

════════════════ 6. 状态机 ════════════════

  tws_connect()
        │
        ▼
  WAIT_HELLO ──HELLO──> READY ──SYNC──> READY（offset/anchor 更新）
        │                  │
        │                  ├──MEDIA──> on_media
        │                  ├──KEY/BATTERY──> 回调
        │                  └──ROLE_REQ/ACK──> 角色翻转

一句话：主耳先 HELLO 把角色/手机地址给副耳，之后周期 SYNC 传时钟锚点，A2DP 每帧经 MEDIA 转发，按键/电量单独消息，角色切换由 ROLE_REQ/ACK 完成；当前这条链路在 tests/tws_selftest.c 里 12/12 实跑通过。
```
