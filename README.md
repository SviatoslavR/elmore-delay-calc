# Delay Predictor (Elmore Delay Calculator)

这是一个基于 C++ 的高性能 Elmore 延迟计算器。它能够解析 SPEF (Standard Parasitic Exchange Format) 文件，结合网表信息构建 RC 树，并使用 OpenMP 并行计算每个节点的延迟。项目还包含一个 Python 脚本用于验证计算结果的准确性。

## 📋 环境依赖

### C++ 部分
*   **编译器**: 支持 C++17 的编译器 (如 GCC, MSVC, Clang)。
*   **构建工具**: CMake (3.9 或更高版本)。
*   **依赖库**:
    *   OpenMP (用于多线程并行加速)。
    *   Windows API (`psapi.h`, 仅在 Windows 下用于内存统计)。

### Python 部分 (测试脚本)
*   Python 3.x
*   NumPy
*   Matplotlib

## 🛠️ 编译与构建

### 手动编译
```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## 🚀 程序用法

### 1. 延迟计算主程序 (`course.exe`)

编译生成的可执行文件位于 `build/` 目录下。

**基本命令格式：**
```bash
./course.exe --spef_num <ID> --file_path <DataDir> --feature_path <OutputDir> --memory_usage
```

**参数说明：**

| 参数 (长/短) | 是否必须 | 默认值 | 说明 |
| :--- | :--- | :--- | :--- |
| `--spef_num` / `-m` | **是** | 无 | 指定测试组编号 (例如 `0`, `1`)。程序将读取 `Group0.spef` 等文件。 |
| `--file_path` / `-f` | 否 | `./Data` | 数据文件的根目录路径。 |
| `--feature_path` / `-e` | 否 | `./features` | 计算结果输出的目录路径。 |
| `--memory_usage` / '-s' | 否 | `false` | 是否打印内存使用情况。 |

**文件存放结构要求：**

假设 `--file_path` 设置为 `../Data`，程序期望的目录结构如下：

```text
Data/
├── netlist_info.txt          # 网表信息文件 (必须存在)
├── SPEF/
│   └── Group0.spef           # SPEF 寄生参数文件 (文件名需匹配 spef_num)
└── delay_data/
    └── Group0.txt            # (可选) 参考延迟数据，用于程序内部读取
```

**输出结果：**
程序将在 `--feature_path` 指定的目录下生成 `delay<spef_num>.txt` (例如 `features/delay0.txt`)。

---

### 2. 结果验证脚本 (`test.py`)

用于比较 C++ 程序的计算结果与 Golden (标准) 结果的误差，并生成误差分布直方图。

**命令格式：**
```bash
python test.py --path <ComputedResult> --golden <GoldenResult>
```

**参数说明：**
*   `--path`: C++ 程序生成的计算结果文件路径 (例如 `features/delay0.txt`)。
*   `--golden`: 标准参考结果文件路径 (例如 `Data/delay_data/Group0.txt`)。

**输出内容：**
*   控制台打印：平均误差 (Mean)、最大误差 (Max)、偏差 (Bias) 等统计信息。
*   图片文件：
    *   `smaller_50_error_distribution.png`: Golden Delay <= 50ps 的误差分布。
    *   `larger_50_error_distribution.png`: Golden Delay > 50ps 的误差分布。

## 💾 内存开销提示

本程序针对速度进行了优化，采用了**内存池 (Memory Pooling)** 和 **全量加载** 的策略。

1.  **SPEF 解析**: 程序会一次性将整个 SPEF 文件解析并加载到内存中。对于大型 SPEF 文件 (数百 MB 或 GB 级别)，内存占用会显著增加。
2.  **RC 树构建**: 为了加速图的遍历和延迟计算，程序使用了 `std::deque` 作为内存池来存储节点信息，避免了频繁的 `new/delete` 开销，但这会使得内存占用略高于按需分配。
3.  **监控**: 程序运行结束时可选择在终端输出当前的内存使用情况：
    *   **Physical (Working Set)**: 实际占用的物理内存。
    *   **Commit**: 程序申请的总内存 (包含虚拟内存)。

**建议**: 在处理大型测试用例时，请确保机器有足够的可用内存。（在course.cpp中将print_memory_usage()注释去掉即可监控内存使用情况）
