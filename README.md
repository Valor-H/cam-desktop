# CamDemo

QJCam 云端协同平台的桌面端（CamDemo），基于 **Qt 5.15 + QCefView + SARibbonBar + libhv** �?C++/CMake 工程。通过内嵌 CEF 浏览器承�?cloud-cam-front 前端页面，并�?user-service 后端对接，实现桌面形态下的账号体系、文件管理与工作区入口�?
## 功能概览

- 主窗口：SARibbonBar 功能�?+ 可切换的首页工作�?/ 文件管理工作�?- 账号体系：内�?CEF 页面完成登录、注册、找回密码；持久�?Token 与会�?- 用户身份芯片（TitleBarUserChip）：标题栏显示当前用户、下拉进入个人中�?/ 团队 / 退�?- 文件管理：调用前端的文件页面，支持打开项目、新建项目等桌面侧入�?- 本地桌面前端服务：libhv HTTP 服务，向内嵌 CEF 注入 bootstrap 数据（后端地址、Token、用户、离线态等�?- 本地文件快照：记录最近打开的本地文件（LocalFilesSnapshot�?- 国际化：随程序启动加�?`translations/CamDemo_zh_CN.qm`

## 技术栈

- C++17、CMake �?3.19、MSVC（v142 Toolset�?- Qt 5.15.2（Core / Gui / Widgets / Network / Concurrent / LinguistTools�?- QCefView（CEF 桌面浏览器控件）
- SARibbonBar（Ribbon 风格主窗口）
- libhv（HTTP 客户�?/ 本地 HTTP 服务�?
## 目录结构

```
cam-demo/
  main.cpp                     # 程序入口，加载翻译、构�?CamDemo 主窗�?  NApplication.*               # QApplication 派生类，全局初始�?  NMainWindow.*                # Ribbon 主窗口，承载工作区与用户芯片
  CamDemo.*                    # 主窗口外壳（ui/qrc�?  mock_main_workspace.h          # 首页占位工作�?  DesktopWebServer.*      # 本地 libhv HTTP 服务，向前端注入 bootstrap
  QCefRuntime.*                # QCefView 运行时初始化
  TitleBarUserChip.*           # 标题栏用户身份组�?  user/                        # 账号 / 登录 / 文件管理 UI 动态库
    AccountAuthDialog.*        # 登录/注册/找回密码 CEF 对话�?    AuthHttpClient.*           # 调用 user-service �?HTTP 客户�?    UserAuthService.*          # 认证服务门面
    UserSession.*              # 会话�?Token 持久�?    FileManagerView.*          # 文件管理 CEF 视图
    LocalFilesSnapshot.*       # 本地文件快照
    LoginWebAuthHelpers.*      # Web 登录桥接
    MockCamOptions.*           # 可调参数（示�?调试用）
    user_module_config.h         # user 模块默认配置
  resource/                    # 图标与头像资�?  translations/                # Qt Linguist .ts 翻译
```

## 依赖前置

构建前需准备以下三方依赖，默认路径见 [CMakeLists.txt](CMakeLists.txt)，可按需修改�?
| 依赖 | 默认路径 | 说明 |
| --- | --- | --- |
| Qt 5.15.2 | `D:/Tools/QT5/5.15.2/msvc2019_64` | `CMAKE_PREFIX_PATH` |
| QCefView | `../3rdparty/QCefView`（Debug/Release 各含 `bin/` �?`lib/`�?| CEF 内嵌浏览�?|
| SARibbonBar | `D:/Codes/SARibbon/bin_qt5.15.2_MSVC_x64` | Ribbon 控件 |
| libhv | `D:/Codes/libhv/installed_debug` | HTTP �?|

构建�?`POST_BUILD` 会自动把 QCefView、libhv、SARibbon 的运行时 DLL 以及翻译文件复制到目标输出目录�?
## 构建与运�?
```powershell
# 从仓库根目录进入 cam-demo
cmake -S . -B build -G "Visual Studio 16 2019" -A x64
cmake --build build --config Debug

# 运行（产物位�?build/bin�?.\build\bin\CamDemo.exe
```

> 首次运行会生成本地设置（`QianJiZN / CamDemo`），并尝试启动本地前端桥接服务�?
## 运行时配�?
默认运行时参数定义在 [user/user_module_config.h](user/user_module_config.h)�?
| �?| 默认�?| 说明 |
| --- | --- | --- |
| `apiBaseUrl` | `http://localhost:8080/` | user-service 后端地址 |
| `websocketUrl` | `ws://localhost:8091/ws` | WebSocket 推送地址 |
| `helpDocUrl` | `http://localhost:4173` | 帮助文档站点 |
| `frontendBaseUrl` | `http://localhost:31870/` | 内嵌 CEF 加载的前端地址（由本地桥接服务提供�?|
| `externalFrontendBaseUrl` | `http://localhost:5173/` | 外链浏览器打开的前端地址 |
| `settingsOrg` / `settingsApp` | `QianJiZN` / `CamDemo` | QSettings 组织与应用名 |
| `authTokenKey` | `auth/token` | Token 在本地设置中的键 |

## 关联项目

- [cloud-cam-front](../cloud-cam-front) �?内嵌 CEF 加载�?Vue 3 前端
- [user-service](../user-service) �?对应�?Spring Boot 后端服务

## 备注

- 如出�?CEF 加载空白，先确认 `QCefView` Debug/Release 版本与当前构建配置一致，并检�?`POST_BUILD` 是否已把 `bin/` 产物拷贝完整�?