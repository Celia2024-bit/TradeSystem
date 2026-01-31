@echo off
chcp 65001 >nul
echo ==============================================
echo Stopping Trading System components...
echo ==============================================

:: 定义窗口标题常量（和启动脚本完全一致）
set "TITLE_MAIN=Trading System Main Program"
set "TITLE_FETCH=Market Data Fetch Service"
set "TITLE_MONITOR=Performance Monitor Server"
set "TITLE_FRONTEND=Frontend HTTP Server"

:: 1. 终止 trading_system.exe 进程 + 关闭对应CMD窗口
taskkill /f /im trading_system.exe >nul 2>&1
taskkill /f /fi "WINDOWTITLE eq %TITLE_MAIN%" >nul 2>&1
if %errorlevel% equ 0 (
    echo [SUCCESS] %TITLE_MAIN% stopped and window closed.
) else (
    echo [INFO] %TITLE_MAIN% is not running.
)

:: 2. 终止 MarketFetch.py 对应的Python进程 + 关闭CMD窗口
taskkill /f /fi "WINDOWTITLE eq %TITLE_FETCH%" >nul 2>&1
if %errorlevel% equ 0 (
    echo [SUCCESS] %TITLE_FETCH% stopped and window closed.
) else (
    echo [INFO] %TITLE_FETCH% is not running.
)

:: 3. 终止性能监控服务端 + 关闭CMD窗口
taskkill /f /fi "WINDOWTITLE eq %TITLE_MONITOR%" >nul 2>&1
if %errorlevel% equ 0 (
    echo [SUCCESS] %TITLE_MONITOR% stopped and window closed.
) else (
    echo [INFO] %TITLE_MONITOR% is not running.
)

:: 4. 终止前端HTTP服务 + 关闭CMD窗口
taskkill /f /fi "WINDOWTITLE eq %TITLE_FRONTEND%" >nul 2>&1
if %errorlevel% equ 0 (
    echo [SUCCESS] %TITLE_FRONTEND% stopped and window closed.
) else (
    echo [INFO] %TITLE_FRONTEND% is not running.
)

echo ==============================================
echo All components and their CMD windows are cleared!
echo ==============================================
pause >nul