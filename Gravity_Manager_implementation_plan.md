# 力管理系统（Force System）实施设计文档（Unreal Engine）

> 目标：让"力"成为游戏的核心玩法语言，并且做到 **可控、可扩展、可调试**。  
> 约束与规模（根据你提供的数据）：
> - Sources：常见 3–5 个，极端 ~15 个
> - Receivers：以 100 个上限设计
> - Source 不频繁移动
> - 早期保留 UE 世界重力，后期大概率接管（替换成自定义重力 Source）
> - 特殊 Receiver 数量"很多"（需要一个 **低绑定成本** 的自定义路径）
> - 未来可能加入遮挡/Trace，但当前不做（预留接口位）

---

## 1. 核心原则（Why 这样做）

### 1.1 统一思路：两阶段流水线
系统按物理步（Physics Tick）走一个固定流水线：

1) **Discover（发现受力对象）**：由 Source 的 Collision Overlap 维护"范围内 Receivers"
2) **Gather（收集贡献）**：Source 计算每个 Receiver 的 contribution（建议用"加速度 accel"），提交给 Manager 累加
3) **Resolve（合力裁决 / Goalkeeper）**：Manager 对每个本帧被触达的 Receiver 只执行一次非线性规则（deadzone/clamp/过滤/优先级等）
4) **Apply（一次施加）**：对普通物理对象直接 AddForce / AddAcceleration；对特殊对象走自定义接口

> 为什么必须把"裁剪/阈值/非线性规则"放到 Resolve？  
> 因为 `clamp(a+b)` 通常 **不等于** `clamp(clamp(a)+b)`，在 Gather 阶段做裁剪会让结果强烈依赖遍历顺序，导致不可控与不一致。

### 1.2 为什么不使用 "count == num_sources 才 apply"
不要用 `count` / `num_sources` 这类"等所有 Source 报告完再 apply"的门闩逻辑。原因：
- Source 每帧触达的 Receiver 集合不同，某个 Source 本帧可能 0 接触但并不意味着你不能 apply
- Source 动态生成/销毁时 num_sources 会变化
- Tick 顺序不保证，count 容易产生时序 bug

✅ 正确做法：**Manager 自己每个 Physics Tick 固定执行一次 Resolve+Apply**。  
Sources 只负责在本 Tick 的前半段不断 AddContribution（累加器保证线性可交换）。

---

## 2. 总体架构与职责边界（What & Who）

### 2.1 三类对象
- **Gravity_Source（力源）**：黑洞、白洞、风场等  
  - 负责：维护范围内 Receiver 列表（Overlap Enter/Exit），计算 contribution，并提交给 Manager  
  - 不负责：裁剪/阈值/合力决策/最终施加（避免顺序依赖）
- **ForceManager（中转站 + Goalkeeper + Apply）**  
  - 负责：累加 contribution；对 touched receivers 统一 Resolve；把最终结果一次性 Apply（或派发给特殊 Receiver）
- **Receiver（受力对象）**
  - **普通 Receiver：零绑定**（不需要你写 Receiver 类）  
    - 只要是 `Simulate Physics = true` 的 `UPrimitiveComponent`，就能自动接收力  
  - **特殊 Receiver：低成本扩展**  
    - 大量特殊 Receiver 建议通过 "轻量组件 + 接口" 的组合解决（而不是每个都写一个复杂派生类）

---

## 3. 数据模型（Data Structures）

### 3.1 Contribution：统一使用"加速度 accel"
内部统一使用加速度向量（方向+大小）：

```cpp
struct FForceContribution
{
    FVector Accel;            // 本次贡献（m/s^2）
    int32 Priority = 0;        // 可选：优先级（数值大更高）
    bool bOverride = false;    // 可选：是否覆盖型（强制替换合力）
    uint32 TypeMask = 0;       // 可选：力类型位掩码（黑洞/白洞/风/自定义...）
    FName DebugName;           // 可选：调试来源名
};
```

> 统一用 accel 的原因：
> - 玩法更可控（你可以决定是否受质量影响）
> - Apply 阶段可选择：`Force = Mass * Accel`（质量影响）或 `AddForce(Accel, bAccelChange=true)`（忽略质量）

### 3.2 Accumulator：Gather 阶段只做"线性统计"

```cpp
struct FForceAccumulator
{
    FVector SumAccel = FVector::ZeroVector;
    bool bTouched = false;

    // 可选：override / priority
    bool bHasOverride = false;
    int32 MaxPrioritySeen = MIN_int32;
    FVector OverrideAccel = FVector::ZeroVector;

    // 可选：为特殊 Receiver 预留 breakdown（按需开启）
    // FVector PerTypeSum[NUM_TYPES];
};
```

### 3.3 ReceiverSettings：让"特殊 Receiver 很多"也不痛

你说特殊 Receiver "很多"，所以我们需要一个**低维护成本**的方式：
不让每个特殊对象写一堆方法，而是尽量用数据驱动。

```cpp
UENUM()
enum class EMassMode : uint8
{
    Physical,      // F = m * a（质量影响）
    AccelChange,   // 直接施加加速度变化（忽略质量）
    Hybrid         // 预留：可选曲线/指数（后期再实现）
};

UENUM()
enum class EReceiverMode : uint8
{
    DefaultPhysicsApply,  // 普通物体：Resolve 后直接 Apply
    IgnoreAllForces,      // 不受 Source 力
    ConsumeOnly,          // 不施加物理力，但把合力发给自定义事件
    ConsumeAndApply       // 既派发事件，也施加（少数情况）
};

struct FReceiverForceSettings
{
    float MinAccel = 0.0f;        // deadzone（|a| < MinAccel -> 0）
    float MaxAccel = 999999.0f;   // clamp
    uint32 AcceptTypeMask = 0xFFFFFFFF; // 可选：过滤类型
    EMassMode MassMode = EMassMode::Physical;
    EReceiverMode ReceiverMode = EReceiverMode::DefaultPhysicsApply;

    // 可选：平滑/阻尼（让手感更稳）
    float Damping = 0.0f;

    // 可选：是否请求 breakdown（如果很多特殊 receiver 需要分解）
    bool bRequestBreakdown = false;
};
```

实现方式：给特殊对象（Actor 或 Component）挂一个轻量组件 `UReceiverSettingsComponent` 只存这些参数。
普通对象不挂任何组件，自动用 Manager 的 DefaultSettings。

---

## 4. Unreal Engine 落地方案（How in UE）

### 4.1 Manager 放在哪里（推荐两种，选一种落地）

**推荐 A：World Subsystem（更"框架化"）**

* `UWorldSubsystem` 或 `UTickableWorldSubsystem` 实现 Manager
* 好处：全关卡通用、无需关卡摆放
* 代价：对新开发者稍微抽象一些

**推荐 B：Level 里的单例 Actor（更直观）** ✅（新开发者更容易实现）

* 放一个 `AForceManagerActor` 到关卡
* Sources 在 `BeginPlay` 里 `GetActorOfClass` 找到它并注册
* 好处：直观、易调试、易在 Editor 看见
* 代价：每关卡要放一个（可以做成 GameMode 自动生成）

> 为了让"新开发者照文档就能做"，建议先用 **B**，后续想升级为 Subsystem 再迁移。

---

## 5. 类设计（Class Specs）

> 注意：下面将 Receiver 的"默认实现"定位为 **UPrimitiveComponent**（而不是 Actor）。
> 原因：UE 物理真正施加力的是 PrimitiveComponent，Actor 可能有多个组件且只有部分模拟物理。

---

### 5.1 `AGravitySourceBase`（力源基类）

**职责**：维护范围内 receivers、计算 contribution、提交给 manager。

**成员变量：**

* `FName SourceId`
* `FName SourceType`
* `float Strength`
* `UShapeComponent* RangeCollision`（Sphere/Box 等）
* `TSet<TWeakObjectPtr<UPrimitiveComponent>> ReceiversInRange`
* `TWeakObjectPtr<AForceManagerActor> ManagerRef`

**关键方法：**

* `virtual FVector CalculateContribution(UPrimitiveComponent* ReceiverComp, float Dt) const = 0;`
* `bool IsValidReceiver(UPrimitiveComponent* Comp) const;`
  * `Comp != nullptr`
  * `Comp->IsSimulatingPhysics() == true`
  * 如果存在 settings 且 `ReceiverMode == IgnoreAllForces`，则 false

**事件与生命周期：**

* `BeginPlay()`：
  * 绑定 overlap：`OnComponentBeginOverlap`, `OnComponentEndOverlap`
  * 找到 Manager 并 `RegisterSource(this)`
* `EndPlay()` / `Destroyed()`：
  * `UnregisterSource(this)`
  * 清理 ReceiversInRange（weak ptr 可省略强制清理）
* `OnBeginOverlap(OtherComp)`：
  * 若 `IsValidReceiver(OtherComp)` 则加入 `ReceiversInRange`
* `OnEndOverlap(OtherComp)`：
  * 从 `ReceiversInRange` 移除

**Tick（建议 TG_PrePhysics）**：

* 遍历 `ReceiversInRange`：
  * 若 weak ptr 失效，剔除
  * `accel = CalculateContribution(receiver, dt)`
  * `Manager->AddContribution(receiver, accel, TypeMask, ...)`

> 你问"每算完一个 receiver 就发 manager，还是全部算完再统一发？"
> **默认建议：每算完一个就 AddContribution**（即时累加）。
> - 不会破坏可控性（因为 AddContribution 不做 clamp）
> - 省内存、实现简单
>   若以后 Blueprint 调用频繁导致性能问题，再改成 `AddContributionsBatch(Array)` 优化。

---

### 5.2 具体 Source 示例：黑洞 / 白洞 / 单向风

#### 黑洞（吸引）

* 方向：`dir = normalize(SourcePos - ReceiverPos)`
* 强度：建议 `strength / max(dist, epsilon)^2` + 上限 clamp
* 可选：在近距离做软化（避免爆炸）

#### 白洞（排斥）

* 方向：`dir = normalize(ReceiverPos - SourcePos)`（反向）
* 强度类似黑洞

#### 单向风

* 方向固定（不依赖 SourcePos）
* 强度可按距离衰减或常量

> 注意：把"上限/爆炸保护"放在 Source 的 CalculateContribution 里是合理的（这是 Source 自身的物理定义）。
> 把"合力 deadzone / 合力 max clamp / 类型过滤"放在 Manager Resolve（这是系统规则）。

---

### 5.3 `AForceManagerActor`（Manager）

**职责**：收集贡献、Goalkeeper、Apply（一次性）。

**成员变量：**

* `TArray<TWeakObjectPtr<AGravitySourceBase>> Sources`
* `TMap<TWeakObjectPtr<UPrimitiveComponent>, FForceAccumulator> AccMap`
* `TArray<TWeakObjectPtr<UPrimitiveComponent>> TouchedReceivers`
* `FReceiverForceSettings DefaultSettings`

**核心方法：**

#### `RegisterSource(Source)`

* Add 到 Sources（weak ptr）
* 不需要维护 num_sources/计数门闩

#### `AddContribution(ReceiverComp, ContributionMeta...)`

* 若 receiver 不在 AccMap：创建 Accumulator
* 若 accumulator.bTouched == false：
  * 标记 bTouched
  * push 到 `TouchedReceivers`
* 累加（只做线性统计，不做 clamp）：
  * 若 bOverride：记录 max priority 的 override
  * 否则 `SumAccel += Accel`

#### `Tick（TG_PrePhysics） -> ResolveAndApply(Dt)`

* 遍历 `TouchedReceivers`：
  * 获取 receiver settings（若没有组件则用 DefaultSettings）
  * 根据 settings 做 Resolve（goalkeeper）
  * 根据 ReceiverMode：
    * DefaultPhysicsApply：Apply 到物理
    * ConsumeOnly：派发事件，不 Apply
    * ConsumeAndApply：两者都做
    * IgnoreAllForces：理论上不会进入 touched，但这里也可直接 skip
* 帧末清理：
  * 对 touched receivers 从 AccMap 移除（或重置）
  * 清空 touched list

---

## 6. Resolve（Goalkeeper）规则建议（可直接实现）

### 6.1 统一 Resolve 伪代码

```cpp
FVector ResolveFinalAccel(const FForceAccumulator& Acc, const FReceiverForceSettings& S, float Dt)
{
    FVector A = FVector::ZeroVector;

    // 1) 选择 override 或 sum
    if (Acc.bHasOverride)
        A = Acc.OverrideAccel;
    else
        A = Acc.SumAccel;

    // 2) deadzone（用 length squared 避免 sqrt）
    if (A.SizeSquared() < S.MinAccel * S.MinAccel)
        return FVector::ZeroVector;

    // 3) clamp 最大值（只有超限时才 sqrt）
    const float Max2 = S.MaxAccel * S.MaxAccel;
    const float Len2 = A.SizeSquared();
    if (Len2 > Max2)
    {
        const float Len = FMath::Sqrt(Len2);
        A *= (S.MaxAccel / Len);
    }

    // 4) 可选平滑（Damping > 0 时）
    // A = Lerp(PrevA, A, 1 - exp(-S.Damping * Dt))

    return A;
}
```

### 6.2 Apply：质量如何处理（早期建议默认 Physical）

你说质量可能会带来一定影响，但目前不确定具体玩法。建议：

* **默认**：`MassMode = Physical`（F = m * a）
* 对特殊对象可改成 `AccelChange`（忽略质量）做对比测试

Apply 伪代码：

```cpp
void ApplyToPhysics(UPrimitiveComponent* Comp, const FVector& FinalAccel, const FReceiverForceSettings& S)
{
    if (!Comp || !Comp->IsSimulatingPhysics()) return;

    switch (S.MassMode)
    {
        case EMassMode::Physical:
        {
            const float Mass = Comp->GetMass();
            const FVector Force = FinalAccel * Mass;
            Comp->AddForce(Force);
            break;
        }
        case EMassMode::AccelChange:
        {
            // 直接作为加速度变化施加（忽略质量）
            Comp->AddForce(FinalAccel, NAME_None, /*bAccelChange=*/true);
            break;
        }
        case EMassMode::Hybrid:
        {
            // 预留：后期可以加入质量指数、曲线等
            break;
        }
    }
}
```

---

## 7. "大量特殊 Receiver"怎么低成本实现（重点）

你说特殊 receiver 的比例"很多"。这意味着我们必须避免：

* 每个特殊对象都写一个复杂派生类
* 每个对象都手动绑定很多函数

✅ 推荐方案：**数据组件 + 接口（可蓝图实现）**

### 7.1 `UReceiverSettingsComponent`（轻量数据组件）

* 存 `FReceiverForceSettings`
* 可选：暴露 `bRequestBreakdown`（如果特殊对象需要来源分解）

开发者流程：

* 普通物体：无需组件
* 特殊物体：加一个组件并调整枚举/阈值即可

### 7.2 `UForceConsumerInterface`（特殊逻辑接口）

让特殊对象（Actor）可以用蓝图实现回调，例如：

* `OnForceResolved(FVector FinalAccel, float Dt, OptionalBreakdownData)`
* 特殊对象可以：
  * 播放动画
  * 触发机关
  * 自己决定移动方式（不走物理）
  * 或仅用于 UI/FX 表现

Manager 在 Resolve 后：

* 如果 `ReceiverMode == ConsumeOnly / ConsumeAndApply`：
  * 调用接口回调
* 再根据 mode 决定是否 Apply

> 这样"特殊对象很多"也不会麻烦：
> - 大部分只需要改设置（组件）
> - 真正要自定义行为的才实现接口（蓝图即可）

---

## 8. Tick 顺序与一致性（Determinism-ish）

你希望尽量一致性。需要明确：

* Chaos 物理并不保证跨平台完全确定性（浮点、并行、子步等），但我们可以做"玩法层的一致性增强"。

### 8.1 推荐 TickGroup

* `AGravitySourceBase`：`TG_PrePhysics`
* `AForceManagerActor`：`TG_PrePhysics`，并且 **在 Sources 之后执行**

### 8.2 如何保证 Manager 在 Sources 之后

两种方式（二选一）：

**方式 A：Tick Prerequisite（推荐）**

* Source 在 BeginPlay 时：
  * `Manager->AddTickPrerequisiteActor(this)`（或 Manager 依赖 Source）
* 让 Manager tick 发生在所有 Source tick 后

**方式 B：Manager 放到更晚的 TickGroup**

* Source：PrePhysics
* Manager：DuringPhysics / PostPhysics（谨慎）

> 不推荐 DuringPhysics 直接 AddForce（可能不安全/不符合预期），所以还是推荐方式 A。

---

## 9. 世界重力策略（早期保留，后期接管）

### 9.1 早期（保留 UE 世界重力）

* UE 会自动施加世界重力（baseline）
* 你的系统只负责额外贡献（黑洞/白洞/风等）

优点：实现快、稳定、省心
代价：系统不是闭环（存在"看不见的 baseline"）

### 9.2 后期（接管世界重力）

当你希望"力语言完全统一"时：

* 将 World Gravity 设为 0（或对相关物体 Disable Gravity）
* 创建 `GlobalGravitySource`（向下恒定 accel），也走同一套 pipeline
* 这样 "所有运动原因都在你的系统里" → 可调试性与可设计性大幅提升

---

## 10. 遮挡/Trace 预留位置（当前不实现）

你说目前没有遮挡，但要预留位置。建议：

* 在 `AGravitySourceBase::CalculateContribution` 内预留：
  * `bool bUseOcclusion = false;`
  * 若开启则对 `SourcePoint -> Receiver` 做 LineTrace，决定是否衰减/阻断
* 这类计算昂贵，后期可以只对：
  * 距离近的
  * 强度大的
  * 特定类型 Source
    做 trace

---

## 11. 性能预估与优化路线

在你的规模下：

* 最坏情况：15 sources × 每个覆盖 100 receivers = 1500 次贡献计算 / tick
  这在 C++ 下非常轻；在蓝图下也通常可接受（但要避免过多蓝图函数调用）。

### 11.1 默认就做的优化（不增加复杂度）

* `ReceiversInRange` 用 Overlap 维护 → 不全世界扫描
* Manager 只遍历 `TouchedReceivers` → 没受力的物体不做空计算
* Accumulator 只做线性累加 → cache 友好，避免频繁分配

### 11.2 可能的后期优化（按需）

* Source 批量提交：`AddContributionsBatch(TArray<...>)`（减少蓝图调用开销）
* 可选 breakdown 只对 bRequestBreakdown 的 receiver 记录
* 需要更大规模时再引入空间哈希/网格索引（当前不必要）

---

## 12. 开发任务清单（Step-by-step）

> 目标：让一个新开发者按步骤做完就能跑通框架。

### Step 1：创建 Manager

1. 新建 `AForceManagerActor`（或 Subsystem）
2. 开启 Tick，设为 `TG_PrePhysics`
3. 实现：
   * `RegisterSource/UnregisterSource`
   * `AddContribution`
   * `ResolveAndApply`
   * touched 清理逻辑
4. 加入 DefaultSettings（MinAccel/MaxAccel/MassMode 等）

### Step 2：创建 ReceiverSettingsComponent + ForceConsumerInterface

1. `UReceiverSettingsComponent`：只存 `FReceiverForceSettings`
2. `UForceConsumerInterface`：定义 `OnForceResolved(FVector, float)` 等回调
3. 在 Manager Resolve 阶段读取组件与接口，走对应模式

### Step 3：创建 Source 基类

1. `AGravitySourceBase`：
   * 一个 CollisionComponent（Sphere/Box）
   * Overlap enter/exit 维护 `ReceiversInRange`
2. BeginPlay 注册到 Manager
3. Tick 遍历 `ReceiversInRange`，计算 contribution 并 AddContribution

### Step 4：实现第一个具体 Source（黑洞）

1. 继承 SourceBase
2. 实现 `CalculateContribution`（1/r² + 上限 + epsilon）
3. 放到关卡里测试：
   * 丢一个 Simulate Physics 的 cube → 自动受力

### Step 5：实现白洞/风场

* 白洞：方向反转
* 风场：方向固定

### Step 6：实现特殊 receiver 示例（验证扩展路径）

* 例 A：IgnoreAllForces 的物体（仍模拟物理但不受 source 力）
* 例 B：ConsumeOnly 的机关（不受物理力，但能收到合力触发动画/事件）
* 例 C：ConsumeAndApply 的对象（既动又触发效果）

### Step 7：Debug 工具

* `DrawDebugLine`：显示合力方向
* 打印：TouchedReceiver 数、总贡献数、每个 source 覆盖数

---

## 13. 常见坑与规避（Very Practical）

1. **把裁剪放在 Gather** → 结果依赖遍历顺序，手感漂移
   ✅ 只在 Resolve 做 clamp/deadzone

2. **Receiver 用 Actor 而不是 PrimitiveComponent** → 施力对象不对，质量/物理体不准确
   ✅ 用 `UPrimitiveComponent*` 做 receiver 句柄

3. **Manager 完全不 Tick，依赖 count 门闩** → 时序 bug、Source 0 receiver 时 apply 不发生
   ✅ Manager 每个 PrePhysics Tick 固定 Resolve+Apply

4. **Overlap 列表残留无效引用** → 崩溃或无效调用
   ✅ 存 `TWeakObjectPtr` 并在遍历时剔除无效项

5. **蓝图里 per-receiver 频繁调用 manager** → 性能抖动
   ✅ 默认即时 AddContribution（C++），蓝图多时再做批量接口