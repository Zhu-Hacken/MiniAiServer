# MiniAiServer

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

> **语言 / Language**：本文档提供 [中文](#项目简介) 与 [English](#introduction) 两个版本。

> 基于 [ModularNetServer](https://github.com/Zhu-Hacken/ModularNetServer) 的轻量级 AI 对话服务器，使用 ONNX Runtime 在 CPU 上本地推理 Microsoft **Phi-3-mini-4k-instruct** 模型，支持普通对话与 WebSocket 流式输出。

---

## 目录

- [项目简介](#项目简介)
- [特性](#特性)
- [整体架构](#整体架构)
- [目录结构](#目录结构)
- [核心模块说明](#核心模块说明)
- [依赖环境](#依赖环境)
- [模型准备](#模型准备)
- [编译与运行](#编译与运行)
- [接口说明](#接口说明)
- [前端页面](#前端页面)
- [设计亮点](#设计亮点)
- [注意事项](#注意事项)

---

## 项目简介

MiniAiServer 是 [ModularNetServer](https://github.com/Zhu-Hacken/ModularNetServer) 的一个上层应用，在其模块化高性能网络服务器框架之上，引入了 **ONNX Runtime** 推理引擎，实现了一个可在 CPU 上本地运行的 AI 对话服务。

项目以 Microsoft **Phi-3-mini-4k-instruct**（int4 量化 ONNX 模型）作为推理模型，实现了：

- **自回归文本生成**：基于 KV-cache 的增量推理，逐步生成回复 token。
- **流式输出**：通过 WebSocket 将生成结果按 UTF-8 安全分片实时推送给前端，实现打字机式体验。
- **多轮对话**：基于 Session 维护上下文，将历史对话拼入 prompt，实现有记忆的对话。
- **引擎池**：预创建多个推理引擎实例，通过 RAII 自动获取/归还，提升并发吞吐。

---

## 特性

- **本地 CPU 推理**：无需 GPU，依赖 ONNX Runtime CPU 后端运行 int4 量化模型。
- **自回归 + KV-cache**：增量式 token 生成，避免重复计算历史前缀。
- **Greedy 解码**：对 logits 取 `argmax` 得到下一个 token。
- **流式 WebSocket 输出**：逐 token 推送，配合 UTF-8 增量切分算法，避免中文字符被截断。
- **多轮上下文记忆**：`ChatContextStore` 按 Session 保存历史，超时自动清理。
- **引擎池 + RAII**：`Phi3EnginePool` 预分配推理引擎，`Phi3EngineRAII` 自动归还。
- **复用框架基础设施**：继承 ModularNetServer 的路由、MVC、线程池、拦截器、限流、Session 等能力。
- **双 tokenizer 方案**：支持本地 C++ 贪心匹配，也支持调用 Python `tokenizers` 库进行精确编解码。

---

## 整体架构

```
                        浏览器前端 (index.html)
                              │
              ┌───────────────┴───────────────┐
              │ HTTP (9006)      WebSocket (9007)
              ▼                                ▼
   ┌──────────────────────────────────────────────────────┐
   │          ModularNetServer（网络框架，submodule 依赖）  │
   │   NetServer / Router / ThreadPool / Session / ...     │
   └──────────────────────┬───────────────────────────────┘
                          │
                          ▼
                    ChatController
        ┌──────────────┼──────────────────┐
        │              │                  │
   POST /ai/chat   POST /ai/history   WS /ai/streamInfer
        │              │                  │
        └──────────────┼──────────────────┘
                       ▼
              Phi3EngineRAII (RAII 借还引擎)
                       │
                       ▼
                Phi3EnginePool (引擎池 × N)
                       │
                       ▼
                   Phi3Engine (自回归生成)
        ┌──────────────┼──────────────────┐
        ▼              ▼                  ▼
   Tokenizer      Phi3GlobalModel     ChatContextStore
  (编码/解码)     (ONNX Session 单例)   (多轮上下文)
```

---

## 目录结构

```
MiniAiServer-master/
├── .gitmodules                    # 声明 external/ModularNetServer 子模块
├── CMakeLists.txt                 # 构建脚本
├── encode.py                      # Python tokenizer 编码脚本
├── decode.py                      # Python tokenizer 解码脚本
├── external/                      # 子模块目录
│   └── ModularNetServer/          # 基础网络框架（git submodule）
└── src/
    ├── main.cpp                   # 程序入口（初始化 ONNX Runtime + 启动服务器）
    ├── init.{h,cpp}               # 模块统一初始化（含 Phi3 引擎池）
    ├── ai/                        # AI 推理核心
    │   ├── onnx_engine.{h,cpp}    # ONNX 推理引擎基类
    │   └── phi3/                  # Phi-3 专属实现
    │       ├── phi3_global_model.*# 全局 ONNX Session + Tokenizer（单例）
    │       ├── phi3_engine.*      # 自回归生成引擎（chat / chatStream）
    │       ├── phi3_engine_pool.* # 引擎池
    │       └── phi3_engine_raii.* # RAII 借还封装
    ├── ai_session/                # 对话上下文管理
    │   └── chat_context_store.*   # Session → 多轮历史，带定时清理
    ├── tokenizer/                 # 分词器
    │   └── tokenizer.{h,cpp}      # 本地贪心 + Python 调用的双方案
    ├── mvc/controller/
    │   └── chat_controller.*      # 对话接口控制器
    ├── util/utf8_utils.h          # UTF-8 流式增量切分
    ├── config/server_config.json  # 运行时配置（热更新）
    ├── test/test_chat.cpp         # 本地推理测试
    └── www/index.html             # 前端聊天界面
```

> 注：`net/`、`conn/`、`router/`、`threadpool/`、`timer/`、`db/`、`cache/`、`session/`、`token/`、`log/`、`config/` 等框架模块位于 `external/ModularNetServer/`（git submodule），由 [ModularNetServer](https://github.com/Zhu-Hacken/ModularNetServer) 提供；`src/` 下仅包含 MiniAiServer 的 AI 业务增量。

---

## 核心模块说明

### 1. 推理引擎基类 `ai/OnnxEngine`

- 封装 ONNX Runtime 的 `Run` 调用流程，输入/输出均使用 `Ort::Value`。
- 提供 `fillDynamicShape()` 静态方法，将带 `-1` 的动态维度填充为实际的 batch 与 seq_len。
- 实际推理复用全局单例 `Phi3GlobalModel` 的 Session 与输入/输出名称。

### 2. 全局模型单例 `ai/phi3/Phi3GlobalModel`

- 单例模式，持有 `Ort::Env`、`Ort::SessionOptions`、`Ort::Session`。
- 构造时从 `model/phi3/` 加载 `.onnx` 模型与 `tokenizer.json`。
- 缓存输入/输出名称与 shape 模板，供引擎构造张量时使用。
- 模型路径：`<root>/model/phi3/phi3-mini-4k-instruct-cpu-int4-rtn-block-32-acc-level-4.onnx`。

### 3. 对话引擎 `ai/phi3/Phi3Engine`

- **`chat()`**：普通对话，一次性生成完整回复。
- **`chatStream()`**：流式生成，逐 token 计算 UTF-8 增量并通过 WebSocket 推送。
- **自回归生成流程**：
  1. `buildPromptWithHistory()` 拼接 `<|system|>`、历史 `<|user|>/<|assistant|>` 与当前输入。
  2. 编码 prompt 得到 `input_ids`。
  3. 初始化全零的 `past_key_values`。
  4. 循环：构造 `input_ids + attention_mask + past_key_values` → 推理 → 对 logits 取 `argmax` → 得到下一 token。
  5. 命中 EOS（`32007`）或达到 `max_steps`（2048）时停止。
  6. 解码生成结果，写入 `ChatContextStore` 保存上下文。
- **KV-cache 增量**：除首步外，每轮只喂入上一个 token，并回传 `present` 的 KV 张量。

### 4. 引擎池与 RAII `Phi3EnginePool` / `Phi3EngineRAII`

- `Phi3EnginePool`：预创建 N 个（默认 8）`Phi3Engine`，阻塞式 `getEngine()` / `releaseEngine()`，条件变量调度。
- `Phi3EngineRAII`：构造时从池中借引擎，析构时自动归还，保证异常路径下引擎不泄漏。

### 5. 分词器 `Tokenizer`

- 加载 `tokenizer.json` 的 `model.vocab`，构建 token ↔ id 双向映射。
- 提供两条实现路径：
  - **本地 C++**：贪心最长匹配编码 / 查表解码，处理 `<0x..>` 字节 token。
  - **调用 Python**：通过 `popen` 调用 `encode.py` / `decode.py`（基于 HuggingFace `tokenizers`），获得更精确的结果。
- 对话流程实际使用 Python 方案（`encodeFromPython` / `decodeFromPython`）。

### 6. 上下文存储 `ai_session/ChatContextStore`

- 单例，维护 `SessionId → ChatContext`（`vector<pair<user, ai>>` 历史）。
- 配合 `TimerManager` 定时清理过期上下文，默认 15 分钟超时。

### 7. 控制器 `mvc/controller/ChatController`

| 方法 | 路径              | 处理函数      | 说明                           |
| ---- | ----------------- | ------------- | ------------------------------ |
| POST | `/ai/chat`        | `chat`        | 普通对话，返回回复与 sessionId |
| POST | `/ai/history`     | `getHistory`  | 返回指定 session 的历史        |
| WS   | `/ai/streamInfer` | `streamInfer` | 流式推理，逐段推送             |

### 8. UTF-8 流式切分 `util/utf8_utils.h`

- `safeUtf8Delta()` 计算新生成文本相对于已发送文本的增量，并保证不切断多字节 UTF-8 字符（过滤替换字符 `0xEF 0xBF 0xBD`），避免中文乱码。

---

## 依赖环境

| 依赖                                                         | 说明                                                         |
| ------------------------------------------------------------ | ------------------------------------------------------------ |
| [ModularNetServer](https://github.com/Zhu-Hacken/ModularNetServer) | 基础网络框架（git submodule，位于 `external/ModularNetServer`） |
| ONNX Runtime                                                 | C++ API（`onnxruntime_cxx_api.h`），路径在 `CMakeLists.txt` 中配置 |
| Python3 + `tokenizers`                                       | tokenizer 编解码（`encode.py` / `decode.py`）                |
| CMake                                                        | ≥ 3.10                                                       |
| 编译器                                                       | 支持 C++17（g++/clang++）                                    |
| Linux                                                        | 依赖 epoll（继承自框架）                                     |

> ModularNetServer 仓库地址（HTTPS）：`https://github.com/Zhu-Hacken/ModularNetServer.git`
> SSH：`git@github.com:Zhu-Hacken/ModularNetServer.git`

---

## 模型准备

推理模型与 tokenizer 需放置在项目根目录的 `model/phi3/` 下：

```
<project>/model/phi3/
├── phi3-mini-4k-instruct-cpu-int4-rtn-block-32-acc-level-4.onnx
└── tokenizer.json
```

- 模型：Microsoft Phi-3-mini-4k-instruct，CPU int4 量化 ONNX 格式。
- tokenizer：对应的 `tokenizer.json`（供本地加载与 Python 脚本使用）。

模型路径在 `Phi3GlobalModel` 构造中硬编码，若文件不存在则跳过模型加载（服务器仍可启动，但 AI 接口不可用）。

---

## 编译与运行

### 1. 拉取子模块

MiniAiServer 通过 git submodule 依赖 ModularNetServer，先拉取框架代码到 `external/ModularNetServer/`（该目录初始为空）：

```bash
git submodule update --init --recursive
```

### 2. 配置 ONNX Runtime 路径

`CMakeLists.txt` 中 ONNX Runtime 路径硬编码为 `/dev/onnxruntime`，请根据本机实际安装位置修改：

```cmake
set(ONNXRUNTIME_ROOTDIR /dev/onnxruntime)
include_directories(${ONNXRUNTIME_ROOTDIR}/include ${ONNXRUNTIME_ROOTDIR}/include/onnxruntime/core/session)
link_directories(${ONNXRUNTIME_ROOTDIR}/build/Linux/Release)
```

### 3. 构建

```bash
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

构建生成两个可执行文件：

- `MiniAiServer` —— 主程序（HTTP + WebSocket 服务器）
- `test_chat` —— 本地推理测试

### 4. 运行

```bash
./MiniAiServer
```

启动后，HTTP 监听 `9006` 端口、WebSocket 监听 `9007` 端口，访问 `http://localhost:9006/` 即可打开聊天界面。

---

## 接口说明

### 1. 普通对话

```
POST /ai/chat
Content-Type: application/json

{ "message": "你好", "sessionId": "" }
```

响应：

```json
{ "data": "AI 回复内容", "sessionId": "生成的会话ID" }
```

### 2. 获取历史

```
POST /ai/history
{ "sessionId": "xxx" }
```

响应：

```json
{ "sessionId": "xxx", "history": [ { "user": "问题", "ai": "回答" }, ... ] }
```

### 3. 流式推理

```
WS /ai/streamInfer?sessionId=xxx
```

连接后发送文本消息，服务端逐段推送生成结果，结束时推送 `[DONE]`。

---

## 前端页面

`src/www/index.html` 提供了一个完整的暗色聊天界面：

- 首次发送走 `POST /ai/chat` 获取并保存 `sessionId`（存于 `localStorage`）。
- 后续发送走 `WebSocket`（`ws://host:9007/ai/streamInfer?sessionId=...`）流式接收。
- 页面加载时调用 `/ai/history` 恢复历史对话。

访问 `http://localhost:9006/` 即可体验。

---

## 设计亮点

1. **框架与业务解耦**：网络、路由、并发等基础能力全部复用 ModularNetServer，AI 逻辑以增量模块接入。
2. **全局模型单例 + 引擎池**：模型只加载一次，多个轻量 `Phi3Engine` 共享同一 Session，兼顾内存与并发。
3. **KV-cache 增量推理**：显著减少自回归生成时的重复计算。
4. **UTF-8 安全的流式输出**：`safeUtf8Delta` 保证多字节字符不被截断，中文输出无乱码。
5. **RAII 资源管理**：引擎借还、上下文清理均通过 RAII 与定时器自动完成。

---

## 注意事项

- **模型文件缺失**：模型与 tokenizer 未放置时，`Phi3GlobalModel` 会跳过加载，AI 相关接口将无法正常工作。
- **依赖 Python**：对话流程的 tokenizer 编解码默认调用 `python3` 与 HuggingFace `tokenizers`，需确保环境可用，且 `encode.py` / `decode.py` 位于工作目录可访问路径。
- **当前实现状态**：`ChatController::chat()` 中实际推理调用被注释（仅返回 sessionId），可用的完整推理路径为 WebSocket 流式接口 `/ai/streamInfer`。
- **平台限制**：继承自 ModularNetServer，依赖 Linux 专属系统调用（`epoll`），无法在 Windows 上直接编译运行。

---

# MiniAiServer

> A lightweight AI chat server built on [ModularNetServer](https://github.com/Zhu-Hacken/ModularNetServer), running Microsoft **Phi-3-mini-4k-instruct** locally on CPU via ONNX Runtime, with support for standard chat and WebSocket streaming output.

## Table of Contents

- [Introduction](#introduction)
- [Features](#features)
- [Architecture](#architecture)
- [Directory Layout](#directory-layout)
- [Core Modules](#core-modules)
- [Dependencies](#dependencies)
- [Model Preparation](#model-preparation)
- [Build & Run](#build--run)
- [API Reference](#api-reference)
- [Frontend](#frontend)
- [Design Highlights](#design-highlights)
- [Notes](#notes)

---

## Introduction

MiniAiServer is an upper-layer application of [ModularNetServer](https://github.com/Zhu-Hacken/ModularNetServer). On top of its modular high-performance network framework, MiniAiServer introduces an **ONNX Runtime** inference engine to provide an AI chat service that runs locally on CPU.

Using Microsoft **Phi-3-mini-4k-instruct** (int4-quantized ONNX model) as its inference model, it implements:

- **Autoregressive text generation**: incremental inference with KV-cache, generating reply tokens step by step.
- **Streaming output**: pushes generated results to the frontend over WebSocket with UTF-8-safe chunking for a typewriter effect.
- **Multi-turn dialogue**: maintains context per session and stitches history into the prompt for memory-aware conversation.
- **Engine pool**: pre-creates multiple inference engines, borrowed/returned via RAII for higher concurrency.

---

## Features

- **Local CPU inference**: runs an int4-quantized model on CPU via ONNX Runtime, no GPU required.
- **Autoregressive + KV-cache**: incremental token generation avoids recomputing the historical prefix.
- **Greedy decoding**: picks the next token via `argmax` over logits.
- **Streaming WebSocket output**: token-by-token push with a UTF-8 delta algorithm to avoid splitting multibyte characters.
- **Multi-turn memory**: `ChatContextStore` keeps history per session with timeout-based auto-cleanup.
- **Engine pool + RAII**: `Phi3EnginePool` pre-allocates engines, `Phi3EngineRAII` auto-returns them.
- **Reuses framework infrastructure**: inherits routing, MVC, thread pool, interceptors, rate limiting, and sessions from ModularNetServer.
- **Dual tokenizer**: local C++ greedy matching, or Python `tokenizers` for exact encoding/decoding.

---

## Architecture

```
                       Browser frontend (index.html)
                              │
              ┌───────────────┴───────────────┐
              │ HTTP (9006)      WebSocket (9007)
              ▼                                ▼
   ┌──────────────────────────────────────────────────────┐
   │       ModularNetServer (framework, submodule)        │
   │   NetServer / Router / ThreadPool / Session / ...    │
   └──────────────────────┬───────────────────────────────┘
                          │
                          ▼
                    ChatController
        ┌──────────────┼──────────────────┐
        │              │                  │
   POST /ai/chat   POST /ai/history   WS /ai/streamInfer
        │              │                  │
        └──────────────┼──────────────────┘
                       ▼
              Phi3EngineRAII (borrow/return engine)
                       │
                       ▼
                Phi3EnginePool (pool × N)
                       │
                       ▼
                   Phi3Engine (autoregressive generation)
        ┌──────────────┼──────────────────┐
        ▼              ▼                  ▼
   Tokenizer      Phi3GlobalModel     ChatContextStore
  (encode/decode) (ONNX Session)      (multi-turn context)
```

---

## Directory Layout

```
MiniAiServer-master/
├── .gitmodules                    # declares external/ModularNetServer submodule
├── CMakeLists.txt                 # build script
├── encode.py                      # Python tokenizer encode script
├── decode.py                      # Python tokenizer decode script
├── external/                      # submodule directory
│   └── ModularNetServer/          # base network framework (git submodule)
└── src/
    ├── main.cpp                   # entry point (init ONNX Runtime + start server)
    ├── init.{h,cpp}               # unified module init (incl. Phi3 engine pool)
    ├── ai/                        # AI inference core
    │   ├── onnx_engine.{h,cpp}    # ONNX inference engine base
    │   └── phi3/                  # Phi-3 specific implementation
    │       ├── phi3_global_model.*# global ONNX Session + Tokenizer (singleton)
    │       ├── phi3_engine.*      # autoregressive generation (chat / chatStream)
    │       ├── phi3_engine_pool.* # engine pool
    │       └── phi3_engine_raii.* # RAII borrow/return wrapper
    ├── ai_session/                # chat context management
    │   └── chat_context_store.*   # Session → history, with timer cleanup
    ├── tokenizer/                 # tokenizer
    │   └── tokenizer.{h,cpp}      # local greedy + Python-call dual approach
    ├── mvc/controller/
    │   └── chat_controller.*      # chat endpoint controller
    ├── util/utf8_utils.h          # UTF-8 streaming delta split
    ├── config/server_config.json  # runtime config (hot-reload)
    ├── test/test_chat.cpp         # local inference test
    └── www/index.html             # frontend chat UI
```

> Note: framework modules (`net/`, `conn/`, `router/`, `threadpool/`, `timer/`, `db/`, `cache/`, `session/`, `token/`, `log/`, `config/`) live under `external/ModularNetServer/` (git submodule), provided by [ModularNetServer](https://github.com/Zhu-Hacken/ModularNetServer); `src/` contains only MiniAiServer's AI-specific additions.

---

## Core Modules

### 1. Inference base `ai/OnnxEngine`

- Wraps the ONNX Runtime `Run` call, using `Ort::Value` for inputs/outputs.
- Provides the static `fillDynamicShape()` to fill `-1` dynamic dims with actual batch and seq_len.
- Actual inference reuses the global singleton `Phi3GlobalModel`'s Session and input/output names.

### 2. Global model singleton `ai/phi3/Phi3GlobalModel`

- Singleton holding `Ort::Env`, `Ort::SessionOptions`, and `Ort::Session`.
- Loads the `.onnx` model and `tokenizer.json` from `model/phi3/` at construction.
- Caches input/output names and shape templates for tensor construction.
- Model path: `<root>/model/phi3/phi3-mini-4k-instruct-cpu-int4-rtn-block-32-acc-level-4.onnx`.

### 3. Chat engine `ai/phi3/Phi3Engine`

- **`chat()`**: standard chat, generates the full reply at once.
- **`chatStream()`**: streaming generation, pushes UTF-8-safe deltas token by token over WebSocket.
- **Autoregressive flow**:
  1. `buildPromptWithHistory()` stitches `<|system|>`, history `<|user|>/<|assistant|>` and the current input.
  2. Encodes the prompt into `input_ids`.
  3. Initializes all-zero `past_key_values`.
  4. Loop: build `input_ids + attention_mask + past_key_values` → infer → `argmax` over logits → next token.
  5. Stop on EOS (`32007`) or `max_steps` (2048).
  6. Decode the result and store it in `ChatContextStore`.
- **KV-cache incremental**: after the first step, only the last token is fed in, and the `present` KV tensors are carried forward.

### 4. Engine pool & RAII `Phi3EnginePool` / `Phi3EngineRAII`

- `Phi3EnginePool`: pre-creates N (default 8) `Phi3Engine` instances, blocking `getEngine()` / `releaseEngine()` with condition-variable scheduling.
- `Phi3EngineRAII`: borrows an engine on construction and auto-returns it on destruction.

### 5. Tokenizer `Tokenizer`

- Loads `model.vocab` from `tokenizer.json` to build token ↔ id maps.
- Two implementations:
  - **Local C++**: greedy longest-match encoding / table-lookup decoding, handling `<0x..>` byte tokens.
  - **Python call**: invokes `encode.py` / `decode.py` (based on HuggingFace `tokenizers`) via `popen` for more accurate results.
- The chat flow uses the Python approach (`encodeFromPython` / `decodeFromPython`).

### 6. Context store `ai_session/ChatContextStore`

- Singleton maintaining `SessionId → ChatContext` (`vector<pair<user, ai>>` history).
- Uses `TimerManager` to clean up expired contexts (default 15-minute timeout).

### 7. Controller `mvc/controller/ChatController`

| Method | Path              | Handler       | Description                                |
| ------ | ----------------- | ------------- | ------------------------------------------ |
| POST   | `/ai/chat`        | `chat`        | standard chat, returns reply and sessionId |
| POST   | `/ai/history`     | `getHistory`  | returns history for a session              |
| WS     | `/ai/streamInfer` | `streamInfer` | streaming inference, pushes segments       |

### 8. UTF-8 streaming split `util/utf8_utils.h`

- `safeUtf8Delta()` computes the delta between the newly generated text and the already-sent text, ensuring multibyte UTF-8 characters are not split (filters the replacement char `0xEF 0xBF 0xBD`), avoiding garbled Chinese output.

---

## Dependencies

| Dependency                                                   | Description                                                  |
| ------------------------------------------------------------ | ------------------------------------------------------------ |
| [ModularNetServer](https://github.com/Zhu-Hacken/ModularNetServer) | base network framework (git submodule, under `external/ModularNetServer`) |
| ONNX Runtime                                                 | C++ API (`onnxruntime_cxx_api.h`), path configured in `CMakeLists.txt` |
| Python3 + `tokenizers`                                       | tokenizer encode/decode (`encode.py` / `decode.py`)          |
| CMake                                                        | ≥ 3.10                                                       |
| Compiler                                                     | C++17-capable (g++/clang++)                                  |
| Linux                                                        | depends on epoll (inherited from framework)                  |

> ModularNetServer repository (HTTPS): `https://github.com/Zhu-Hacken/ModularNetServer.git`
> SSH: `git@github.com:Zhu-Hacken/ModularNetServer.git`

---

## Model Preparation

Place the inference model and tokenizer under `model/phi3/` in the project root:

```
<project>/model/phi3/
├── phi3-mini-4k-instruct-cpu-int4-rtn-block-32-acc-level-4.onnx
└── tokenizer.json
```

- Model: Microsoft Phi-3-mini-4k-instruct, CPU int4-quantized ONNX format.
- Tokenizer: the corresponding `tokenizer.json` (for local loading and Python scripts).

The model path is hardcoded in the `Phi3GlobalModel` constructor; if the file is missing, model loading is skipped (the server still starts, but the AI endpoints are unavailable).

---

## Build & Run

### 1. Fetch the submodule

MiniAiServer depends on ModularNetServer via a git submodule; fetch the framework into `external/ModularNetServer/` (initially empty):

```bash
git submodule update --init --recursive
```

### 2. Configure the ONNX Runtime path

`CMakeLists.txt` hardcodes the ONNX Runtime path as `/dev/onnxruntime`; adjust it to your local installation:

```cmake
set(ONNXRUNTIME_ROOTDIR /dev/onnxruntime)
include_directories(${ONNXRUNTIME_ROOTDIR}/include ${ONNXRUNTIME_ROOTDIR}/include/onnxruntime/core/session)
link_directories(${ONNXRUNTIME_ROOTDIR}/build/Linux/Release)
```

### 3. Build

```bash
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

This produces two executables:

- `MiniAiServer` — the main program (HTTP + WebSocket server)
- `test_chat` — a local inference test

### 4. Run

```bash
./MiniAiServer
```

On startup, HTTP listens on port `9006` and WebSocket on `9007`; visit `http://localhost:9006/` to open the chat UI.

---

## API Reference

### 1. Standard chat

```
POST /ai/chat
Content-Type: application/json

{ "message": "Hello", "sessionId": "" }
```

Response:

```json
{ "data": "AI reply", "sessionId": "generated-session-id" }
```

### 2. Get history

```
POST /ai/history
{ "sessionId": "xxx" }
```

Response:

```json
{ "sessionId": "xxx", "history": [ { "user": "question", "ai": "answer" }, ... ] }
```

### 3. Streaming inference

```
WS /ai/streamInfer?sessionId=xxx
```

Send a text message after connecting; the server pushes generated segments, ending with `[DONE]`.

---

## Frontend

`src/www/index.html` provides a complete dark-themed chat UI:

- The first message goes through `POST /ai/chat` to obtain and store a `sessionId` (in `localStorage`).
- Subsequent messages use `WebSocket` (`ws://host:9007/ai/streamInfer?sessionId=...`) for streaming.
- On page load, `/ai/history` restores previous conversation.

Visit `http://localhost:9006/` to try it.

---

## Design Highlights

1. **Framework/business decoupling**: network, routing, and concurrency are all reused from ModularNetServer, with AI logic added as incremental modules.
2. **Global model singleton + engine pool**: the model loads once; multiple lightweight `Phi3Engine` instances share the same Session, balancing memory and concurrency.
3. **KV-cache incremental inference**: greatly reduces redundant computation during autoregressive generation.
4. **UTF-8-safe streaming**: `safeUtf8Delta` prevents splitting multibyte characters, so Chinese output stays clean.
5. **RAII resource management**: engine borrow/return and context cleanup are handled automatically via RAII and timers.

---

## Notes

- **Missing model files**: if the model and tokenizer are not placed correctly, `Phi3GlobalModel` skips loading and the AI endpoints will not work.
- **Python dependency**: the chat flow's tokenizer defaults to calling `python3` with HuggingFace `tokenizers`; ensure the environment works and `encode.py` / `decode.py` are accessible from the working directory.
- **Current implementation status**: the actual inference call in `ChatController::chat()` is commented out (it only returns the sessionId); the fully working inference path is the WebSocket streaming endpoint `/ai/streamInfer`.
- **Platform limitation**: inherited from ModularNetServer, it depends on Linux-specific syscalls (`epoll`) and cannot be built/run directly on Windows.

---

## 许可证 / License

本项目采用 [MIT License](LICENSE) 发布。你可以在自己的项目中自由使用、修改和分发，但需保留版权声明。

This project is released under the [MIT License](LICENSE). You are free to use, modify, and distribute it in your own projects, provided the copyright notice is retained.
