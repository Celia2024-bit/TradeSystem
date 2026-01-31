@echo off
chcp 65001 >nul
echo ==============================================
echo Starting Trading System components, please wait...
echo ==============================================

:: 1. Start trading_system.exe (relative path: ./output/)
start "Trading System Main Program" cmd /k "cd /d ./output && .\trading_system.exe"

:: 2. Start MarketFetch.py (relative path: ./src/)
start "Market Data Fetch Service" cmd /k "cd /d ./src && python.exe .\MarketFetch.py"

:: 3. Start performance monitor server (relative path: ./tools/performance_monitor/)
start "Performance Monitor Server" cmd /k "cd /d ./tools/performance_monitor && python server_main.py"

:: 4. Start frontend HTTP server (relative path: ./tools/performance_monitor/frontEnd/)
start "Frontend HTTP Server" cmd /k "cd /d ./tools/performance_monitor/frontEnd && python -m http.server 7070"

:: Wait 3 seconds to ensure services are fully started before opening browser
timeout /t 3 /nobreak >nul

:: 5. Open browser to access frontend page
start "" "http://localhost:7070/index.html"

echo ==============================================
echo All component startup commands have been sent!
echo Frontend page has been opened automatically.
echo If not opened, please visit manually:
echo http://localhost:7070/index.html
echo ==============================================
pause >nul