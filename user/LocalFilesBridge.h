#pragma once

#include <QString>

namespace LocalFilesBridge
{
inline QString MethodRequestLocalFiles()
{
    return QStringLiteral("Desktop.RequestLocalFiles");
}

inline QString MethodRequestLogin()
{
    return QStringLiteral("Desktop.RequestLogin");
}

inline QString MethodRequestOpenRecentFile()
{
  return QStringLiteral("Desktop.RequestOpenRecentFile");
}

inline QString BridgeInjectScript()
{
    return QStringLiteral(R"JS(
      (function() {
        var existing = window.__DESKTOP_QT__;
        if (!existing || typeof existing !== 'object') {
          existing = {};
        }
        existing.__localFilesReady = true;
        existing.requestLogin = function(payload) {
          try {
            if (window.CallBridge && typeof window.CallBridge.invoke === 'function') {
              window.CallBridge.invoke('Desktop.RequestLogin', payload || {});
              return true;
            }
          } catch (e) {
          }
          return false;
        };
        existing.requestLocalFiles = function(payload) {
          try {
            if (window.CallBridge && typeof window.CallBridge.invoke === 'function') {
              window.CallBridge.invoke('Desktop.RequestLocalFiles', payload || {});
              return true;
            }
          } catch (e) {
          }
          return false;
        };
        existing.requestOpenRecentFile = function(payload) {
          try {
            if (window.CallBridge && typeof window.CallBridge.invoke === 'function') {
              window.CallBridge.invoke('Desktop.RequestOpenRecentFile', payload || {});
              return true;
            }
          } catch (e) {
          }
          return false;
        };
        window.__DESKTOP_QT__ = existing;
      })();
    )JS");
}
}
