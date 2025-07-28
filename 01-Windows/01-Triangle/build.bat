cl /c /EHsc /I "C:\VulkanSDK\1.3.296.0\Include" Vlkn.cpp

rc.exe Vlkn.rc

link Vlkn.obj Vlkn.res /LIBPATH:"C:\VulkanSDK\1.3.296.0\Lib" user32.lib gdi32.lib /SUBSYSTEM:WINDOWS
