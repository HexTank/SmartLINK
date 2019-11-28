pushd "%~dp0"
cl /EHsc VirtualSDImage\main.cpp /link /out:..\bin\smartsd.exe
cl /EHsc /I ..\3rdparty\asio\include -D_WIN32_WINNT=0x0501 Client\main.cpp /link /out:..\bin\smartlink.exe
popd
pause
