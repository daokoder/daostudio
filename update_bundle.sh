cp -r dao/doc/html/en doc/html/
cp -r doc daostudio.app/Contents/Resources
cp dao/libdao.dylib daostudio.app/Contents/Frameworks/
cp dao/modules/serializer/libdao_serializer.dylib daostudio.app/Contents/Frameworks/

install_name_tool -id @executable_path/../Frameworks/libdao.dylib daostudio.app/Contents/Frameworks/libdao.dylib 
install_name_tool -id @executable_path/../Frameworks/libdao_serializer.dylib daostudio.app/Contents/Frameworks/libdao_serializer.dylib 
install_name_tool -change libdao.dylib @executable_path/../Frameworks/libdao.dylib daostudio.app/Contents/MacOS/daostudio
install_name_tool -change libdao_serializer.dylib @executable_path/../Frameworks/libdao_serializer.dylib daostudio.app/Contents/MacOS/daostudio

