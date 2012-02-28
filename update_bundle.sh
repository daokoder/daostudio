cp dao/libdao.dylib daostudio.app/Contents/Frameworks/
cp dao/modules/auxlib/libdao_aux.dylib daostudio.app/Contents/Frameworks/

install_name_tool -id @executable_path/../Frameworks/libdao.dylib daostudio.app/Contents/Frameworks/libdao.dylib 
install_name_tool -id @executable_path/../Frameworks/libdao_aux.dylib daostudio.app/Contents/Frameworks/libdao_aux.dylib 
install_name_tool -change libdao.dylib @executable_path/../Frameworks/libdao.dylib daostudio.app/Contents/MacOS/daostudio
install_name_tool -change libdao_aux.dylib @executable_path/../Frameworks/libdao_aux.dylib daostudio.app/Contents/MacOS/daostudio

