@echo off

IF NOT EXIST .\bin mkdir .\bin

set warning_opts=-D_CRT_SECURE_NO_WARNINGS -Wall -WX -wd4100 -wd4820 -wd4189 -wd5045 -wd4005 -wd4668 -wd4702 -wd4201 -wd5246 -wd4191
set compile_out=-Fe:"bin\veng.exe" -Fo:"bin\veng.obj" -Fd:"bin\vc140.pdb"
set additional_include=/I %VULKAN_SDK%\Include /I thirdparty\GLFW /I thirdparty\volk /I thirdparty\meshoptimizer\src /I thirdparty\meshoptimizer\extern
set veng_defs=-DVK_USE_PLATFORM_WIN32_KHR -DGLFW_EXPOSE_NATIVE_WIN32 -DWIN32_LEAN_AND_MEAN -DDEBUG_BUILD

cl -nologo -Zi -GR- -EHa- -Odi %additional_include% %veng_defs% %warning_opts% user32.lib gdi32.lib shell32.lib "src\veng_main.cpp" "thirdparty\glfw3_mt.lib" %compile_out%

set warning_opts=
set compile_out=
set additional_include=
set veng_defs=

glslangValidator src\shaders\triangle.vert.glsl -V -o .\src\shaders\triangle.vert.spv
glslangValidator src\shaders\triangle.frag.glsl -V -o .\src\shaders\triangle.frag.spv
