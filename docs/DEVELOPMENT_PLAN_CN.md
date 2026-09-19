# AudMeS 开发计划（中文仪器版）

更新时间：2026-09-18  
目标分支：`feature/instrument-ui-cn-sa440f5`

> 本计划用于 ZXYHtech/AudMeS 下游版本。上游项目、原作者、版权和 GPLv2 许可保持不变。

## 1. 产品目标

将原版 AudMeS 从“通用声卡测量工具”逐步改造成更接近电子测试仪器的桌面软件，当前第一目标设备为 **TOPPING E4x4 Pre**，第一目标 DUT 为 **SA-440F5 / 等效低噪声前置放大器**。

第一阶段重点保留和强化：

- FFT 频谱分析
- THD / 后续 THD+N
- Sweep 幅频响应
- 低频噪声分析
- 音频接口自动识别与校准
- SA-440F5 一键测试流程
- 中文化、仪器化、可重复测试和报告导出

Generator 和 Oscilloscope 暂不作为主界面核心入口，但继续保留为 Sweep、校准和自动测试的底层能力。

---

## 2. 当前状态

### 已完成

- [x] AudMeS 官方 GPLv2 源码迁入 ZXYHtech/AudMeS
- [x] 保留 `COPYING`、原作者和上游来源说明
- [x] 创建中文仪器版开发分支
- [x] 主界面改为“仪器总览 / FFT·THD / Sweep / SA-440F5 一键测试”
- [x] 默认中文主界面与音频接口设置
- [x] E4x4 / E4x4 Pre / TOPPING 设备名自动匹配
- [x] 兼容 Windows 下输入设备与输出设备分开枚举
- [x] E4x4 优先选择 96 kHz 公共采样率
- [x] 泛化 TOPPING 设备名增加“型号待确认”提示，避免误判
- [x] FFT 页面增加基波、幅度、THD 仪器读数
- [x] 原版 THD 算法改为谐波平方和开根号 / 基波
- [x] SA-440F5 7 步测试向导框架
- [x] 测试图片改为可替换资源槽，不把图片逻辑写死在代码里
- [x] Windows portable 打包规则预留 `guide_assets`

### 当前阻塞

GitHub-hosted runner 当前不可用，Actions Job 在任何 step 执行前即结束，表现为：

- `runner_id = 0`
- `steps = []`

因此后续 Windows Preview 不依赖 GitHub-hosted runner，优先改成本机一键构建。

---

## 3. P1：Windows 本机一键构建【最高优先级】

目标：不依赖 GitHub Runner，在普通 Windows 电脑本机稳定生成可直接运行的免安装 ZIP。

计划新增：

```text
scripts/
├─ build_windows_portable.ps1
├─ build_windows_portable.bat
├─ clean_build.bat
└─ check_build_env.ps1

dist/
└─ AudMeS-YYYY.MM.DD-cn-preview-win64.zip
```

### 构建脚本职责

- [x] 自动检测 MSYS2
- [x] 检查 MinGW64 GCC / CMake / Make / wxWidgets 3.2
- [x] 自动获取或检查 fast-cpp-csv-parser
- [x] 清理旧 build
- [x] CMake Release 构建
- [x] CPack 生成 ZIP
- [x] 自动复制 `guide_assets`
- [x] 输出 EXE 与 ZIP 的完整路径
- [x] 构建失败时给中文错误信息
- [x] 可选：构建完成后自动启动 AudMeS.exe

实现状态（2026-09-19）：脚本与文档已完成，Windows PowerShell 5 入口、清理流程及缺失环境提示已验证；
当前开发机未安装 MSYS2，下面的完整编译、打包和启动验收仍待在具备 MinGW64 工具链的 Windows 10/11 电脑执行。

### 验收

- Windows 10/11 x64
- 不安装 AudMeS
- 解压 ZIP 即可运行
- 新电脑不需要手工找 DLL
- ZIP 中包含所有 wxWidgets / MinGW runtime DLL
- ZIP 中包含 `guide_assets`

---

## 4. P2：主界面仪器化完善

当前是第一版结构重排，下一阶段重点是视觉与操作效率，而不是继续堆菜单。

### 总览页

- [ ] 顶部设备状态
- [ ] E4x4 Pre / 当前输入 / 当前输出 / 采样率
- [ ] Loopback 校准状态
- [ ] 当前 DUT
- [ ] 快速进入 FFT / THD / Sweep / 一键测试
- [ ] 最近一次测试摘要

### FFT / THD

- [ ] 基波频率
- [ ] 基波幅度
- [ ] H2 / H3 / H4 / H5
- [ ] THD
- [ ] THD+N
- [ ] SINAD
- [ ] SNR
- [ ] Noise Floor
- [ ] Peak Hold
- [ ] Average
- [ ] Hann / Blackman-Harris 等窗函数
- [ ] 32768 / 65536 / 131072 / 262144 点 FFT
- [ ] dBFS / Vrms 显示模式
- [ ] CSV 导出

### Sweep

- [ ] 起始频率
- [ ] 截止频率
- [ ] 点数
- [ ] 对数 / 线性扫频
- [ ] 输入电平
- [ ] 左右通道
- [ ] 归一化到 1 kHz
- [ ] -3 dB 自动寻找
- [ ] CSV 导出
- [ ] 图上 Marker

---

## 5. P3：E4x4 Pre 深度支持

### 设备识别

- [x] 名称自动匹配
- [x] 输入/输出分离枚举兼容
- [ ] 显示真实通道数
- [ ] 输入通道选择
- [ ] 输出通道选择
- [ ] 当前 API 显示
- [ ] 设备热插拔检测

### Windows 音频后端

当前 AudMeS 上游 Windows 构建主要使用 RtAudio 的 WASAPI / DirectSound 路径。

后续验证：

- [ ] WASAPI Exclusive 是否满足测量需求
- [ ] TOPPING 官方 ASIO 驱动识别
- [ ] 如果必要，为 Windows 构建加入 RtAudio ASIO 支持
- [ ] ASIO / WASAPI 测量结果对比
- [ ] buffer size 可选
- [ ] underrun / overrun 显示

> 在没有完成实机测试前，不把“ASIO 一定优于 WASAPI”写成固定结论，最终以 E4x4 Pre Loopback 测试结果决定默认后端。

---

## 6. P4：校准系统

准确测量前必须先知道 E4x4 本身的输出、输入和频响。

### Loopback 校准

- [ ] 一键 Loopback
- [ ] 1 kHz 输入/输出电平基准
- [ ] 频率响应校正
- [ ] 左右通道增益差
- [ ] 本底 THD
- [ ] 本底 Noise Floor
- [ ] 保存校准配置
- [ ] 记录校准时间

建议数据结构：

```text
calibration/
├─ E4x4_Pre_xxxxx.json
└─ loopback_reference.csv
```

后续 DUT 测试结果必须明确显示：

- Raw
- Calibrated
- Instrument Floor

避免把音频接口自身限制误判为 DUT 性能。

---

## 7. P5：SA-440F5 一键测试

当前向导固定拆成 7 步，后续继续在这个结构上做自动化，不再把所有功能挤在一个页面。

### Step 1：设备与安全检查

- E4x4 Pre 检测
- 48 V Phantom OFF 提示
- 采样率
- DUT 电源
- 接线确认

### Step 2：Loopback 基准

记录：

- 1 kHz 基波
- THD
- Noise Floor
- E4x4 输入/输出基线

### Step 3：1 kHz 增益

目标：

- 输入约 10 mVpp 差分
- DUT 输出约 1 Vpp
- 自动计算 V/V 与 dB
- 目标约 40 dB

### Step 4：Sweep

音频接口负责：

- 20 Hz–40 kHz

更高频率：

- 100 kHz–20 MHz 留给外部信号源 / 示波器 / VNA

软件中必须明确分界，避免让用户误以为 E4x4 可以验证 20 MHz 带宽。

### Step 5：THD

- 1 kHz
- 读取 H2 / H3 / THD
- 与 Loopback 基线比较
- 自动提示“接近仪器底线”或“DUT 主导”

### Step 6：Noise

- DUT 输入端短路
- FFT 平均
- 输出噪声
- RTI 折算
- 后续加入 nV/√Hz
- 使用 RSS 去除已知测量系统本底

### Step 7：报告

至少保存：

- DUT 名称
- 日期
- 音频接口
- 采样率
- 校准文件
- 增益
- Sweep
- THD
- Noise
- CSV
- 测试条件与备注

---

## 8. 测试指导图机制

本次不直接生成新图片。

程序按文件名加载：

```text
guide_assets/
├─ sa440f5_step_01.png
├─ sa440f5_step_02.png
├─ sa440f5_step_03.png
├─ sa440f5_step_04.png
├─ sa440f5_step_05.png
├─ sa440f5_step_06.png
└─ sa440f5_step_07.png
```

计划：

- [x] 资源槽结构
- [x] 图片缺失时使用程序内置占位图
- [ ] 将已确认的测试指导图逐步放入对应步骤
- [ ] 每一步允许“重新载入引导”
- [ ] 后续可增加“替换本步图片”
- [ ] 是否接入 AI 图片生成留到独立阶段，不与核心测试代码耦合

---

## 9. P6：测量算法增强

### THD

- [x] 修正为 RMS harmonic sum
- [ ] 基波插值，降低 FFT bin 不对齐误差
- [ ] 谐波搜索窗口而非单 bin
- [ ] 自动排除超过 Nyquist 的谐波
- [ ] H2–H10 独立显示

### THD+N

- [ ] 基波 notch / 排除
- [ ] 带宽可选：20 Hz–20 kHz 等
- [ ] A-weighting 可选
- [ ] THD+N dB / %

### Noise

- [ ] dBV/√Hz
- [ ] V/√Hz
- [ ] nV/√Hz RTI
- [ ] 1/f 区域
- [ ] 白噪声平台
- [ ] 50/60 Hz 与谐波 Marker
- [ ] RSS 仪器本底修正
- [ ] 输入增益参数

### Sweep

- [ ] 使用 Loopback 校准曲线补偿接口频响
- [ ] 1 kHz normalization
- [ ] 自动 -3 dB 点
- [ ] passband ripple

---

## 10. P7：SA-440F5 专用判定模板

后续在软件里提供“目标规格模板”，但必须区分：

- 原机目标
- 当前克隆功能样机目标
- 当前测量系统是否有能力验证

初步项目目标：

| 项目 | 目标 / 说明 |
|---|---|
| 增益 | 40 dB / 100× |
| 音频段 Sweep | E4x4 测 20 Hz–40 kHz |
| 全带宽 | 外部仪器验证至 20 MHz |
| THD | 1 kHz 小信号 |
| RTI Noise | 目标约 1.8 nV/√Hz；功能样机阶段可先用 ≤2.2 nV/√Hz |
| CMRR 10 kHz | 外部差分夹具配合 |
| CMRR 1 MHz | 不由 E4x4 单独完成 |

软件不能用音频接口的能力去“假验收”20 MHz / 1 MHz CMRR。

---

## 11. P8：可重复测试与报告

- [ ] Project / DUT 概念
- [ ] 测试 Session
- [ ] 保存设备和校准信息
- [ ] CSV
- [ ] JSON 原始数据
- [ ] 一键生成 HTML 报告
- [ ] PASS / FAIL 条件
- [ ] 测试截图
- [ ] 测试历史对比

建议目录：

```text
measurements/
└─ SA440F5_SN001/
   └─ 2026-09-18_223000/
      ├─ session.json
      ├─ gain.csv
      ├─ sweep.csv
      ├─ fft.csv
      ├─ noise.csv
      └─ report.html
```

---

## 12. P9：发布与版本策略

开发阶段：

```text
feature/instrument-ui-cn-sa440f5
        ↓
本机 Windows 编译
        ↓
Preview ZIP
        ↓
人工实测
        ↓
修复
        ↓
PR 合并 main
```

版本建议：

- `cn-preview1`：界面 + 流程验证
- `cn-preview2`：E4x4 实机 + 校准
- `cn-preview3`：FFT / THD / Noise 算法增强
- `v1.0-zxyh`：SA-440F5 测试流程稳定版

正式发布前至少验证：

- Windows 10
- Windows 11
- E4x4 Pre
- 无 E4x4 时的普通声卡兼容
- 中文字体
- 无 guide PNG 时不崩溃
- portable ZIP 在干净 Windows 环境运行

---

## 13. 下一步执行顺序

当前优先级固定为：

1. **P1 Windows 本机一键构建**
2. 编译并运行当前中文 Preview
3. 修 UI 布局 / 中文显示 / wxWidgets 编译错误
4. E4x4 Pre 实机识别
5. Loopback 基准
6. FFT / THD 实测
7. Sweep 实测
8. SA-440F5 实机接线流程
9. Noise Density / RTI
10. 自动报告
11. 再考虑更深层 ASIO 与自动化扩展

原则：

> 先让 Preview 在真实 Windows + E4x4 Pre 上稳定跑起来，再继续扩测量算法；不在未验证的 UI 壳上堆大量功能。
