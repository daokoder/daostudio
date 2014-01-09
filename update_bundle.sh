cp -r dao/doc/html/en doc/html/
cp -r doc DaoStudio.app/Contents/Resources
mkdir DaoStudio.app/Contents/Frameworks
cp dao/libdao.dylib DaoStudio.app/Contents/Frameworks/
cp dao/modules/serializer/libdao_serializer.dylib DaoStudio.app/Contents/Frameworks/

install_name_tool -id @executable_path/../Frameworks/libdao.dylib DaoStudio.app/Contents/Frameworks/libdao.dylib 
install_name_tool -id @executable_path/../Frameworks/libdao_serializer.dylib DaoStudio.app/Contents/Frameworks/libdao_serializer.dylib 
install_name_tool -change libdao.dylib @executable_path/../Frameworks/libdao.dylib DaoStudio.app/Contents/MacOS/daostudio
install_name_tool -change libdao_serializer.dylib @executable_path/../Frameworks/libdao_serializer.dylib DaoStudio.app/Contents/MacOS/daostudio

