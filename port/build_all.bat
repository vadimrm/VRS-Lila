@echo off
rem ============================================================
rem  Сборка портов Лилы на Windows (MinGW-w64 в C:\tools\mingw32)
rem  build_all.bat - собрать все три программы
rem ============================================================
set PATH=C:\tools\mingw32\bin;%PATH%
set SRC=%~dp0
set CC=gcc
rem ВАЖНО: -lwinmm -lm должны стоять ПОСЛЕ списка исходников!
set LIBS=-lwinmm -lcomctl32 -lm

echo === lila_install.exe (консольный генератор данных) ===
%CC% -O2 -I%SRC%common -static-libgcc -o "%SRC%install\lila_install.exe" "%SRC%install\lila_install.c" %LIBS% || goto :err

echo === lila58.exe ===
%CC% -O2 -I%SRC%common -static-libgcc -mwindows -o "%SRC%vrs58\data\lila58.exe" ^
  "%SRC%vrs58\vrs58.c" "%SRC%vrs58\vrs58c.c" "%SRC%vrs58\vrs58r.c" ^
  "%SRC%vrs58\vrs58s.c" "%SRC%vrs58\vrs58ap1.c" "%SRC%common\dos2win.c" %LIBS% || goto :err

echo === lila57.exe ===
%CC% -O2 -I%SRC%common -static-libgcc -mwindows -o "%SRC%vrs57\data\lila57.exe" ^
  "%SRC%vrs57\vrs57.c" "%SRC%vrs57\vrs57c.c" "%SRC%vrs57\vrs57r.c" ^
  "%SRC%vrs57\vrs57s.c" "%SRC%vrs57\vrs57ap1.c" "%SRC%common\dos2win.c" %LIBS% || goto :err

echo.
echo Сборка завершена успешно!
goto :eof

:err
echo ОШИБКА СБОРКИ!
exit /b 1
