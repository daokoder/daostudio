/*
// Dao Studio
// http://daovm.net
//
// Copyright (C) 2009-2014, Limin Fu
// All rights reserved.
//
// Dao Studio is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
*/

#include<QtGui>
#include<cmath>
#include<QDir>

#include<daoMonitor.h>
#include<daoInterpreter.h>
#include<daoStudio.h>
#include<daoStudioMain.h>

QFont DaoStudioSettings::codeFont;
QString DaoStudioSettings::locale;
QString DaoStudioSettings::program;
QString DaoStudioSettings::program_path;
QString DaoStudioSettings::socket_suffix;
QString DaoStudioSettings::socket_monitor;
QString DaoStudioSettings::socket_data;
QString DaoStudioSettings::socket_script;
QString DaoStudioSettings::socket_stdin;
QString DaoStudioSettings::socket_stdout;
QString DaoStudioSettings::socket_stderr;
QString DaoStudioSettings::socket_execution;
QString DaoStudioSettings::socket_logger;
QString DaoStudioSettings::socket_debug;
QString DaoStudioSettings::socket_breakpoints;
QString DaoStudioSettings::socket_path;

void DaoStudioSettings::SetProgramPath( const char *cmd, const char *suffix )
{
	program = cmd;

    QFileInfo finfo( program ); 

    program_path = finfo.absolutePath();
    locale = QLocale::system().name().toLower();

	QString path = program_path;

	path = QDir::tempPath ();
	if( path.size() and path[ path.size()-1 ] != QDir::separator() )
		path += QDir::separator();

#ifdef Q_WS_WIN
	path = "\\\\.\\pipe\\";
#endif

	if( suffix ) socket_suffix = suffix;

	socket_monitor = path + ".daostudio.socket.monitor" + socket_suffix;
	socket_data = path + ".daostudio.socket.data" + socket_suffix;
	socket_script = path + ".daostudio.socket.script" + socket_suffix;
	socket_stdin = path + ".daostudio.socket.stdin" + socket_suffix;
	socket_stdout = path + ".daostudio.socket.stdout" + socket_suffix;
	socket_stderr = path + ".daostudio.socket.stderr" + socket_suffix;
	socket_execution = path + ".daostudio.socket.execution" + socket_suffix;
	socket_logger = path + ".daostudio.socket.logger" + socket_suffix;
	socket_debug = path + ".daostudio.socket.debug" + socket_suffix;
	socket_breakpoints = path + ".daostudio.socket.breakpoints" + socket_suffix;
	socket_path = path + ".daostudio.socket.path" + socket_suffix;
}
void DaoStudioSettings::AppendSuffix( const QString & suffix )
{
	socket_suffix += suffix;
	socket_monitor += suffix;
	socket_data += suffix;
	socket_script += suffix;
	socket_stdin += suffix;
	socket_stdout += suffix;
	socket_stderr += suffix;
	socket_execution += suffix;
	socket_logger += suffix;
	socket_debug += suffix;
	socket_breakpoints += suffix;
	socket_path += suffix;
}
int StudioMain( QApplication & app, int argc, char *argv[] )
{
	DaoStudio studio( argv[0] );
	//studio.resize( 800, 600 );
	//DaoInitLibrary( argv[0] );
	app.setActiveWindow( & studio );
	studio.showMaximized();
	studio.show();
	return app.exec();
}

int InterpreterMain( int argc, char *argv[] )
{
	QCoreApplication app( argc, argv );
	DaoInterpreter interperter( argv[0] );
	return app.exec();
};

int main( int argc, char *argv[] )
{
	char *suffix = NULL;
	bool monitor = false;
	bool interpreter = false;
	int i;
	for(i=1; i<argc; i++){
		if( strcmp( argv[i], "--monitor" ) ==0 ){
			monitor = true;
		}else if( strcmp( argv[i], "--interpreter" ) ==0 ){
			interpreter = true;
			setvbuf(stdout, NULL, _IONBF, 0);
		}else if( strcmp( argv[i], "--socket-suffix" ) ==0 ){
			i += 1;
			if( i < argc ) suffix = argv[i];
		}
	}
	DaoInit( argv[0] );
	setlocale( LC_CTYPE, "" );
	DaoStudioSettings::SetProgramPath( argv[0], suffix );
	if( interpreter ) return InterpreterMain( argc, argv );

	QApplication app( argc, argv );
	DaoLanguages languages;

	QFileInfo finfo( argv[0] ); 
	QString langPath = finfo.absolutePath();
#ifdef MAC_OSX
	langPath += "/../Resources";
#endif
	langPath += "/../langs";

	QTranslator translator;
	translator.load( langPath + QString("/daostudio_") + DaoStudioSettings::locale );
	app.installTranslator(&translator);

	int id1 = QFontDatabase::addApplicationFont(":/fonts/CourierCode/CourierCode-Roman.ttf");
	//qDebug() << id2 << QFontDatabase::applicationFontFamilies(id2);

	//DaoStudioSettings::codeFont.setWeight( 60 );
	DaoStudioSettings::codeFont.setFamily( "Courier Code" );
	DaoStudioSettings::codeFont.setPointSize( 16 );
	QFontInfo fi( DaoStudioSettings::codeFont );

	QLocalSocket socket;
    socket.connectToServer( DaoStudioSettings::socket_stdout );
	while( socket.waitForConnected( 100 ) ){ // IDE is running with the socket
		DaoStudioSettings::AppendSuffix( "@" );
		socket.disconnectFromServer();
		socket.connectToServer( DaoStudioSettings::socket_stdout );
	}
	return StudioMain( app, argc, argv );
}
