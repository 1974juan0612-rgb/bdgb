@echo off
REM Ejecutable del panal generacion-contenido para Windows
python "%~dp0run.py"
if errorlevel 1 (
    echo ERROR: El pipeline fallo
    pause
)
