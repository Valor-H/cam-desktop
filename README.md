# CamDemo

`cam-desktop` 是云端协同平台的桌面端，基于 Qt 5.15.2、QCefView、SARibbonBar、libhv 和 CMake。

## 依赖路径

当前 `CMakeLists.txt` 使用以下路径：

| 依赖 | 路径 |
| --- | --- |
| Qt 5.15.2 | `D:/Tools/QT5/5.15.2/msvc2019_64` |
| QCefView | `./3rdparty/QCefView` |
| SARibbonBar | `./3rdparty/SARibbon` |
| libhv Debug | `./3rdparty/libhv` |

关键配置见 [CMakeLists.txt](CMakeLists.txt)。

## CMake 构建

在 `cam-desktop` 目录执行：

```cmd
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
cmake --build build --config Release
```

如果你已经设置了 Qt 环境变量，也可以参考 [CMakePresets.json](CMakePresets.json) 中的 `QTDIR` 方案。

## 运行产物

- 可执行文件输出目录：`x64/`
- Debug 可执行文件：`x64/Debug/qjcam.exe`
- Release 可执行文件：`x64/Release/qjcam.exe`

构建后会自动复制 QCefView、SARibbonBar、libhv 的运行时 DLL 到输出目录。

## 关联项目

- `cloud-cam-front`：前端静态页面，桌面端会内嵌加载
- `user-service`：后端接口服务
