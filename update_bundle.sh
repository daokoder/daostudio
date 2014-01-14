cp -r dao/doc/html/en doc/html/
cp -r doc DaoStudio.app/Contents/Resources
mkdir DaoStudio.app/Contents/Resources/langs
cp langs/*.qm DaoStudio.app/Contents/Resources/langs
#mkdir DaoStudio.app/Contents/Frameworks
#cp build/libdao.dylib DaoStudio.app/Contents/Frameworks/
#cp build/modules/serializer/libdao_serializer.dylib DaoStudio.app/Contents/Frameworks/
#cp build/modules/debugger/libdao_debugger.dylib DaoStudio.app/Contents/Frameworks/
#cp build/modules/profiler/libdao_profiler.dylib DaoStudio.app/Contents/Frameworks/


install_name_tool -id @executable_path/../Frameworks/lib/libdao.dylib DaoStudio.app/Contents/Frameworks/lib/libdao.dylib 
install_name_tool -id @executable_path/../Frameworks/lib/dao/modules/libdao_serializer.dylib DaoStudio.app/Contents/Frameworks/lib/dao/modules/libdao_serializer.dylib 
install_name_tool -id @executable_path/../Frameworks/lb/dao/modules/ibdao_debugger.dylib DaoStudio.app/Contents/Frameworks/lib/dao/modules/libdao_debugger.dylib 
install_name_tool -id @executable_path/../Frameworks/lib/dao/modules/libdao_profiler.dylib DaoStudio.app/Contents/Frameworks/lib/dao/modules/libdao_profiler.dylib 

install_name_tool -change @rpath/libdao.dylib @executable_path/../Frameworks/lib/libdao.dylib DaoStudio.app/Contents/MacOS/DaoStudio
install_name_tool -change @rpath/libdao_serializer.dylib @executable_path/../Frameworks/lib/dao/modules/libdao_serializer.dylib DaoStudio.app/Contents/MacOS/DaoStudio
install_name_tool -change @rpath/libdao_debugger.dylib @executable_path/../Frameworks/lib/dao/modules/libdao_debugger.dylib DaoStudio.app/Contents/MacOS/DaoStudio
install_name_tool -change @rpath/libdao_profiler.dylib @executable_path/../Frameworks/lib/dao/modules/libdao_profiler.dylib DaoStudio.app/Contents/MacOS/DaoStudio
