mkdir tmp
mkdir tmp\doc
mkdir tmp\src
mkdir tmp\src\tools
mkdir tmp\src\library
mkdir tmp\src\library\encoding
mkdir tmp\src\filetype
mkdir tmp\tools

cl /MT /TP /D"_WINDOWS" /Fo.\tmp\src\tools\ src\tools\convert.c /link /DYNAMICBASE "kernel32.lib" "user32.lib" "gdi32.lib" "winspool.lib" "comdlg32.lib" "advapi32.lib" "shell32.lib" "ole32.lib" "oleaut32.lib" "uuid.lib" "odbc32.lib" "odbccp32.lib" "libcmt.lib" /MACHINE:x64 /SUBSYSTEM:console /OUT:tmp\tools\convert.exe

tmp\tools\convert.exe --bin2c LICENSE.md tmp\LICENSE.h file_LICENSE keep
tmp\tools\convert.exe --bin2c doc\index.md tmp\doc\index.h file_index keep
tmp\tools\convert.exe --bin2c doc\regex.md tmp\doc\regex.h file_regex keep

tmp\tools\convert.exe --bin2c src\config.default.txt tmp\src\config.default.h file_config_default reduce

cl /c /MT /TP /Ob2 /Oi /Ot /GL /W3 /D"_WINDOWS" /Fo.\tmp\src\library\ src\library\*.c 
cl /c /MT /TP /Ob2 /Oi /Ot /GL /W3 /D"_WINDOWS" /Fo.\tmp\src\library\encoding\ src\library\encoding\*.c 
cl /c /MT /TP /Ob2 /Oi /Ot /GL /W3 /D"_WINDOWS" /Fo.\tmp\src\filetype\ src\filetype\*.c 
cl /c /MT /TP /Ob2 /Oi /Ot /GL /W3 /D"_WINDOWS" /Fo.\tmp\src\ src\*.c 
cl tmp\src\library\*.obj tmp\src\library\encoding\*.obj tmp\src\filetype\*.obj tmp\src\*.obj /link /DYNAMICBASE "kernel32.lib" "user32.lib" "gdi32.lib" "winspool.lib" "comdlg32.lib" "advapi32.lib" "shell32.lib" "ole32.lib" "oleaut32.lib" "uuid.lib" "odbc32.lib" "odbccp32.lib" "libcmt.lib" /MACHINE:x64 /SUBSYSTEM:windows /OUT:tippse.exe
