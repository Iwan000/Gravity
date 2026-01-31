# Force System 调试指南 - 没有吸引力问题排查

## 快速排查清单

### 1. 确认代码已编译并加载

**步骤**：
1. 关闭 UE 编辑器
2. 重新编译项目：
```bash
cd /mnt/e/Unreal_Engine_Files/Gravity_test
<UE_ROOT>/Engine/Build/BatchFiles/Build.bat Gravity_testEditor Win64 Development -project="$(pwd)/Gravity_test.uproject"
```
3. 等待编译成功
4. 重新打开编辑器

**验证**：
- Content Browser → Filters → C++ Classes → 能看到：
  - `ForceManagerSubsystem`
  - `GravitySourceBlackHole`
  - `GravitySourceWhiteHole`
  - `GravitySourceWind`

---

### 2. 添加调试日志

我们需要添加日志来确认系统是否在运行。

#### 修改 ForceManagerSubsystem.cpp

在 `Initialize` 函数中添加日志（第 12 行后）：

```cpp
void UForceManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UE_LOG(LogTemp, Warning, TEXT("✅ ForceManagerSubsystem Initialized!"));
}
```

在 `RegisterSource` 函数中添加日志（第 62 行后）：

```cpp
void UForceManagerSubsystem::RegisterSource(AGravitySourceBase* Source)
{
    if (!IsValid(Source))
    {
        return;
    }

    Sources.AddUnique(Source);
    UE_LOG(LogTemp, Warning, TEXT("✅ Source Registered: %s"), *Source->GetName());
}
```

在 `AddContribution` 函数中添加日志（第 92 行后）：

```cpp
void UForceManagerSubsystem::AddContribution(UPrimitiveComponent* ReceiverComp, const FVector& Accel, uint32 TypeMask, FName DebugName)
{
    // ... 现有代码 ...

    Accumulator.SumAccel += Accel;

    UE_LOG(LogTemp, Warning, TEXT("✅ AddContribution: Receiver=%s, Accel=%s"),
           *ReceiverComp->GetName(), *Accel.ToString());
}
```

#### 修改 GravitySourceBase.cpp

在 `BeginPlay` 中添加日志（第 48 行后）：

```cpp
if (UWorld* World = GetWorld())
{
    ManagerRef = World->GetSubsystem<UForceManagerSubsystem>();
    if (ManagerRef.IsValid())
    {
        ManagerRef->RegisterSource(this);
        UE_LOG(LogTemp, Warning, TEXT("✅ Source BeginPlay: %s registered to Manager"), *GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ Source BeginPlay: Failed to get Manager!"));
    }
}
```

在 `Tick` 函数中添加日志（第 72 行后）：

```cpp
void AGravitySourceBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!ManagerRef.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("❌ Source Tick: ManagerRef is invalid!"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("✅ Source Tick: %s, ReceiversInRange=%d"),
           *GetName(), ReceiversInRange.Num());

    // ... 其余代码
}
```

在 `HandleBeginOverlap` 中添加日志（第 178 行后）：

```cpp
if (IsValidReceiver(OtherComp))
{
    ReceiversInRange.Add(OtherComp);
    UE_LOG(LogTemp, Warning, TEXT("✅ Overlap Begin: %s added to ReceiversInRange"), *OtherComp->GetName());
    return;
}
```

---

### 3. 重新编译并测试

```bash
# 重新编译
Build.bat Gravity_testEditor Win64 Development -project="..."
```

启动编辑器，Play 测试关卡。

---

## 检查 Output Log

按 `Window → Developer Tools → Output Log`，查看日志输出。

### 预期的正常日志流程：

```
✅ ForceManagerSubsystem Initialized!
✅ Source BeginPlay: BP_BlackHole_C_0 registered to Manager
✅ Source Registered: BP_BlackHole_C_0
✅ Overlap Begin: StaticMeshComponent_0 added to ReceiversInRange
✅ Source Tick: BP_BlackHole_C_0, ReceiversInRange=1
✅ AddContribution: Receiver=StaticMeshComponent_0, Accel=(X=XXX Y=XXX Z=XXX)
```

### 问题诊断：

#### ❌ 如果看不到 "ForceManagerSubsystem Initialized!"
**原因**：Subsystem 没有创建
**解决**：
- 确认 `.cpp` 文件在项目中编译了
- 检查 `Gravity_test.Build.cs` 是否包含了模块依赖

#### ❌ 如果看不到 "Source Registered"
**原因**：Source 的 BeginPlay 没有执行
**解决**：
- 确认 Source Actor 真的在关卡里（Outliner 中能看到）
- 确认是从 C++ 类派生的 Blueprint

#### ❌ 如果看到 "ReceiversInRange=0"
**原因**：Overlap 没有触发
**解决**：见下面 "Overlap 问题排查"

#### ❌ 如果看到 "ManagerRef is invalid"
**原因**：Manager 在 Tick 时已经被销毁或未初始化
**解决**：检查 Subsystem 生命周期，确认在 PIE 模式下测试

---

## 关卡设置验证

### 检查黑洞设置

选中关卡中的黑洞 Actor，在 Details 面板确认：

```
✅ RangeCollision (SphereComponent)
   ├─ Sphere Radius = 1500.0 (或更大)
   ├─ Collision Enabled = Query Only
   ├─ Collision Object Type = WorldDynamic
   ├─ Generate Overlap Events = True
   └─ Collision Responses: Overlap (不要是 Ignore 或 Block)

✅ Class Defaults
   ├─ Strength = 3000000.0
   ├─ MinRadius = 150.0
   ├─ SourceType = BlackHole
```

### 检查物理对象（Cube）设置

选中 Cube，确认：

```
✅ Static Mesh Component
   ├─ Simulate Physics = TRUE (非常重要!)
   ├─ Collision Enabled = Query and Physics
   ├─ Collision Presets = PhysicsActor (或自定义但必须 Overlap WorldDynamic)
   └─ Mass = 10.0 kg (或任意非零值)

✅ Transform
   └─ Location 在黑洞的 Sphere Radius 范围内
```

**关键点**：Cube 必须能与黑洞的 RangeCollision **Overlap**，而不是 Block！

---

## Overlap 问题排查

### 可视化碰撞体积

运行游戏时按：
- `Alt + C`（显示碰撞）
- 或控制台输入：`show collision`

**预期**：
- 能看到黑洞的绿色球体（Sphere）
- Cube 在球体内部或边缘

### 测试 Overlap 是否触发

在 `GravitySourceBase::HandleBeginOverlap` 的**最开头**添加：

```cpp
void AGravitySourceBase::HandleBeginOverlap(...)
{
    UE_LOG(LogTemp, Warning, TEXT("🔔 HandleBeginOverlap called! OtherActor=%s, OtherComp=%s"),
           OtherActor ? *OtherActor->GetName() : TEXT("NULL"),
           OtherComp ? *OtherComp->GetName() : TEXT("NULL"));

    // ... 现有代码
}
```

**如果这个日志都没有输出**：
- Overlap 事件根本没触发
- 检查碰撞设置（上面的清单）
- 确认 `OnComponentBeginOverlap` 已绑定（应该在 BeginPlay 中自动绑定）

---

## 物理模拟验证

### 确认 Cube 真的在模拟物理

1. Play 游戏
2. 按 `F8` 暂停
3. 选中 Cube
4. Details → Physics → Simulate Physics = 应该是勾选状态

**测试**：
- 先**不放黑洞**，只放 Cube
- Play → Cube 应该向下掉（受世界重力）
- 如果 Cube 不掉落 → 物理模拟没开启

---

## Strength 参数测试

可能是 Strength 太小了，尝试：

```
Strength = 50000000.0  (5000 万，非常强的力)
```

如果这样还没反应，说明不是参数问题。

---

## 检查 Manager 的 DefaultSettings

创建一个临时 Blueprint Actor 来检查 Manager 状态：

1. 创建 Blueprint Actor → `BP_DebugManager`
2. Event Graph：

```
Event BeginPlay
├─ Get World Subsystem (ForceManagerSubsystem)
├─ Is Valid?
│   ├─ True → Print String "Manager is Valid"
│   │         └─ Get Default Settings → Break Struct
│   │             ├─ MinAccel → Print String
│   │             ├─ MaxAccel → Print String
│   │             └─ MassMode → Print String
│   └─ False → Print String "Manager is NULL!"
```

**预期输出**：
```
Manager is Valid
MinAccel = 0.0
MaxAccel = 999999.0
MassMode = Physical
```

如果 MinAccel 太大或 MaxAccel 太小，会过滤掉力。

---

## 最小可复现测试

### 创建干净的测试场景

1. **新建空白关卡**
2. **只放 2 个物体**：
   - 一个黑洞（直接用 C++ 类，不用 Blueprint）
   - 一个 Cube

### 黑洞设置（直接在 Details）

```
Actor: AGravitySourceBlackHole (直接从 C++ Classes 拖入)
Location: (0, 0, 200)
Strength: 10000000.0
MinRadius: 150.0

RangeCollision:
  Sphere Radius: 2000.0
```

### Cube 设置

```
Static Mesh Actor
Location: (500, 0, 200)  (黑洞右侧 5 米)

StaticMeshComponent:
  Simulate Physics: TRUE
  Collision Preset: PhysicsActor
  Mass: 10.0
```

### Play 并观察

- Cube 应该立即开始向黑洞加速
- Output Log 应该疯狂输出 "AddContribution" 日志

---

## 终极诊断：手动测试代码路径

创建一个测试 Actor，手动调用 Manager：

```cpp
// BP_ManualForceTest (Blueprint)
Event BeginPlay
├─ Get World Subsystem (ForceManagerSubsystem)
├─ Get Reference to Cube Component (拖入场景中的 Cube)
└─ Event Tick
    └─ Manager → Add Contribution
        ├─ Receiver Comp = Cube Component
        ├─ Accel = (1000, 0, 0)  (向右 1000 cm/s²)
        ├─ Type Mask = 1
        └─ Debug Name = "ManualTest"
```

**如果手动调用也不工作** → Manager 的 ApplyToPhysics 有问题
**如果手动调用有效** → Source 的 CalculateContribution 或 Overlap 检测有问题

---

## 常见原因总结

### Top 5 最可能的问题：

1. **Simulate Physics 没开**（80% 的情况）
2. **Overlap 碰撞设置错误**（10%）
3. **代码没重新编译/编辑器没重启**（5%）
4. **Cube 不在黑洞范围内**（3%）
5. **Manager 没初始化**（2%）

---

## 下一步

请添加上面的调试日志，重新编译，然后：

1. 运行测试关卡
2. 把 **Output Log 的内容**发给我
3. 截图 **黑洞的 Details 面板**
4. 截图 **Cube 的 Details 面板**

我会帮你精确定位问题！
