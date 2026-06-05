@echo off

IF NOT EXIST .\bin mkdir .\bin

set warning_opts=-W3 -Wall -WX -wd4100 -wd4820
set compile_out=-Fe:"bin\veng.exe" -Fo:"bin\veng.obj" -Fd:"bin\vc140.pdb"
set additional_include=/I %VULKAN_SDK%\Include /I thirdparty\

cl %additional_include% -DDEBUG_BUILD -nologo -Zi -GR- -EHa- -Odi %warning_opts% user32.lib gdi32.lib shell32.lib %VULKAN_SDK%\Lib\vulkan-1.lib "src\veng_main.cpp" "thirdparty\glfw3_mt.lib" %compile_out%

set warning_opts=
set compile_out=
set additional_include=