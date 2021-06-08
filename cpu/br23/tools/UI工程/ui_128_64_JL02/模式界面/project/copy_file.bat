@echo off

copy .\project.bin ..\..\..\..\ui_resource\JL.sty
copy .\result.bin  ..\..\..\..\ui_resource\JL.res
copy .\result.str  ..\..\..\..\ui_resource\JL.str
if exist "..\..\..\..\..\..\..\apps\soundbox\include\ui\" (
    copy .\ename.h     ..\..\..\..\..\..\..\apps\soundbox\include\ui\style_JL02.h
)

exit
