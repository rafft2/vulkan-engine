@echo off

IF NOT EXIST .\bin mkdir .\bin

set warning_opts=-Wall -WX -wd4100 -wd4820 -wd4189 -wd5045 -wd4005 -wd4668
set compile_out=-Fe:"bin\veng.exe" -Fo:"bin\veng.obj" -Fd:"bin\vc140.pdb"
set additional_include=/I %VULKAN_SDK%\Include /I thirdparty\

cl %additional_include% -DVK_USE_PLATFORM_WIN32_KHR -DGLFW_EXPOSE_NATIVE_WIN32 -DDEBUG_BUILD -nologo -Zi -GR- -EHa- -Odi %warning_opts% user32.lib gdi32.lib shell32.lib %VULKAN_SDK%\Lib\vulkan-1.lib "src\veng_main.cpp" "thirdparty\glfw3_mt.lib" %compile_out%

set warning_opts=
set compile_out=
set additional_include=