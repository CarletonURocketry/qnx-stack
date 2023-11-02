@echo off

:: Set host from first arg
set host=%1

:: Set binaries from remaining arguments
shift
set binaries=
:loop
if "%1"=="" goto after_loop
set binaries=%binaries% %1
shift
goto loop
:after_loop

:: Host must be provided
if %host% == "" (
    call:usage
)
set user_dir="/tmp/%username%/"

:: Use plink to login
for /F "tokens=*" %%g in ('type .password') do (set password=%%g)
plink -batch -ssh -pw %password% "root@%host%" -t "mkdir -p %user_dir%"
pscp -batch -scp -pw %password% %binaries% "root@%host%:%user_dir%"
exit 0

:: Function to print usage and exit with error
:useage
echo "Usage: %0 host binaries" 1>&2;
exit 1;
goto:eof
