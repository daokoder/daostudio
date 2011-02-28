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

#include<daoStudio.h>
#include<daoMonitor.h>

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
    setlocale( LC_CTYPE, "" );
    QApplication app( argc, argv );
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

	if( argc >1 && strcmp( argv[1], "--monitor" ) ==0 )
		return MonitorMain( app, argc, argv );

	return StudioMain( app, argc, argv );
}
