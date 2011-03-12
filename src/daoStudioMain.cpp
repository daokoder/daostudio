//=============================================================================
/*
   This file is a part of Dao Studio
   Copyright (C) 2009-2011, Fu Limin
Email: limin.fu@yahoo.com, phoolimin@gmail.com

Dao Studio is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
PARTICULAR PURPOSE. See the GNU General Public License for more details.
 */
//=============================================================================

#include<QtGui>
#include<cmath>
#include<QDir>

#include<daoMonitor.h>
#include<daoStudio.h>
#include<daoStudioMain.h>

QFont DaoStudioSettings::codeFont;
QString DaoStudioSettings::locale;
QString DaoStudioSettings::program;
QString DaoStudioSettings::program_path;
QString DaoStudioSettings::socket_suffix;
QString DaoStudioSettings::socket_script;
QString DaoStudioSettings::socket_stdin;
QString DaoStudioSettings::socket_stdout;
QString DaoStudioSettings::socket_debug;
QString DaoStudioSettings::socket_breakpoints;

void DaoStudioSettings::SetProgramPath( const char *cmd, const char *suffix )
{
	program = cmd;

    QFileInfo finfo( program ); 

    program_path = finfo.absolutePath();
    locale = QLocale::system().name();

	QString path = program_path;

	path = QDir::tempPath ();
	if( path.size() and path[ path.size()-1 ] != QDir::separator() )
		path += QDir::separator();

#ifdef Q_WS_WIN
	path = "\\\\.\\pipe\\";
#endif

	if( suffix ) socket_suffix = suffix;

	socket_script = path + ".daostudio.socket.script" + socket_suffix;
	socket_stdin = path + ".daostudio.socket.stdin" + socket_suffix;
	socket_stdout = path + ".daostudio.socket.stdout" + socket_suffix;
	socket_debug = path + ".daostudio.socket.debug" + socket_suffix;
	socket_breakpoints = path + ".daostudio.socket.breakpoints" + socket_suffix;
}
void DaoStudioSettings::AppendSuffix( const QString & suffix )
{
	socket_suffix += suffix;
	socket_script += suffix;
	socket_stdin += suffix;
	socket_stdout += suffix;
	socket_debug += suffix;
	socket_breakpoints += suffix;
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

int MonitorMain( QApplication & app, int argc, char *argv[] )
{
	DaoMonitor monitor( argv[0] );
	QRect size = QApplication::desktop()->screenGeometry();
	monitor.resize( 0.7*size.width(), 0.7*size.height() );
	app.setActiveWindow( & monitor );
	//monitor.showMaximized();
	monitor.show();
	return app.exec();
}

int main( int argc, char *argv[] )
{
	char *suffix = NULL;
	bool monitor = false;
	int i;
	for(i=1; i<argc; i++){
		if( strcmp( argv[i], "--monitor" ) ==0 ){
			monitor = true;
		}else if( strcmp( argv[i], "--socket-suffix" ) ==0 ){
			i += 1;
			if( i < argc ) suffix = argv[i];
		}
	}
	DaoInit();
	setlocale( LC_CTYPE, "" );
	QApplication app( argc, argv );
	DaoLanguages languages;
	DaoStudioSettings::SetProgramPath( argv[0], suffix );

	QFileInfo finfo( argv[0] ); 
	QTranslator translator;
	QString locale = QLocale::system().name();
	translator.load( finfo.absolutePath() + QString("/langs/daostudio_") + locale);
	app.installTranslator(&translator);

	DaoStudioSettings::codeFont.setWeight( 460 );
	DaoStudioSettings::codeFont.setFamily( "Courier 10 Pitch" );
	DaoStudioSettings::codeFont.setPointSize( 14 );
	QFontInfo fi( DaoStudioSettings::codeFont );
	if( fi.family() != "Courier 10 Pitch" )
		DaoStudioSettings::codeFont.setFamily( "Courier New" );

	if( monitor ) return MonitorMain( app, argc, argv );

	QLocalSocket socket;
    socket.connectToServer( DaoStudioSettings::socket_stdout );
	while( socket.waitForConnected( 100 ) ){ // IDE is running with the socket
		DaoStudioSettings::AppendSuffix( "@" );
		socket.disconnectFromServer();
		socket.connectToServer( DaoStudioSettings::socket_stdout );
	}
	return StudioMain( app, argc, argv );
}
