@echo off
setlocal EnableExtensions
cd /d "%~dp0backend"

if not exist "program88.py" (
    echo [BLAD] Brak pliku backend\program88.py
    pause
    exit /b 1
)

if not exist "env.txt" (
    echo [BLAD] Brak pliku backend\env.txt
    echo Skopiuj env.txt.example jako env.txt i wpisz nazwe projektu GEE.
    pause
    exit /b 1
)

where python >nul 2>&1
if errorlevel 1 (
    echo [BLAD] Nie znaleziono Python w PATH.
    pause
    exit /b 1
)

if not exist ".venv\Scripts\python.exe" (
    echo Tworzenie srodowiska wirtualnego .venv ...
    python -m venv .venv
    if errorlevel 1 (
        echo [BLAD] Nie udalo sie utworzyc .venv
        pause
        exit /b 1
    )
)

call ".venv\Scripts\activate.bat"
if errorlevel 1 (
    echo [BLAD] Nie udalo sie aktywowac .venv
    pause
    exit /b 1
)

echo Instalacja / aktualizacja zaleznosci ...
python -m pip install --upgrade pip >nul
python -m pip install -r requirements.txt
if errorlevel 1 (
    echo [BLAD] pip install nie powiodl sie
    pause
    exit /b 1
)

echo.
echo Uruchamiam backend program88: http://127.0.0.1:8000
echo Dokumentacja API:              http://127.0.0.1:8000/docs
echo Zatrzymanie: Ctrl+C
echo.

python -m uvicorn program88:app --reload --host 127.0.0.1 --port 8000
set EXITCODE=%ERRORLEVEL%

echo.
if not "%EXITCODE%"=="0" (
    echo [BLAD] Backend zakonczyl sie kodem %EXITCODE%
)
pause
exit /b %EXITCODE%
