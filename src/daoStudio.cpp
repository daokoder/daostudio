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

#include<daoStudio.h>
#include<daoStudioMain.h>


DaoStudioAbout::DaoStudioAbout( QWidget *parent ) : QDialog( parent )
{
	setupUi( this );
	label->setPixmap( label->pixmap()->scaled( 100, 100,
		Qt::IgnoreAspectRatio, Qt::SmoothTransformation ) );
	labAuthorChinese->setOpenExternalLinks( true );
	labAuthorEnglish->setOpenExternalLinks( true );
	QString locale = QLocale::system().name();
	if( locale.indexOf( "zh", Qt::CaseInsensitive ) ==0 ){
		labAuthorEnglish->hide();
		labNameEnglish->hide();
	}else{
		labAuthorChinese->hide();
		labNameChinese->hide();
	}
}
DaoHelpVIM::DaoHelpVIM( QWidget *parent ) : QDialog( parent )
{
	setupUi( this );
	treeWidget->setColumnWidth(0,250);
	treeWidget->setColumnWidth(1,300);
}

DaoTextBrowser::DaoTextBrowser( QWidget *parent ) : QTextBrowser( parent )
{
	setOpenExternalLinks( true );
}
void DaoTextBrowser::mousePressEvent ( QMouseEvent * event )
{
	QTextBrowser::mousePressEvent( event );
	emit signalFocusIn();
}
void DaoTextBrowser::keyPressEvent ( QKeyEvent * event )
{
	int key = event->key();
	int mod = event->modifiers();
	if( not (mod & Qt::AltModifier) ){
		QTextBrowser::keyPressEvent( event );
		return;
	}
	int i = tabWidget->currentIndex();
	int n = tabWidget->count();
	if( key == Qt::Key_Up || key == Qt::Key_Down ){
		studio->slotMaxConsole();
	}else if( key == Qt::Key_Left ){
		tabWidget->setCurrentIndex( (i+n-1)%n );
	}else if( key == Qt::Key_Right ){
		tabWidget->setCurrentIndex( (i+1)%n );
	}
}
DaoDocViewer::DaoDocViewer( QWidget *parent, const QString & path ) 
: DaoTextBrowser( parent )
{
	docRoot = path;
	docLang = DaoStudioSettings::locale.indexOf( "zh" ) == 0 ? "zh" : "en";
	if( QFile::exists( path + docLang + "/index.html" ) == 0 ) docLang = "en";

	setSearchPaths( QStringList( path + docLang + "/" ) );

	setHtml( "" );
}
void DaoDocViewer::setLanguage( const QString & lang )
{
	docLang = lang;
	if( QFile::exists( docRoot + docLang + "/index.html" ) == 0 ) docLang = "en";

	setSearchPaths( QStringList( docRoot + lang + "/" ) );
}
QVariant DaoTextBrowser::loadResource ( int type, const QUrl & url )
{
	QByteArray bytes = QTextBrowser::loadResource( type, url ).toByteArray();
	QString source = bytes;
	DString *dstring = DString_New();

	DString_SetBytes( dstring, bytes.data(), bytes.size() );
	if( DString_CheckUTF8( dstring ) ){
		source = QString::fromUtf8( bytes.data(), bytes.size() );
	}
	DString_Delete( dstring );
	return QVariant( source );
}
#if 0
void DaoDocViewer::mousePressEvent ( QMouseEvent * event )
{
	emit signalFocusIn();
	QTextBrowser::mousePressEvent( event );
}
#endif


DaoStudio::DaoStudio( const char *cmd ) : QMainWindow()
{
	int i;
	
	locale = DaoStudioSettings::locale;
	program = DaoStudioSettings::program;
	programPath = DaoStudioSettings::program_path;
	
	QCommonStyle style;
	QIcon book( QPixmap( ":/images/book.png" ) );
	QIcon dao( QPixmap( ":/images/dao.png" ) );
	QIcon daostudio( QPixmap( ":/images/daostudio.png" ) );
	QApplication::setWindowIcon( daostudio );
	
	vmState = DAOCON_READY;
	configured = 0;
	
	setupUi(this);
	
	vmSpace = wgtConsole->vmSpace;
	wgtConsole->tabWidget = wgtEditorTabs;
	wgtConsole->studio = this;
	wgtConsole->SetModeSelector( wgtConsoleMode );
	wgtMonitor->studio = this;
	wgtMonitor->SetVmSpace( vmSpace );
	
	//QPalette p = palette();
	//p.setColor( QPalette::Text, QColor( 255, 255, 0, 200) );
	//wgtEditorTabs->setPalette( p );
	
	QWidget *wgt = wgtLogText;
	wgtLogText = new DaoLogBrowser( tab );
	wgtEditorTabs->addTab( wgtLogText, tr("Log") );
	wgtLogText->setMinimumSize( wgt->minimumSize() );
	((DaoLogBrowser*)wgtLogText)->studio = this;
	((DaoLogBrowser*)wgtLogText)->tabWidget = wgtEditorTabs;
	connect( wgtLogText, SIGNAL(signalFocusIn()), this, SLOT(slotMaxEditor()) );
	connect( wgtGroupMonitor, SIGNAL(clicked()), this, SLOT(slotMaxMonitor()) );
	connect( wgtMonitorArea->verticalScrollBar(), SIGNAL(valueChanged(int)),
			this, SLOT(slotMaxMonitor(int)) );
	connect( wgtMonitorArea->verticalScrollBar(), SIGNAL(sliderMoved(int)),
			this, SLOT(slotMaxMonitor(int)) );
	connect( wgtMonitorArea->verticalScrollBar(), SIGNAL(sliderPressed()),
			this, SLOT(slotMaxMonitor()) );

#ifdef MAC_OSX
	QString docRoot = programPath + "/../Frameworks/share/dao/doc/html/";
#else
	QString docRoot = programPath + "/../share/dao/doc/html/";
#endif
	docViewer = new DaoDocViewer( wgtEditorTabs, docRoot );
	docViewer->studio = this;
	docViewer->tabWidget = wgtEditorTabs;
	docViewer->setFontFamily( DaoStudioSettings::codeFont.family() );
	wgtEditorTabs->addTab( docViewer, book, "" );
	wgtEditorTabs->setTabsClosable(1);
	wgtEditorTabs->setFixedHeight( 200 );
	wgtEditorTabs->setUsesScrollButtons( true );
	delete wgtEditorTabs->widget(0);
	delete wgtEditorTabs->widget(0);

	docViewer->setSource( QUrl( "index.html" ) );
	
	wgtEditorTabs->setMinimumHeight( 80 );
	wgtEditorTabs->setMaximumHeight( 5000 );
	wgtGroupMonitor->setMinimumHeight( 80 );
	wgtGroupMonitor->setMaximumHeight( 5000 );
	wgtConsole->setMinimumHeight( 80 );
	wgtConsole->setMaximumHeight( 5000 );
	mainSplitter->setStretchFactor( 1, 4 );
	
#if 0
	wgtFontSize = new QComboBox( DConToolBar );
	wgtEditorColor = new QComboBox( DConToolBar );
	QComboBox *wgtConsoleColor = new QComboBox( DConToolBar );
	DConToolBar->insertWidget( actionMSplit, wgtFontSize );
	DConToolBar->insertWidget( actionMSplit, wgtEditorColor );
	DConToolBar->insertWidget( actionMSplit, wgtConsoleColor );
#endif
	QStringList sizes, colors;
	for(i=12; i<=26; i++) sizes<<QString::number(i);
	colors<<tr("White")<<tr("Light")<<tr("Dark")<<tr("Black");
#ifndef WIN32
	// Builtin Courier Code doesn't show up with this filter:
	wgtFontFamily->setFontFilters( QFontComboBox::MonospacedFonts );
#endif
	wgtFontFamily->setCurrentFont( DaoStudioSettings::codeFont );
	wgtFontSize->addItems( sizes );
	wgtEditorColor->addItems( colors );
	wgtConsoleColor->addItems( colors );
	wgtFontSize->setToolTip( tr("Default text font size") );
	wgtEditorColor->setToolTip( tr("Background color of the code editor") );
	wgtConsoleColor->setToolTip( tr("Background color of the script console") );
	wgtEditorMode->addItem( tr("Normal") );
	wgtEditorMode->addItem( tr("DS-VIM") );
	//wgtEditorMode->addItem( tr("Emacs") );
	wgtConsoleMode->addItem( tr("Normal") );
	wgtConsoleMode->addItem( tr("DS-VIM") );
	//wgtConsoleMode->addItem( tr("Emacs") );
	wgtTabVisibility->setCurrentIndex(1);

	wgtDebuggerList->addItem( tr("Console Debugger") );
	wgtDebuggerList->addItem( tr("Graphical Debugger") );
	wgtProfilerList->addItem( tr("Console Profiler") );
	wgtDebuggerList->setCurrentIndex(1);
	connect( wgtDebuggerList, SIGNAL(currentIndexChanged(int)), this, SLOT(slotSwitchDebugger(int)) );
	connect( wgtProfilerList, SIGNAL(currentIndexChanged(int)), this, SLOT(slotSwitchProfiler(int)) );

	fileFilters << ( tr("All Files") + " (*)" );
	fileFilters << ( tr("Dao Files") + " (*.dao)" );
	fileFilters << ( tr("C/C++ Header Files") + " (*.h *.hpp *.hxx)" );
	fileFilters << ( tr("C Files") + " (*.c)" );
	fileFilters << ( tr("C++ Files") + " (*.cc *.cpp *.cxx *.c++)" );
	fileFilters << ( tr("C/C++ Files") + " (*.h *.hpp *.hxx *.c *.cc *.cpp *.cxx *.c++)" );
	fileFilters << ( tr("Lua Files") + " (*.lua)" );
	fileFilters << ( tr("Python Files") + " (*.py)" );
	fileFilters << ( tr("Perl Files") + " (*.pl *.pm)" );
	fileFilters << ( tr("PipeLang Files") + " (*.pld)" );
	fileFilters << ( tr("SDML Files") + " (*.sdml)" );
	fileFilters << ( tr("Text Files") + " (*.txt)" );
	fileFilters << ( tr("HTML Files") + " (*.html *.htm)" );
	fileFilters << ( tr("TEX Files") + " (*.tex)" );
	fileFilters << ( tr("CGI Files") + " (*.cgi)" );
	fileFilters << ( tr("Qt Files") + " (*.pro *.qrc *.ui)" );
	
	wgtFileSuffix->addItems( fileFilters );
	wgtFileSuffix->setCurrentIndex(1);
	
	connect( wgtPathList, SIGNAL(currentIndexChanged(int)), this, SLOT(slotPathList(int)) );
	connect( wgtFileSuffix, SIGNAL(currentIndexChanged(int)), this, SLOT(slotFileSuffix(int)) );
	
	connect( wgtFontSize, SIGNAL(currentIndexChanged(int)), this, SLOT(slotFontSize(int)) );
	connect( wgtEditorColor, SIGNAL(currentIndexChanged(int)), this, SLOT(slotEditorColor(int)) );
	connect( wgtConsoleColor, SIGNAL(currentIndexChanged(int)), this, SLOT(slotConsoleColor(int)) );
	connect( wgtTabVisibility, SIGNAL(currentIndexChanged(int)), this, SLOT(slotTabVisibility(int)) );
	connect( wgtFontFamily, SIGNAL(currentFontChanged (const QFont&)), 
		this, SLOT(slotFontFamily(const QFont&)) );
	
	wgtFontSize->setCurrentIndex(4);
	wgtEditorColor->setCurrentIndex(2);
	wgtConsoleColor->setCurrentIndex(2);
	slotConsoleColor(2);
	
	wgtFind = new QLineEdit( DConToolBar );
	wgtReplace = new QLineEdit( DConToolBar );
	chkReplaceAll = new QCheckBox( tr("All"), DConToolBar );
	chkCaseSensitive = new QCheckBox( tr("Case Sensitive"), DConToolBar );
	wgtFind->setMaximumWidth( 120 );
	wgtReplace->setMaximumWidth( 120 );
	DConToolBar->insertWidget( actionReplace, wgtFind );
	DConToolBar->addWidget( wgtReplace );
	DConToolBar->addWidget( chkReplaceAll );
	DConToolBar->addWidget( chkCaseSensitive );
	
	
	btSetPath->setIcon( style.standardIcon( QStyle::SP_ArrowUp ) );
	btResetPath->setIcon( style.standardIcon( QStyle::SP_ArrowDown ) );
	connect( btSetPath, SIGNAL(clicked()), this, SLOT(slotSetPathWorking()) );
	connect( btResetPath, SIGNAL(clicked()), this, SLOT(slotSetPathBrowsing()) );
	
	actionNew->setIcon( style.standardIcon( QStyle::SP_FileIcon ) );
	actionOpen->setIcon( style.standardIcon( QStyle::SP_DialogOpenButton ) );
	actionSave->setIcon( style.standardIcon( QStyle::SP_DialogSaveButton ) );
	actionUndo->setIcon( style.standardIcon( QStyle::SP_ArrowLeft ) );
	actionRedo->setIcon( style.standardIcon( QStyle::SP_ArrowRight ) );
	//actionCut->setIcon( style.standardIcon( QStyle::SP_DialogCancelButton ) );
	//actionStart->setIcon( style.standardIcon( QStyle::SP_DialogYesButton ) );
	//actionStop->setIcon( style.standardIcon( QStyle::SP_MediaStop ) );
	connect( actionNew, SIGNAL(triggered()), this, SLOT(slotNew()) );
	connect( actionOpen, SIGNAL(triggered()), this, SLOT(slotOpen()) );
	connect( actionSave, SIGNAL(triggered()), this, SLOT(slotSave()) );
	connect( actionSave_as, SIGNAL(triggered()), this, SLOT(slotSaveAs()) );
	connect( actionPrint, SIGNAL(triggered()), this, SLOT(slotPrint()) );
	connect( actionQuit, SIGNAL(triggered()), this, SLOT(close()) );
	connect( actionUndo, SIGNAL(triggered()), this, SLOT(slotUndo()) );
	connect( actionRedo, SIGNAL(triggered()), this, SLOT(slotRedo()) );
	connect( actionCopy, SIGNAL(triggered()), this, SLOT(slotCopy()) );
	connect( actionCut, SIGNAL(triggered()), this, SLOT(slotCut()) );
	connect( actionPaste, SIGNAL(triggered()), this, SLOT(slotPaste()) );
	connect( actionStart, SIGNAL(triggered()), this, SLOT(slotStart()) );
	connect( actionCompile, SIGNAL(triggered()), this, SLOT(slotCompile()) );
	connect( actionDebugger, SIGNAL(triggered(bool)), this, SLOT(slotDebugger(bool)) );
	connect( actionProfiler, SIGNAL(triggered(bool)), this, SLOT(slotProfiler(bool)) );
	connect( actionStop, SIGNAL(triggered()), this, SLOT(slotStop()) );
	connect( actionMSplit, SIGNAL(triggered(bool)), this, SLOT(slotMSplit(bool)) );
	connect( actionFind, SIGNAL(triggered()), this, SLOT(slotFind()) );
	connect( actionReplace, SIGNAL(triggered()), this, SLOT(slotReplace()) );
	connect( actionRestartMonitor, SIGNAL(triggered()), this, SLOT(slotRestartMonitor()) );
	connect( actionTips, SIGNAL(triggered()), this, SLOT(slotTips()) );
	connect( actionDocs, SIGNAL(triggered()), this, SLOT(slotDocs()) );
	connect( actionHelpVIM, SIGNAL(triggered()), this, SLOT(slotHelpVIM()) );
	connect( actionAbout, SIGNAL(triggered()), this, SLOT(slotAbout()) );
	
	connect( docViewer, SIGNAL(signalFocusIn()), this, SLOT(slotMaxEditor()) );
	connect( wgtConsole, SIGNAL(signalFocusIn()), this, SLOT(slotMaxConsole()) );
	connect( wgtConsole, SIGNAL(signalLoadURL(const QString&)), 
		this, SLOT(slotLoadURL(const QString&)) );
	connect( wgtEditorTabs, SIGNAL(signalFocusIn()), this, SLOT(slotMaxEditor()) );
	connect( wgtEditorTabs, SIGNAL(tabCloseRequested(int)), this, SLOT(slotCloseEditor(int)) );
	connect( wgtEditorTabs, SIGNAL(currentChanged(int)), this, SLOT(slotMaxEditor(int)) );
	
	connect( wgtFileList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), 
		this, SLOT(slotFileActivated(QListWidgetItem*)) );
	connect( wgtDocList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), 
		this, SLOT(slotViewDocument(QListWidgetItem*)) );
	
	
	slotWriteLog( tr("start Dao Studio") + " (" + program + ")" );
	SetPathBrowsing( "." );
	DaoVmSpace_AddPath( vmSpace, programPath.toLocal8Bit().data() );
	
	QIcon fico = style.standardIcon( QStyle::SP_FileIcon );
	wgtDocList->clear();
	new QListWidgetItem(  fico, tr("English"), wgtDocList, QListWidgetItem::UserType );
	new QListWidgetItem(  fico, tr("Chinese"), wgtDocList, QListWidgetItem::UserType );
	connect( & watcher, SIGNAL(directoryChanged(const QString &)),
		this, SLOT(SetPathBrowsing(const QString &)) );
	
	monitor = new QProcess();
	monitor->setProcessChannelMode(QProcess::MergedChannels);
	wgtConsole->monitor = monitor;
	//monitor->setProcessChannelMode(QProcess::MergedChannels);
	//monitor->start( "valgrind --tool=memcheck --leak-check=full --dsymutil=yes ./DaoMonitor" );
	//monitor->start( "echo \"r\" | gdb ./DaoMonitor" );
	//monitor->start( "./DaoMonitor", QIODevice::ReadWrite|QIODevice::Unbuffered );
	//monitor->start( "vi" );
	socket.connectToServer( DaoStudioSettings::socket_script );
	bool newStart = true;
	bool add_suffix = false;
	if( socket.waitForConnected( 100 ) ){
		int ret = QMessageBox::warning(this, tr("DaoStudio"),
			tr("DaoMonitor is running. Use it?") + "\n\n" +
			tr("Warning: if you use the running DaoMonitor, "
			"its standard output will not be available in DaoStudio!"),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
		newStart = (ret == QMessageBox::No);
		add_suffix = true;
	}
	if( newStart ){
		QString command = program;
#ifdef Q_WS_WIN
		command = "\"" + program + "\"";
#endif
		command += " --interpreter";
		if( add_suffix ){
			DaoStudioSettings::AppendSuffix( "@" );
			socket.connectToServer( DaoStudioSettings::socket_script );
			// search for unused socket:
			while( socket.waitForConnected( 100 ) ){
				DaoStudioSettings::AppendSuffix( "@" );
				socket.disconnectFromServer();
				socket.connectToServer( DaoStudioSettings::socket_script );
			}
			command += " --socket-suffix " + DaoStudioSettings::socket_suffix;
		}
		if( add_suffix == false and DaoStudioSettings::socket_suffix != "" )
			command += " --socket-suffix " + DaoStudioSettings::socket_suffix;
		
		monitor->start( command, QIODevice::ReadWrite | QIODevice::Unbuffered );
		monitor->waitForStarted(500);
	}else{
		slotWriteLog( tr("connected to running") + " DaoMonitor" );
	}
	SetPathWorking( "." );
	connect( monitor, SIGNAL(readyReadStandardOutput()),
		wgtConsole, SLOT(slotReadStdOut()));
	//connect( monitor, SIGNAL(readyReadStandardError()),
	//    wgtConsole, SLOT(slotReadStdError()));
	//connect( monitor, SIGNAL(readyRead()),
	//    wgtConsole, SLOT(slotReadStdOut()));
	connect( monitor, SIGNAL(finished(int, QProcess::ExitStatus)),
		this, SLOT(slotRestartMonitor(int, QProcess::ExitStatus)));
	
	QStatusBar *bar = statusBar();
	labTimer = new QLabel( "00:00:00.000", bar );
	bar->addPermanentWidget( new QLabel(bar), 10 );
	bar->addPermanentWidget( labTimer, 0 );
	
	connect( &timer, SIGNAL(timeout()), this, SLOT(slotTimeOut()));
	timer.start( 97 );

	if( QFile::exists( DaoStudioSettings::socket_path ) )
		QFile::remove( DaoStudioSettings::socket_path );
	pathServer.listen( DaoStudioSettings::socket_path );
	connect( & pathServer, SIGNAL(newConnection()), this, SLOT(slotSetPathWorking2()));
}

DaoStudio::~DaoStudio()
{
	SaveSettings();
	if( vmState != DAOCON_READY ) wgtConsole->Quit();
	disconnect( monitor, SIGNAL(finished(int, QProcess::ExitStatus)),
		this, SLOT(slotRestartMonitor(int, QProcess::ExitStatus)));
	monitor->close();
}

void DaoStudio::LoadPath( QListWidget *wgtList, const QString & path, const QString & suffix )
{
	const char *s1 = ":/images/daofile.png";
	const char *s2 = ":/images/plot.png";
	
	QRegExp regex( "\\*\\.\\w[\\w+]*" );
	QString suffix2;
	int offset = 0;
	int pos = 0;
	while( (pos = regex.indexIn( suffix, offset )) >=0 ){
		suffix2 += regex.cap(0) + " ";
		offset = pos + regex.matchedLength();
	}
	
	QCommonStyle style;
	QIcon fico( QPixmap( wgtList == wgtFileList ? s1 : s2 ) );
	QIcon dico = style.standardIcon( QStyle::SP_DirIcon );
	QDir dir( path, suffix2 );
	dir.setFilter( QDir::AllDirs | QDir::Files );
	dir.setSorting( QDir::DirsFirst | QDir::Name );
	QFileInfoList list = dir.entryInfoList();
	wgtList->clear();
	for (int i = 0; i < list.size(); ++i) {
		QFileInfo info = list.at(i);
		new QListWidgetItem(  info.isDir() ? dico : fico, info.fileName(), 
			wgtList, QListWidgetItem::UserType + info.isDir() );
	}
}
void DaoStudio::SendPathWorking()
{
	socket.connectToServer( DaoStudioSettings::socket_script );
	if( socket.waitForConnected( 500 ) ){
		socket.putChar( DAO_SET_PATH );
		socket.write( pathWorking.toUtf8().data() );
		socket.flush();
		socket.waitForDisconnected();
	}
}
void DaoStudio::SetPathWorking( const QString & path )
{
	QDir dir( path );
	pathWorking = dir.absolutePath() + "/";
	wgtPathWorking->setText( pathWorking );
	SendPathWorking();
	DaoVmSpace_SetPath( vmSpace, pathWorking.toLocal8Bit().data() );
	slotWriteLog( tr("change the working path to") + " \"" + pathWorking + "\"" );
	if( pathUsage.contains( pathWorking ) ){
		pathUsage[pathWorking] += 1;
	}else{
		pathUsage[pathWorking] = 1;
		wgtPathList->addItem( pathWorking );
	}
}
void DaoStudio::SetPathBrowsing( const QString & path )
{
	LoadPath( wgtFileList, path, wgtFileSuffix->currentText() );
	QDir dir( path );
	pathBrowsing = dir.absolutePath() + "/";
	int id = wgtPathList->findText( pathBrowsing );
	if( id >=0 ){
		wgtPathList->setCurrentIndex( id );
	}else{
		wgtPathList->addItem( pathBrowsing );
		wgtPathList->setCurrentIndex( wgtPathList->count()-1 );
	}
	QStringList paths = watcher.directories();
	if( paths.size() ) watcher.removePaths( paths );
	watcher.addPath( pathBrowsing );
}
void DaoStudio::slotSetPathWorking()
{
	if( pathWorking == pathBrowsing ) return;
	pathWorking = pathBrowsing;
	wgtPathWorking->setText( pathWorking );

	QString script = pathWorking;
	script.replace( " ", "\\\\ " );
	script = "std.path( '" + script + "', $set );\n";
	wgtConsole->setFocus();
	wgtConsole->ensureCursorVisible();
	wgtConsole->InsertScript( script );
	//SendPathWorking();
	DaoVmSpace_SetPath( vmSpace, pathWorking.toLocal8Bit().data() );
	slotWriteLog( tr("change the working path to") + " \"" + pathWorking + "\"" );
	if( pathUsage.contains( pathWorking ) ){
		pathUsage[pathWorking] += 1;
	}else{
		pathUsage[pathWorking] = 1;
		wgtPathList->addItem( pathWorking );
	}
}
void DaoStudio::slotSetPathWorking2()
{
	QLocalSocket *socket = pathServer.nextPendingConnection();
	socket->waitForReadyRead();
	QByteArray output = socket->readAll();
	SetPathWorking( output );
}
void DaoStudio::slotSetPathBrowsing()
{
	if( pathWorking == pathBrowsing ) return;
	pathBrowsing = pathWorking;
	LoadPath( wgtFileList, pathBrowsing, wgtFileSuffix->currentText() );
}
void DaoStudio::slotPathList(int id)
{
	SetPathBrowsing( wgtPathList->itemText( id ) );
}
void DaoStudio::showEvent ( QShowEvent * event )
{
	QMainWindow::showEvent( event );
}
void DaoStudio::closeEvent ( QCloseEvent *e )
{
	if( vmState != DAOCON_READY ){
		int ret = QMessageBox::warning(this, tr("DaoStudio"),
			tr("The execution is not done yet.") + "\n" + tr("Quit anyway?"),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
		if( ret == QMessageBox::No ){
			e->ignore();
			return;
		}
	}
	int i;
	for(i=wgtEditorTabs->count()-1; i>=ID_EDITOR; i--){
		slotCloseEditor( ID_EDITOR );
	}
	if( wgtEditorTabs->count() > ID_EDITOR ){
		e->ignore();
		return;
	}
	wgtConsole->Quit();
	slotWriteLog( tr("quit Dao Studio") ); // TODO: write log to file
	disconnect( monitor, SIGNAL(finished(int, QProcess::ExitStatus)),
		this, SLOT(slotRestartMonitor(int, QProcess::ExitStatus)));
	monitor->close();
	monitor->waitForFinished();
}
void DaoStudio::slotTimeOut()
{
	int ms = time.elapsed();
	int sec = ms / 1000;
	int min = sec / 60;
	int hr = min / 60;
	char buf[100];
	ms -= sec * 1000;
	sec -= min * 60;
	min -= hr * 60;
	sprintf( buf, "%02i:%02i:%02i.%03i", hr, min, sec, ms );
	if( vmState != DAOCON_READY ) labTimer->setText( buf );
}
void DaoStudio::RestartMonitor()
{
	QByteArray output = monitor->readAllStandardOutput();
	QString text = QString::fromUtf8( output.data(), output.size() );
	wgtConsole->slotPrintOutput( text );
	output = monitor->readAllStandardError();
	text = QString::fromUtf8( output.data(), output.size() );
	wgtConsole->slotPrintOutput( text );
	
	monitor->start( program + " --interpreter", QIODevice::ReadWrite | QIODevice::Unbuffered );
	monitor->waitForStarted();
	do{ socket.connectToServer( DaoStudioSettings::socket_script );
	}while( socket.state() != QLocalSocket::ConnectedState );
	socket.disconnectFromServer();
	connect( monitor, SIGNAL(readyReadStandardOutput()),
		wgtConsole, SLOT(slotReadStdOut()));
	//connect( monitor, SIGNAL(readyReadStandardError()),
	//    wgtConsole, SLOT(slotReadStdError()));
	//connect( monitor, SIGNAL(readyRead()),
	//    wgtConsole, SLOT(slotReadStdOut()));
	connect( monitor, SIGNAL(finished(int, QProcess::ExitStatus)),
		this, SLOT(slotRestartMonitor(int, QProcess::ExitStatus)));
	SendPathWorking();
	slotWriteLog( tr("Restart Dao Virtual Machine") );
	wgtConsole->SetState( DAOCON_READY );
	wgtMonitor->Reset();
}
void DaoStudio::slotRestartMonitor2()
{
	slotWriteLog( tr("Dao Virtual Machine exited for unknown reason") );
	RestartMonitor();
}
void DaoStudio::slotRestartMonitor( int code, QProcess::ExitStatus status )
{
	QString log;
	if( status == QProcess::CrashExit ){
		log = tr("Dao Virtual Machine crashed unexpectly");
	}else{
		log = tr("Dao Virtual Machine exited with code");
		log += " " + QString::number( code );
	}
	slotWriteLog( log );
	RestartMonitor();
}
void DaoStudio::slotWriteLog( const QString & log )
{
	QString date = QDate::currentDate().toString( Qt::ISODate );
	QString time = QTime::currentTime().toString();
	wgtLogText->append( date + ", " + time + ": " + log + "." );
	wgtLogText->moveCursor( QTextCursor::End );
}
void DaoStudio::slotFileSuffix( int id )
{
	LoadPath( wgtFileList, pathBrowsing, wgtFileSuffix->currentText() );
}
void DaoStudio::slotFontSize( int size )
{
	int i, n = wgtEditorTabs->count();
	DaoStudioSettings::codeFont.setPointSize( size + 10 );
	for(i=ID_EDITOR; i<n; i++){
		DaoEditor *editor = (DaoEditor*) wgtEditorTabs->widget( i );
		editor->SetFontSize( size + 10 );
	}
	wgtConsole->SetFontSize( size + 10 );

	docViewer->setFont( DaoStudioSettings::codeFont );
}
void DaoStudio::slotFontFamily( const QFont & font )
{
	QString family = font.family();
	int i, n = wgtEditorTabs->count();
	DaoStudioSettings::codeFont.setFamily( family );
	for(i=ID_EDITOR; i<n; i++){
		DaoEditor *editor = (DaoEditor*) wgtEditorTabs->widget( i );
		editor->SetFontFamily( family );
	}
	wgtConsole->SetFontFamily( family );

	docViewer->setFontFamily( family );
}
void DaoStudio::slotEditorColor( int scheme )
{
	int i, n = wgtEditorTabs->count();
	for(i=ID_EDITOR; i<n; i++){
		DaoEditor *editor = (DaoEditor*) wgtEditorTabs->widget( i );
		editor->SetColorScheme( scheme );
	}
	QString style1 = "font-size: 14pt;";
	QString style2;
	switch( scheme ){
	case 0 : style1 += "color: black;"; style2 = "background-color: #FFF;"; break;
	case 1 : style1 += "color: black;"; style2 = "background-color: #DDD;"; break;
	case 2 : style1 += "color: white;"; style2 = "background-color: #222;"; break;
	default : style1 += "color: white;"; style2 = "background-color: #000;"; break;
	}
	wgtLogText->setStyleSheet( style1 );
	wgtLogText->viewport()->setStyleSheet( style2 );
}
void DaoStudio::slotConsoleColor( int scheme )
{
	wgtConsole->SetColorScheme( scheme );
}
void DaoStudio::slotTabVisibility( int id )
{
	wgtConsole->SetTabVisibility( id );
	int i, n = wgtEditorTabs->count();
	for(i=ID_EDITOR; i<n; i++){
		DaoEditor *editor = (DaoEditor*) wgtEditorTabs->widget( i );
		editor->SetTabVisibility( id );
	}
}
DaoEditor* DaoStudio::NewEditor( const QString & name, const QString & tip )
{
	DaoEditor *editor = new DaoEditor( wgtEditorTabs, & wordList );
	int id = wgtEditorTabs->addTab( editor, name );
	editor->studio = this;
	editor->SetModeSelector( wgtEditorMode );
	editor->SetColorScheme( wgtEditorColor->currentIndex() );
	editor->SetTabVisibility( wgtTabVisibility->currentIndex() );
	editor->SetFontSize( wgtFontSize->currentIndex() + 10 );
	editor->SetFontFamily( wgtFontFamily->currentText() );
	connect( editor, SIGNAL(signalFocusIn()), this, SLOT(slotMaxEditor()) );
	connect( editor, SIGNAL(signalTextChanged(bool)), this, SLOT(slotTextChanged(bool)) );
	wgtEditorTabs->setTabToolTip( id, tip );
	wgtEditorTabs->setCurrentIndex( id );
	slotMaxEditor();
	return editor;
}
void DaoStudio::slotFileActivated( QListWidgetItem *item )
{
	QString file = pathBrowsing + item->text();
	int i, n = wgtEditorTabs->count();
	for(i=ID_EDITOR; i<n; i++){
		if( wgtEditorTabs->tabToolTip(i) == file ){
			slotMaxEditor();
			wgtEditorTabs->setCurrentIndex( i );
			return;
		}
	}
	if( item->type() == QListWidgetItem::UserType ){
		DaoEditor *editor = NewEditor( item->text(), file );
		slotMaxEditor();
		editor->LoadFile( file );
		editor->ExtractWords();
		//wordList.Extract( editor->toPlainText() );
		if( lastCursor.contains( file ) ){
			QTextCursor cursor = editor->textCursor();
			cursor.setPosition( lastCursor[file].toInt() );
			editor->setTextCursor( cursor );
		}
		slotWriteLog( tr("open source file") + " \"" + file + "\"" );
	}else{
		SetPathBrowsing( file );
	}
}
void DaoStudio::slotViewFrame( QListWidgetItem * )
{
}
void DaoStudio::slotViewDocument( QListWidgetItem *item )
{
	const char *const langs[] = { "en", "zh" };
	wgtEditorTabs->setCurrentIndex(ID_DOCVIEW);
	slotMaxEditor();
	docViewer->setLanguage( langs[ wgtDocList->currentRow() ] );
	docViewer->reload();
}
void DaoStudio::slotTextChanged( bool changed )
{
	DaoEditor *editor = (DaoEditor*) QObject::sender();
	int id = wgtEditorTabs->indexOf( editor );
	if( id <0 ) return;
	QString name = editor->FileName();
	if( changed ){
		if( name == "" ) name = "#new";
		wgtEditorTabs->setTabText( id, name +"*" );
	}else{
		wgtEditorTabs->setTabText( id, name );
	}
}
void DaoStudio::slotNew()
{
	NewEditor( "#NewFile", "New File" );
	slotWriteLog( tr("create new source file") );
}
void DaoStudio::slotOpen()
{
	QString name = QFileDialog::getOpenFileName( this, tr("Open file"), 
		pathBrowsing, fileFilters.join( ";;" ) );
	if( name == "" ) return;
	QFileInfo info( name );
	SetPathBrowsing( info.absolutePath() );
	if( info.fileName() != "" ){
		DaoEditor *editor = NewEditor( info.fileName(), name );
		editor->LoadFile( name );
		editor->ExtractWords();
		if( lastCursor.contains( name ) ){
			QTextCursor cursor = editor->textCursor();
			cursor.setPosition( lastCursor[name].toInt() );
			editor->setTextCursor( cursor );
		}
		//wordList.Extract( editor->toPlainText() );
		slotWriteLog( tr("open source file") + " \"" + name + "\"" );
	}
}
void DaoStudio::slotSave()
{
	int id = wgtEditorTabs->currentIndex();
	if( id ==ID_DOCVIEW ) return;
	DaoEditor *editor = (DaoEditor*) wgtEditorTabs->currentWidget();
	QString name = editor->FilePath();
	bool update = false;
	if( name.size() ==0 ){
		name = QFileDialog::getSaveFileName( this, tr("Set file name to save"),
			pathBrowsing, wgtFileSuffix->currentText() );
		update = true;
		if( name == "" ) return;
	}
	editor->Save( name );
	editor->ExtractWords();
	//wordList.Extract( editor->toPlainText() );
	slotWriteLog( tr("save source file") + " \"" + editor->FilePath() + "\"" );
	if( update ){
		SetPathBrowsing( pathBrowsing );
		wgtEditorTabs->setTabText( id, editor->FileName() );
		wgtEditorTabs->setTabToolTip( id, name );
	}
}
void DaoStudio::slotSaveAs()
{
	int id = wgtEditorTabs->currentIndex();
	if( id ==ID_DOCVIEW ) return;
	DaoEditor *editor = (DaoEditor*) wgtEditorTabs->currentWidget();
	QString name = QFileDialog::getSaveFileName( this, tr("Set file name to save"),
		pathBrowsing + "/" + editor->FileName(), wgtFileSuffix->currentText() );
	editor->Save( name );
	slotWriteLog( tr("save source file") + " \"" + editor->FilePath() + "\""
		+ tr("as") + " \"" + name + "\"" );
	SetPathBrowsing( pathBrowsing );
	wgtEditorTabs->setTabText( id, editor->FileName() );
	wgtEditorTabs->setTabToolTip( id, name );
}
void DaoStudio::slotPrint()
{
	QPrinter printer;
	//printer.setOrientation( QPrinter::QPrinter::Landscape );
#if 0
	qreal lm, tm, rm, bm;
	printer.getPageMargins( &lm, &tm, &rm, &bm, QPrinter::Millimeter );
	printer.setPageMargins( lm/2, tm/2, rm/2, bm/2, QPrinter::Millimeter );
	printer.setFullPage( true );
#endif
	QPrintDialog dialog( & printer, this );
	if( wgtEditorTabs->height() > 200 ){
		int id = wgtEditorTabs->currentIndex();
		if( id < ID_EDITOR ) return;
		DaoEditor *editor = (DaoEditor*) wgtEditorTabs->currentWidget();
		printer.setDocName( editor->FileName() );
		printer.setOutputFileName( editor->FileName() );
		dialog.setWindowTitle( tr("Printing: ") + editor->FileName() );
		if (editor->textCursor().hasSelection()){
			dialog.addEnabledOption(QAbstractPrintDialog::PrintSelection);
			if (dialog.exec() != QDialog::Accepted) return;
			editor->print( & printer );
		}else{
			if (dialog.exec() != QDialog::Accepted) return;
			DaoEditor editor2( NULL, NULL );
			editor2.setPlainText( editor->toPlainText() );
			editor2.PreparePrinting( editor->FilePath() );
			editor2.print( & printer );
		}
	}else if( wgtConsole->height() > 200 ){
		printer.setDocName( "daostudio_console" );
		printer.setOutputFileName( "daostudio_console" );
		dialog.setWindowTitle( tr("Printing console output") );
		if (wgtConsole->textCursor().hasSelection())
			dialog.addEnabledOption(QAbstractPrintDialog::PrintSelection);
		if (dialog.exec() != QDialog::Accepted) return;
		wgtConsole->print( & printer );
	}
}
void DaoStudio::slotUndo()
{
	if( wgtEditorTabs->height() > 200 ){
		if( wgtEditorTabs->currentIndex() >= ID_EDITOR ){
			DaoTextEdit *editor = (DaoTextEdit*) wgtEditorTabs->currentWidget();
			editor->Undo();
		}else{
			docViewer->backward();
		}
	}else if( wgtConsole->height() > 200 ){
		wgtConsole->Undo();
	}
}
void DaoStudio::slotRedo()
{
	if( wgtEditorTabs->height() > 200 ){
		if( wgtEditorTabs->currentIndex() >= ID_EDITOR ){
			DaoTextEdit *editor = (DaoTextEdit*) wgtEditorTabs->currentWidget();
			editor->Redo();
		}else{
			docViewer->forward();
		}
	}else if( wgtConsole->height() > 200 ){
		wgtConsole->Redo();
	}
}
void DaoStudio::slotCopy()
{
	if( wgtEditorTabs->height() > 200 ){
		if( wgtEditorTabs->currentIndex() ){
			QTextEdit *editor = (QTextEdit*) wgtEditorTabs->currentWidget();
			editor->copy();
		}else{
			docViewer->copy();
			//			docViewer->triggerPageAction( QWebPage::Copy );
		}
	}else if( wgtConsole->height() > 200 ){
		wgtConsole->copy();
	}
}
void DaoStudio::slotCut()
{
	if( wgtEditorTabs->height() > 200 && wgtEditorTabs->currentIndex() ){
		QTextEdit *editor = (QTextEdit*) wgtEditorTabs->currentWidget();
		editor->cut();
	}else if( wgtConsole->height() > 200 ){
		wgtConsole->cut();
	}
}
void DaoStudio::slotPaste()
{
	if( wgtEditorTabs->height() > 200 && wgtEditorTabs->currentIndex() ){
		QTextEdit *editor = (QTextEdit*) wgtEditorTabs->currentWidget();
		editor->paste();
	}else if( wgtConsole->height() > 200 ){
		wgtConsole->paste();
	}
}
void DaoStudio::slotFind()
{
	QString str = wgtFind->text();
	bool cs = chkCaseSensitive->isChecked();
	QTextDocument::FindFlags opt( cs ? QTextDocument::FindCaseSensitively : 0 );
	if( str == "" ) return;
	if( wgtEditorTabs->height() > 200 ){
		if( wgtEditorTabs->currentIndex() ){
			QTextEdit *editor = (QTextEdit*) wgtEditorTabs->currentWidget();
			if( editor->find( str, opt ) == false ) editor->moveCursor( QTextCursor::Start );
		}else{
			if( docViewer->find( str, opt ) == false )
				docViewer->moveCursor( QTextCursor::Start );
			//			QWebPage::FindFlags opt( cs ? QWebPage::FindCaseSensitively : 0 );
			//			docViewer->findText( str, opt | QWebPage::FindWrapsAroundDocument );
		}
	}else if( wgtConsole->height() > 200 ){
		if( wgtConsole->find( str, opt ) == false )
			wgtConsole->moveCursor( QTextCursor::Start );
	}
}
void DaoStudio::slotReplace()
{
	QString from = wgtFind->text();
	QString to = wgtReplace->text();
	bool all = chkReplaceAll->isChecked();
	bool cs = chkCaseSensitive->isChecked();
	QTextDocument::FindFlags opt( cs ? QTextDocument::FindCaseSensitively : 0 );
	if( from == "" ) return;
	if( wgtEditorTabs->height() > 200 && wgtEditorTabs->currentIndex() ){
		QTextEdit *editor = (QTextEdit*) wgtEditorTabs->currentWidget();
		if( all ) editor->moveCursor( QTextCursor::Start );
		while( editor->find( from, opt ) ){
			editor->textCursor().removeSelectedText();
			editor->insertPlainText( to );
			if( all == false ) break;
		}
	}
}
void DaoStudio::slotRestartMonitor()
{
	QMessageBox mbox( this );
	mbox.setWindowTitle( tr("DaoStudio: Restart Dao Virtual Machine") );
	mbox.setText( tr( "The Dao Virtual Machine is about to restart:") + "\n"
		//      + tr("Choose \"Restore\" to restore data in the workspace;") + "\n"
		+ tr("Choose \"Restart\" to start a new virtual machine;") + "\n"
		+ tr("Choose \"Abort\" to abort this operation.") );
	//  QAbstractButton *bt1 = mbox.addButton( tr("Restore"), QMessageBox::AcceptRole );
	QAbstractButton *bt2 = mbox.addButton( tr("Restart"), QMessageBox::DestructiveRole );
	QAbstractButton *bt3 = mbox.addButton( tr("Abort"), QMessageBox::RejectRole );
	mbox.exec();
	
	QAbstractButton *bt = mbox.clickedButton();
	if( bt == bt3 ) return;
	disconnect( monitor, SIGNAL(finished(int, QProcess::ExitStatus)),
		this, SLOT(slotRestartMonitor(int, QProcess::ExitStatus)));
	monitor->kill();
	monitor->waitForFinished();
	RestartMonitor();
}
void DaoStudio::slotCloseEditor( int id )
{
	if( id < ID_EDITOR ) return;
	DaoEditor *editor = (DaoEditor*) wgtEditorTabs->widget( id );
	if( editor->TextChanged() ){
		int ret = QMessageBox::warning(this, tr("DaoStudio"),
			tr("The script has been modified.\n"
			"Do you want to save your changes?"),
			QMessageBox::Save | QMessageBox::Discard
			| QMessageBox::Cancel, QMessageBox::Save);
		if( ret == QMessageBox::Save ){
			slotSave();
			if( editor->FileName() == "" ) return; // not saved
		}else if( ret == QMessageBox::Cancel ){
			return;
		}
	}
	lastCursor[ editor->FilePath() ] = editor->textCursor().position();
	slotWriteLog( tr("close source file") + " \"" + editor->FilePath() + "\"" );
	wgtEditorTabs->removeTab( id );
	delete editor;
}
void DaoStudio::slotMaxEditor( int )
{
	wgtEditorTabs->currentWidget()->setFocus();
	if( wgtEditorTabs->currentIndex() >= ID_EDITOR ){
		((DaoEditor*)wgtEditorTabs->currentWidget())->ensureCursorVisible();
	}
	if( actionMSplit->isChecked() ) return;
	int h = splitter->height();
	QList<int> sizes;
	sizes<<(h-160)<<80<<80;
	splitter->setSizes( sizes );
}
void DaoStudio::slotMaxMonitor( int )
{
	if( actionMSplit->isChecked() ) return;
	int h = splitter->height();
	QList<int> sizes;
	sizes<<80<<(h-160)<<80;
	splitter->setSizes( sizes );
}
void DaoStudio::slotMaxConsole( int )
{
	wgtConsole->setFocus();
	wgtConsole->ensureCursorVisible();
	if( actionMSplit->isChecked() ) return;
	int h = splitter->height();
	QList<int> sizes;
	sizes<<80<<80<<(h-160);
	splitter->setSizes( sizes );
}
void DaoStudio::slotCompile()
{
	int id = wgtEditorTabs->currentIndex();
	//wgtFrameList->clear();
	if( id < ID_EDITOR ) return;
	DaoEditor *editor = (DaoEditor*) wgtEditorTabs->widget( id );
	QString script = wgtEditorTabs->tabToolTip( id );
	if( script.right( 4 ) != ".dao" ) return;
	if( editor->TextChanged() ) slotSave();
	slotMaxConsole();
	slotWriteLog( tr("compile script file") + " \"" + script + "\"" );
	script.replace( " ", "\\\\ " );
	wgtConsole->InsertScript( "\\dao -c " + script + "\n" );
}
void DaoStudio::slotStart()
{
	if( wgtConsole->debugSocket ){
		wgtConsole->Resume();
		return;
	}
	if( vmState == DAOCON_DEBUG ){
		wgtConsole->Resume();
		return;
	}
	if( actionDebugger->isChecked() ){
		slotDebug();
		return;
	}

	int id = wgtEditorTabs->currentIndex();
	//wgtFrameList->clear();
	if( id < ID_EDITOR ) return;
	DaoEditor *editor = (DaoEditor*) wgtEditorTabs->widget( id );
	QString script = wgtEditorTabs->tabToolTip( id );
	if( script.right( 4 ) != ".dao" ) return;
	if( editor->TextChanged() ) slotSave();
	slotMaxConsole();
	slotWriteLog( tr("load script file") + " \"" + script + "\"" );
	script.replace( " ", "\\\\ " );
	wgtConsole->LoadScript( script );
}
void DaoStudio::slotDebug()
{
	if( wgtConsole->debugSocket ){
		wgtConsole->Resume();
		return;
	}
	if( vmState == DAOCON_DEBUG ){
		wgtConsole->Resume();
		return;
	}
	int id = wgtEditorTabs->currentIndex();
	//wgtFrameList->clear();
	if( id < ID_EDITOR ) return;
	DaoEditor *editor = (DaoEditor*) wgtEditorTabs->widget( id );
	QString script = wgtEditorTabs->tabToolTip( id );
	if( script.right( 4 ) != ".dao" ) return;
	if( editor->TextChanged() ) slotSave();
	for(int i=ID_EDITOR; i<wgtEditorTabs->count(); i++){
		editor = (DaoEditor*) wgtEditorTabs->widget( i );
		editor->SetEditLines( 0, -1 );
	}
	slotMaxConsole();
	script = wgtEditorTabs->tabToolTip( id );
	slotWriteLog( tr("start to debug script file") + " \"" + script + "\"" );
	script.replace( " ", "\\\\ " );
	wgtConsole->LoadScript( script, true );
}
void DaoStudio::slotDebugger( bool checked )
{
	socket.connectToServer( DaoStudioSettings::socket_script );
	if( socket.waitForConnected( 500 ) ){
		socket.putChar( DAO_DEBUGGER_SWITCH );
		socket.putChar( checked ? wgtDebuggerList->currentIndex() + 1 : 0 );
		socket.flush();
		socket.waitForDisconnected();
	}
}
void DaoStudio::slotProfiler( bool checked )
{
	socket.connectToServer( DaoStudioSettings::socket_script );
	if( socket.waitForConnected( 500 ) ){
		socket.putChar( DAO_PROFILER_SWITCH );
		socket.putChar( checked );
		socket.flush();
		socket.waitForDisconnected();
	}
}
void DaoStudio::slotSwitchDebugger(int)
{
	if( actionDebugger->isChecked() == false ) return;
	slotDebugger( true );
}
void DaoStudio::slotSwitchProfiler(int)
{
	if( actionProfiler->isChecked() == false ) return;
	slotProfiler( true );
}
void DaoStudio::slotStop()
{
	wgtConsole->Stop();
}
void DaoStudio::slotMSplit( bool checked )
{
	if( checked ){
		wgtEditorTabs->setMaximumHeight( 5000 );
		wgtConsole->setMaximumHeight( 5000 );
	}
}
void DaoStudio::SetState( int state )
{
	vmState = state;
	btSetPath->setDisabled( true );
	switch( state ){
	case DAOCON_READY :
		actionState->setIcon( QPixmap( ":/images/greenball.png" ) );
		actionStart->setDisabled( false );
		actionDebugger->setDisabled( false );
		btSetPath->setDisabled( false );
		break;
	case DAOCON_RUN :
		slotWriteLog( tr("execution started successfully") );
		actionState->setIcon( QPixmap( ":/images/redball.png" ) );
		actionStart->setDisabled( true );
		actionDebugger->setDisabled( true );
		break;
	case DAOCON_DEBUG :
		slotWriteLog( tr("pause for debugging") );
		actionState->setIcon( QPixmap( ":/images/violaball.png" ) );
		actionStart->setDisabled( false );
		actionDebugger->setDisabled( false );
		break;
	default :
		slotWriteLog( tr("pause for standard input") );
		actionState->setIcon( QPixmap( ":/images/violaball.png" ) );
		actionStart->setDisabled( true );
		break;
	}
	if( state != DAOCON_READY ) return;
	int i, n = wgtEditorTabs->count();
	for(i=ID_EDITOR; i<n; i++){
		DaoEditor *editor = (DaoEditor*) wgtEditorTabs->widget( i );
		editor->SetExePoint( NULL );
		editor->SetEditLines( 0, 1E9 );
	}
}
void DaoStudio::slotLoadURL( const QString & )
{
	//docViewer->load( url );
	slotMaxEditor();
	wgtConsole->ensureCursorVisible();
}
void DaoStudio::slotTips()
{
	QString tips = "daostudio_tips";
	if( locale.indexOf( "zh" ) ==0 ) tips += "_zh_cn";
	docViewer->setSource( QUrl( tips + ".html" ) );
	slotMaxEditor();
	wgtEditorTabs->setCurrentIndex(ID_DOCVIEW);
}
void DaoStudio::slotDocs()
{
	docViewer->setSource( QUrl( "index.html" ) );

	slotMaxEditor();
	wgtEditorTabs->setCurrentIndex(ID_DOCVIEW);
}
void DaoStudio::slotHelpVIM()
{
	DaoHelpVIM help( this );
	help.resize( 800, 600 );
	help.exec();
}
void DaoStudio::slotAbout()
{
	DaoStudioAbout about( this );
	about.resize( 200, 200 );
	about.exec();
}

void DaoStudio::LoadSettings()
{
	int i, id, size;
	QSettings settings( "daovm.net", "DaoStudio" );
	settings.sync();

	if( configured ) return;
	configured = 1;
	
	QStringList histSearch = settings.value( "History/Search" ).toStringList();
	QStringList histCommand = settings.value( "History/Command" ).toStringList();
	DaoCommandEdit::searchHistory = histSearch;
	DaoCommandEdit::commandHistory = histCommand;
	
	lastCursor = settings.value( "Editor/Cursors" ).toHash();
	
	QStringList paths = settings.value( "Path/List" ).toStringList();
	if( paths.size() ){
		for(i=0; i<paths.size(); i++){
			QString path = paths[i];
			if( pathUsage.contains( path ) ){
				pathUsage[path] += 1;
			}else{
				pathUsage[path] = 1;
			}
			id = wgtPathList->findText( path );
			if( id <0 ) wgtPathList->addItem( path );
		}
	}
	
	QString pathw = settings.value( "Path/Working" ).toString();
	QString pathb = settings.value( "Path/Browsing" ).toString();
	QString filter = settings.value( "File/Filters" ).toString();
	if( pathw.size() ) SetPathWorking( pathw );
	if( pathb.size() ) SetPathBrowsing( pathb );
	id = wgtFileSuffix->findText( filter );
	if( id >=0 ) wgtFileSuffix->setCurrentIndex( id );
	
	int modeEditor = settings.value( "Editor/EditMode" ).toInt();
	int modeConsole = settings.value( "Console/EditMode" ).toInt();
	wgtEditorMode->setCurrentIndex( modeEditor );
	wgtConsoleMode->setCurrentIndex( modeConsole );
	
	if( settings.contains( "Editor/ColorScheme" ) ){
		int csEditor = settings.value( "Editor/ColorScheme" ).toInt();
		wgtEditorColor->setCurrentIndex( csEditor );
	}

	if( settings.contains( "Console/ColorScheme" ) ){
		int csConsole = settings.value( "Console/ColorScheme" ).toInt();
		wgtConsoleColor->setCurrentIndex( csConsole );
	}

	int tabVis = settings.value( "Editor/TabVisibility" ).toInt();
	wgtTabVisibility->setCurrentIndex( tabVis );
	
	if( settings.contains( "Font/Family" ) ){
		QString family = settings.value( "Font/Family" ).toString();
		id = wgtFontFamily->findText( family );
		if( id >=0 ) wgtFontFamily->setCurrentIndex( id );
	}

	size = settings.value( "Font/Size" ).toInt();
	if( size >= 10 ) wgtFontSize->setCurrentIndex( size - 10 );

	int debugger = settings.value( "Tools/Debugger" ).toInt();
	int profiler = settings.value( "Tools/Profiler" ).toInt();
	wgtDebuggerList->setCurrentIndex( debugger );
	wgtProfilerList->setCurrentIndex( profiler );
}
void DaoStudio::SaveSettings()
{
	QSettings settings( "daovm.net", "DaoStudio" );
	int modeEditor = wgtEditorMode->currentIndex();
	int modeConsole = wgtConsoleMode->currentIndex();
	int csEditor = wgtEditorColor->currentIndex();
	int csConsole = wgtConsoleColor->currentIndex();
	int debugger = wgtDebuggerList->currentIndex();
	int profiler = wgtProfilerList->currentIndex();
	settings.setValue( "Path/Working", pathWorking );
	settings.setValue( "Path/Browsing", pathBrowsing );
	settings.setValue( "File/Filters", wgtFileSuffix->currentText() );
	settings.setValue( "Editor/EditMode", modeEditor );
	settings.setValue( "Console/EditMode", modeConsole );
	settings.setValue( "Editor/ColorScheme", csEditor );
	settings.setValue( "Console/ColorScheme", csConsole );
	settings.setValue( "Editor/TabVisibility", wgtTabVisibility->currentIndex() );
	settings.setValue( "Font/Family", wgtFontFamily->currentText() );
	settings.setValue( "Font/Size", wgtFontSize->currentIndex() + 10 );
	settings.setValue( "Tools/Debugger", debugger );
	settings.setValue( "Tools/Profiler", profiler );
	
	QStringList histSearch = DaoCommandEdit::searchHistory;
	QStringList histCommand = DaoCommandEdit::commandHistory;
	while( histSearch.size() > 100 ) histSearch.pop_front();
	while( histCommand.size() > 100 ) histCommand.pop_front();
	settings.setValue( "History/Search", DaoCommandEdit::searchHistory );
	settings.setValue( "History/Command", DaoCommandEdit::commandHistory );
	
	QStringList paths( pathUsage.keys() );
	settings.setValue( "Path/List", paths );
	
	int i, n = wgtEditorTabs->count();
	for(i=ID_EDITOR; i<n; i++){
		DaoEditor *editor = (DaoEditor*) wgtEditorTabs->widget( i );
		lastCursor[ editor->FilePath() ] = editor->textCursor().position();
	}
	settings.setValue( "Editor/Cursors", lastCursor );
	settings.sync();
}

