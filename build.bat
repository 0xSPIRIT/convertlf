@echo off
cl.exe main.cpp /nologo /Z7 /Gs9999999 /GS- /EHa- /link user32.lib kernel32.lib shell32.lib /SUBSYSTEM:console /NODEFAULTLIB /incremental:no /out:convertlf.exe