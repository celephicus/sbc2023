for /f %%f in (..\Relay\Slave\files_for_grab.txt) do (
attrib -R Slave\%%f
copy /Y ..\Relay\Slave\%%f Slave\%%f 
attrib +R Slave\%%f )

attrib -R Slave\Slave.cppproj
attrib -R Slave\Slave.componentinfo.xml
