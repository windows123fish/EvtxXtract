@echo off
chcp 65001 >nul
cd /d D:\main\EvtxXtract

REM ===== 检查是否在 dev 分支 =====
for /f "tokens=*" %%i in ('git branch --show-current') do set CUR_BRANCH=%%i
if "%CUR_BRANCH%" NEQ "dev" (
    echo ERROR: Not in dev branch! Current: %CUR_BRANCH%
    echo Please run: git switch dev
    pause
    exit /b 1
)

echo ==========================================
echo EvtxXtract auto-commit started
echo Branch: %CUR_BRANCH%
echo Check every 30s (Press Ctrl+C to stop)
echo ==========================================
echo.

:loop
REM ===== 核心修正：不用临时文件，直接捕获 git 输出 =====
set HAS_CHANGES=0
for /f "delims=" %%i in ('git status --porcelain 2^>nul') do set HAS_CHANGES=1

if "%HAS_CHANGES%"=="1" (
    set "NOW=%date:~0,4%-%date:~5,2%-%date:~8,2%_%time:~0,2%:%time:~3,2%:%time:~6,2%"
    echo [%NOW%] Change detected, committing...
    
    git add .
    git commit -m "dev: update %NOW%" >nul 2>&1
    if %errorlevel% EQU 0 (
        git push >nul 2>&1
        if %errorlevel% EQU 0 (
            echo [%NOW%] Push success
        ) else (
            echo [%NOW%] Push failed, retry later
        )
    ) else (
        echo [%NOW%] No real changes, skip
    )
) else (
    echo [%time:~0,8%] No change, waiting...
)

REM ===== 用 ping 代替 timeout（VS Code 终端兼容）=====
ping -n 31 127.0.0.1 >nul

goto loop