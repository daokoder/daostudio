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
#include<QtNetwork>

#include<daoConsole.h>
#include<daoEditor.h>
#include<daoStudio.h>
#include<daoStudioMain.h>

QStringList DaoConsole::oldComdHist;
bool DaoConsole::appendToFile = 0;

const char* const dao_color_names[8]
= { "black", "red", "green", "yellow", "blue", "magenta", "cyan", "white" };

const QColor dao_colors[8] = 
{
	Qt::black ,
	Qt::red ,
	Qt::green ,
	Qt::yellow ,
	Qt::blue ,
	Qt::magenta ,
	Qt::cyan ,
	Qt::white
};

DaoConsole::DaoConsole( QWidget *parent ) : DaoTextEdit( parent, & wordList )
{
	rgxDaoFile.setPattern( "^\\s*\\\\\\s*\\w+\\.dao\\b" );
	rgxDaoFile.setCaseSensitivity( Qt::CaseInsensitive );
	rgxPrintRes.setPattern( "^\\s*=" );
	rgxShellCmd.setPattern( "^\\s*\\\\" );
	rgxHttpURL.setPattern( "^\\s*http(s?)://\\w+\\.\\w+" );

	ResetDaoColors(0);

	debugSocket = NULL;
	stdinSocket = NULL;
	stdoutSocket = NULL;
	loggerSocket = NULL;
	executeSocket = NULL;

	editor = NULL;
	shellTop = false;
	shell = false;

	setWordWrapMode( QTextOption::WrapAnywhere );

	lexer = DaoLexer_New();
	outputBound = cursorBound = 0;
	state = 0;
	count = 0;
	stdinCount = 0;
	prompt = "(dao)";
	QString copy_notice = DaoVmSpace_GetCopyNotice();
	copy_notice.replace( "\n  ", "\n#  " );
	insertPlainText( "#" + copy_notice );
	insertPlainText("\n");
	PrintPrompt();
	UpdateCursor( false );

	QAction *fold = new QAction( "Fold/Unfold Output", this );
	extraMenuActions.append( fold );
	connect(fold, SIGNAL(triggered()),this,SLOT(slotFoldUnfoldOutput()));

	QAction *all = new QAction( "Clear All Output", this );
	extraMenuActions.append( all );
	connect(all, SIGNAL(triggered()),this,SLOT(slotClearAll()));

	QAction *part = new QAction( "Clear This Output", this );
	extraMenuActions.append( part );
	connect(part, SIGNAL(triggered()),this,SLOT(slotClearPart()));

	QAction *save = new QAction( "Save All Output", this );
	extraMenuActions.append( save );
	connect(save, SIGNAL(triggered()),this,SLOT(slotSaveAll()));

	connect( this, SIGNAL( cursorPositionChanged() ), this, SLOT( slotBoundCursor() ) );
	//connect( &scriptSocket, SIGNAL( disconnected() ), this, SLOT( slotScriptFinished2() ) );

	connect( &shellProcess, SIGNAL(finished(int,QProcess::ExitStatus)),
			this, SLOT(slotProcessFinished(int,QProcess::ExitStatus)));
	LoadCmdHistory();

	SetVmSpace( DaoInit(NULL) ); //XXX

	if( QFile::exists( DaoStudioSettings::socket_debug ) )
		QFile::remove( DaoStudioSettings::socket_debug );
	debugServer.listen( DaoStudioSettings::socket_debug );
	connect( & debugServer, SIGNAL(newConnection()), this, SLOT(slotSocketDebug()));

	if( QFile::exists( DaoStudioSettings::socket_stdin ) )
		QFile::remove( DaoStudioSettings::socket_stdin );
	stdinServer.listen( DaoStudioSettings::socket_stdin );
	connect( & stdinServer, SIGNAL(newConnection()), this, SLOT(slotSocketStdin()));

	if( QFile::exists( DaoStudioSettings::socket_stdout ) )
		QFile::remove( DaoStudioSettings::socket_stdout );
	stdoutServer.listen( DaoStudioSettings::socket_stdout );
	connect( & stdoutServer, SIGNAL(newConnection()), this, SLOT(slotSocketStdout()));

	if( QFile::exists( DaoStudioSettings::socket_stderr ) )
		QFile::remove( DaoStudioSettings::socket_stderr );
	stderrServer.listen( DaoStudioSettings::socket_stderr );
	connect( & stderrServer, SIGNAL(newConnection()), this, SLOT(slotSocketStderr()));

	if( QFile::exists( DaoStudioSettings::socket_execution ) )
		QFile::remove( DaoStudioSettings::socket_execution );
	executionServer.listen( DaoStudioSettings::socket_execution );
	connect( & executionServer, SIGNAL(newConnection()), this, SLOT(slotSocketExecution()));

	if( QFile::exists( DaoStudioSettings::socket_logger ) )
		QFile::remove( DaoStudioSettings::socket_logger );
	loggerServer.listen( DaoStudioSettings::socket_logger );
	connect( & loggerServer, SIGNAL(newConnection()), this, SLOT(slotSocketLogger()));
}
DaoConsole::~DaoConsole()
{
	SaveCmdHistory();
	DaoLexer_Delete( lexer );
}
void DaoConsole::SetVmSpace( DaoVmSpace *vms )
{
	vms->options |= DAO_OPTION_IDE | DAO_OPTION_INTERUN;
	vms->mainNamespace->options |= DAO_OPTION_IDE | DAO_NS_AUTO_GLOBAL;
	vmSpace = vms;
}
void DaoConsole::LoadCmdHistory()
{
	if( oldComdHist.size()==0 ){
		QString home = QDir::home().path();
		QFile hist( home + "/.dao/daostudio/command.history" );
		if( hist.open( QFile::ReadOnly )) {
			QTextStream fin( &hist );
			QString all = fin.readAll();
			oldComdHist = all.split( "(dao)", QString::SkipEmptyParts );
		}
	}
	for(int i=0; i<oldComdHist.size(); i++){
		QString s = oldComdHist[i].trimmed();
		if( s.size() > 0 ) commandHistory<<oldComdHist[i].trimmed();
	}

	idCommand = commandHistory.size();
}
void DaoConsole::SaveCmdHistory()
{
	QString home = QDir::home().path();
	QString fname = home + "/.dao/daostudio/command.history";
	if( ! QFile::exists( fname ) ) QDir().mkpath( home + "/.dao/daostudio/" );
	QFile hist( fname );
	if( appendToFile ){
		if( hist.open( QFile::Append )) {
			QTextStream sout( &hist );
			int i = commandHistory.size() -1000;
			if( i<0 ) i = 0;
			for (i = 0; i < commandHistory.size(); ++i){
				QString cmd = commandHistory.at(i);
				if( i >0 && cmd == commandHistory.at(i-1) ) continue;
				sout<<"(dao) "<<cmd<<"\n";
			}
		}
	}else{
		if( hist.open( QFile::WriteOnly | QFile::Truncate )) {
			QTextStream sout( &hist );
			int i = commandHistory.size() -1000;
			if( i<0 ) i = 0;
			for (i = 0; i < commandHistory.size(); ++i){
				QString cmd = commandHistory.at(i);
				if( i >0 && cmd == commandHistory.at(i-1) ) continue;
				sout<<"(dao) "<<cmd<<"\n";
			}
		}
		appendToFile = 1;
	}
}
void DaoConsole::AppendCmdHistory( const QString & cmd )
{
	int n = commandHistory.size();
	QString c = cmd.trimmed();
	idCommand = n;
	if( n && c == commandHistory[ n-1 ] ) return;
	commandHistory<<c;
	idCommand = n+1;
}
void DaoConsole::ClearCommand()
{
	QTextCursor cursor = textCursor();
	cursor.setPosition( cursorBound );
	setTextCursor( cursor );
	moveCursor( QTextCursor::End, QTextCursor::KeepAnchor );
	cursor = textCursor();
	cursor.removeSelectedText();
}
void DaoConsole::ResetDaoColors( int scheme )
{
	int i, j;
	QColor fcolor = Qt::black;
	QColor bcolor = Qt::white;
	QTextCharFormat charFormat;

	switch( scheme ){
	case 0 :
		bcolor = Qt::white;
		break;
	case 1 :
		bcolor = QColor( "#DDDDDD" );
		break;
	case 2 :
		fcolor = Qt::white;
		bcolor = QColor( "#222222" );
		break;
	default :
		fcolor = Qt::white;
		bcolor = QColor( "#000000" );
		break;
	}

	charFormat.setForeground( fcolor );
	charFormat.setBackground( bcolor );
	charFormats[ ":" ] = charFormat;

	for(i=0; i<8; ++i){
		QString name1 = dao_color_names[i];
		QString name2 = ":";
		name1 += ":";
		name2 += dao_color_names[i];
		charFormat.setForeground( dao_colors[i] );
		charFormat.setBackground( bcolor );
		charFormats[ name1 ] = charFormat;
		charFormat.setForeground( fcolor );
		charFormat.setBackground( dao_colors[i] );
		charFormats[ name2 ] = charFormat;
		charFormat.setForeground( dao_colors[i] );
		for(j=0; j<8; ++j){
			QString name = dao_color_names[i];
			name += ":";
			name += dao_color_names[j];
			charFormat.setBackground( dao_colors[j] );
			charFormats[ name ] = charFormat;
		}
	}
}
void DaoConsole::SetColorScheme( int scheme )
{
	DaoTextEdit::SetColorScheme( scheme );
	ResetDaoColors( scheme );
	QString src = toPlainText().mid( cursorBound );
	ClearCommand();
	insertPlainText( src );
}
void DaoConsole::focusInEvent ( QFocusEvent * event )
{
	//emit signalFocusIn();
	DaoTextEdit::focusInEvent( event );
}
void DaoConsole::mousePressEvent ( QMouseEvent * event )
{
	QPlainTextEdit::mousePressEvent( event );
	emit signalFocusIn();
}
void DaoConsole::AdjustCursor( QTextCursor *cursor, QTextCursor::MoveMode m )
{
	if( cursor->position() < cursorBound ){
		cursor->setPosition( cursorBound, m );
	}
	if( cursor->position() < outputBound ){
		cursor->setPosition( outputBound, m );
	}
}
void DaoConsole::keyPressEvent ( QKeyEvent * event )
{
	QScrollBar *bar = verticalScrollBar();
	QTextCursor cursor = textCursor();
	Qt::KeyboardModifiers mdf = event->modifiers();
	int key = event->key();
	int st = studio->GetState();
	if( st == DAOCON_RUN ){
#ifdef MAC_OSX
		if( key == Qt::Key_C && ( mdf & Qt::MetaModifier ) ) Stop();
#else
		if( key == Qt::Key_C && ( mdf & Qt::ControlModifier ) ) Stop();
#endif
		return;
	}
	if( st != DAOCON_READY && st != DAOCON_STDIN ) return;
	if( cursor.position() < cursorBound && ! ( mdf & Qt::ControlModifier ) ){
		cursor.setPosition( cursorBound );
		setTextCursor( cursor );
	}
	setReadOnly(false);
	bool vis = cmdPrompt->isVisible();
	bool vim = EditModeVIM( event );
	if( vis or (vim and key != Qt::Key_Enter and key != Qt::Key_Return) ) return;
	if( mdf & Qt::AltModifier ){
		if( key == Qt::Key_Up || key == Qt::Key_Down ){
			studio->slotMaxEditor();
			return;
		}
	}
	if( state ==0 && idCommand >= commandHistory.size() )
		script = toPlainText().mid( cursorBound );
	if( state == DAOCON_STDIN ){
		if( key == Qt::Key_C && ( mdf & Qt::ControlModifier ) ){
			QTextCursor cursor = textCursor();
			cursor.setPosition( outputBound );
			setTextCursor( cursor );
			moveCursor( QTextCursor::End, QTextCursor::KeepAnchor );
			cursor = textCursor();
			QString txt = cursor.selectedText();
			txt.replace( QChar( 0x2029 ), '\n' );
			if( stdinSocket ){
				stdinSocket->write( txt.toUtf8() );
				/* Maybe necessary on Windows: */
				stdinSocket->waitForBytesWritten( 1000 );
				stdinSocket->disconnectFromServer();
				/* Necessary on Windows: */
				stdinSocket->waitForDisconnected(1000);
				stdinSocket = NULL;
			}
			return;
		}
	}else if( state ){
		if( key == Qt::Key_C && ( mdf & Qt::ControlModifier ) ) Stop();
		return;
	}
	switch( key ){
	case Qt::Key_Enter :
	case Qt::Key_Return :
		if( codehl.toktype >= DTOK_CMT_OPEN && codehl.toktype <= DTOK_WCS_OPEN ){
			DaoTextEdit::keyPressEvent( event );
		}else if( state == DAOCON_STDIN ){
			//QMessageBox::about( this, "", QString::number( stdinCount ) + " " + QString::number((size_t)stdinSocket) );
			QTextCursor cursor = textCursor();
			cursor.setPosition( outputBound );
			setTextCursor( cursor );
			moveCursor( QTextCursor::End, QTextCursor::KeepAnchor );
			cursor = textCursor();
			QString txt = cursor.selectedText();
			txt.replace( QChar( 0x2029 ), '\n' );
			QByteArray data = txt.toUtf8();
			moveCursor( QTextCursor::End );
			if( data.size() >= stdinCount && stdinSocket ){
				stdinSocket->write( data );
				/* Maybe necessary on Windows: */
				stdinSocket->waitForBytesWritten( 1000 );
				stdinSocket->disconnectFromServer();
				/* Necessary on Windows: */
				stdinSocket->waitForDisconnected(1000);
				stdinSocket = NULL;
			}
			DaoTextEdit::keyPressEvent( event );
		}else{
			QString src = toPlainText().mid( cursorBound );
			QString raw = src;
			short bcount, cbcount, sbcount;
			src = src.trimmed();
			if( src[0] == '=' ){
				int len = src.size() - 1 - ( src[src.size()-1] == ';' );
				src = "io.writeln( " + src.mid( 1, len ) + ")";
			}else if( src[0] == '\\' && rgxDaoFile.indexIn( src ) >=0 ){
				src = "std.load( \"" + src.mid( 1 ) + "\", 0, 1 )";
			}else if( src[0] == '\\' ){
				shellTop = (src.indexOf( QRegExp( "\\\\top(\\s|$)" ) ) ==0);
				shell = true;
				//src = "{ var p = io.popen( '" + src.mid(1) + "', 'r' ); ";
				//src += "while( not p.eof() ){ io.write( p.read() ); } p.close(); }";
#if 0
				AppendCmdHistory( raw.trimmed() );
				slotStateChanged( DAOCON_SHELL );
				script = "";
				append( "" );
				codehl.SetState(0);
				outputBound = textCursor().position();
				shellProcess.start( src.mid(1), QIODevice::ReadWrite );
				shellProcess.waitForFinished();
				slotPrintOutput( shellProcess.readAllStandardOutput() );
				slotPrintOutput( shellProcess.readAllStandardError() );
				slotStateChanged( DAOCON_READY );
				PrintPrompt();
				return;
#else
#endif
			}else if( rgxHttpURL.indexIn( src ) >=0 ){
				AppendCmdHistory( raw.trimmed() );
				emit signalLoadURL( raw.trimmed() );
				script = "";
				PrintPrompt();
				break;
			}
			DaoLexer_Tokenize( lexer, src.toLocal8Bit().data(), DAO_LEX_COMMENT|DAO_LEX_SPACE );
			bcount = sbcount = cbcount = 0;
			for(size_t i=0; i<lexer->tokens->size; i++){
				DaoToken *tk = lexer->tokens->items.pToken[i];
				switch( tk->type ){
				case DTOK_LB : bcount --; break;
				case DTOK_RB : bcount ++; break;
				case DTOK_LCB : cbcount --; break;
				case DTOK_RCB : cbcount ++; break;
				case DTOK_LSB : sbcount --; break;
				case DTOK_RSB : sbcount ++; break;
				default : break;
				}
				if( tk->type == DTOK_IDENTIFIER ) wordList.AddWord( tk->string.chars );
			}
			if( bcount <0 || sbcount <0 || cbcount <0 ){
				if( not vim ) DaoTextEdit::keyPressEvent( event );
			}else{
				appendPlainText( "" );
				moveCursor( QTextCursor::End );
				UpdateCursor( true );
				if( src.trimmed().size() ){
					AppendCmdHistory( raw.trimmed() );
					codehl.SetState(0);
					script = "";
					cursorBound = outputBound = textCursor().position();
					RunScript( src );
				}else{
					PrintPrompt();
				}
			}
		}
		break;
	case Qt::Key_Up :
		if( cursor.block().position() > cursorBound || state != DAOCON_READY ){
			DaoTextEdit::keyPressEvent( event );
			cursor = textCursor();
			if( cursor.position() < cursorBound ){
				cursor.setPosition( cursorBound );
				setTextCursor( cursor );
			}
			return;
		}
		if( idCommand==0 ) return;
		idCommand --;
		ClearCommand();
		insertPlainText( commandHistory.at( idCommand ) );
		bar->setSliderPosition( bar->maximum() );
		break;
	case Qt::Key_Down :
		if( cursor.block().next().length() ){
			DaoTextEdit::keyPressEvent( event );
			return;
		}
		ClearCommand();
		if( idCommand+1 >= commandHistory.size() ){
			idCommand = commandHistory.size();
			insertPlainText( script );
			event->accept();
			return;
		}
		idCommand ++;
		insertPlainText( commandHistory.at( idCommand ) );
		moveCursor( QTextCursor::End );
		bar->setSliderPosition( bar->maximum() );
		break;
	case Qt::Key_Left :
	case Qt::Key_Clear :
	case Qt::Key_Backspace :
		if( cursor.position() <= cursorBound ) break;
		DaoTextEdit::keyPressEvent( event );
		break;
	case Qt::Key_C :
		if( mdf & Qt::ControlModifier ) ClearCommand();
		DaoTextEdit::keyPressEvent( event );
		break;
	case Qt::Key_X :
		//if( mdf & Qt::ControlModifier ) break;
	default : DaoTextEdit::keyPressEvent( event );
	}
	event->accept();
	if( wgtWords && not labPopup->isVisible() )
		wgtWords->TryCompleteWord( event, 0 );
}
void DaoConsole::slotFoldUnfoldOutput()
{
	QTextCursor cursor = cursorForPosition( menuPosition );
	QTextBlock block = cursor.block();
	int i, bnum = block.blockNumber();
	for(i=0; i<promptBlocks.size(); i++){
		if( promptBlocks[i] > bnum ) break;
	}
	if( i == 0 or i >= promptBlocks.size() ) return;
	int first = promptBlocks[i-1];
	int last = promptBlocks[i];
	if( last < first + 10 ) return;
	block = document()->findBlockByNumber( first + 5 );
	int count0 = 0;
	int count1 = 0;
	while( block.blockNumber() + 5 < last ){
		bool state = block.isVisible();
		count0 += state == false;
		count1 += state == true;
		block.setVisible( not state );
		block = block.next();
	}
	bool hide = count0 < count1;
	if( hide ){
		while( block.blockNumber() > first ) block = block.previous();
		HighlightFullLine( QTextCursor( block ) );
	}else{
		QList<QTextEdit::ExtraSelection> selections = extraSelections();
		QTextEdit::ExtraSelection selection;
		for(i=0; i<selections.size(); i++){
			selection = selections[i];
			int k = selection.cursor.block().blockNumber();
			if( k == first ) selections.removeAt(i);
		}
		setExtraSelections(selections);
	}
	viewport()->resize( viewport()->sizeHint() );
	repaint();
}
int DaoConsole::ShowClearActionMessage( bool extra )
{
	QString one, ems;
	if( extra ){
		one = tr("The output from this run is about to be erased:") + "\n";
		ems = tr("The first and last 5 lines of output might be remained.") + "\n";
	}else{
		one = tr("The output of all run is about to be erased:") + "\n";
	}
	QMessageBox mbox( this );
	mbox.setWindowTitle( tr("DaoStudio: Clear Console Output") );
	mbox.setText( one + "\n" + ems
			+ "  " + tr("Choose \"Save\" to save these output to file;") + "\n"
			+ "  " + tr("Choose \"No Save\" to erase without saving;") + "\n"
			+ "  " + tr("Choose \"Abort\" to abort this operation.") );
	QAbstractButton *bt1 = mbox.addButton( tr("Save"), QMessageBox::AcceptRole );
	QAbstractButton *bt2 = mbox.addButton( tr("No Save"), QMessageBox::DestructiveRole );
	QAbstractButton *bt3 = mbox.addButton( tr("Abort"), QMessageBox::RejectRole );
	mbox.exec();

	QAbstractButton *bt = mbox.clickedButton();
	if( bt == bt1 ) return 2;
	if( bt == bt2 ) return 1;
	return 0;
}
void DaoConsole::slotSaveAll()
{
	QTextBlock block;
	QString name = QFileDialog::getSaveFileName( this, tr("Set file name to save"),
			studio->GetWorkingPath(), "Text File (*.txt)" );
	if( name == "" ){
		QMessageBox::about( this, "DaoStudio", tr("Output will not be saved") );
	}else{
		QFile file( name );
		if( file.open( QFile::WriteOnly ) ) {
			QTextStream fout( & file );
			block = document()->firstBlock();
			block = block.next();
			while( block.isValid() ){
				fout<<block.text()<<"\n";
				block = block.next();
			}
		}else{
			QMessageBox::about( this, "DaoStudio", 
					tr("Failed to open file") + "\n" + tr("Output will not be saved") );
		}
	}
}
void DaoConsole::slotClearAll()
{
	QTextBlock block;
	int rc = ShowClearActionMessage();
	if( rc ==0 ) return;
	if( rc ==2 ) slotSaveAll();
	block = document()->firstBlock();
	block = block.next();
	while( block.isValid() and block.text().indexOf( "(dao)" ) <0 ) block = block.next();
	QTextCursor cursor( block );
	cursor.movePosition( QTextCursor::End, QTextCursor::KeepAnchor );
	cursor.removeSelectedText();
	PrintPrompt();
}
void DaoConsole::slotClearPart()
{
	QTextCursor cursor = cursorForPosition( menuPosition );
	QTextBlock block = cursor.block();
	int i, bnum = block.blockNumber();
	for(i=0; i<promptBlocks.size(); i++){
		if( promptBlocks[i] > bnum ) break;
	}
	if( i == 0 or i >= promptBlocks.size() ) return;
	int first = promptBlocks[i-1];
	int last = promptBlocks[i];

	int rc = ShowClearActionMessage( true );
	if( rc ==0 ) return;
	if( rc ==2 ){
		QString name = QFileDialog::getSaveFileName( this, tr("Set file name to save"),
				studio->GetWorkingPath(), "Text File (*.txt)" );
		if( name == "" ){
			QMessageBox::about( this, "DaoStudio", tr("Output will not be saved") );
		}else{
			QFile file( name );
			if( file.open( QFile::WriteOnly ) ) {
				QTextStream fout( & file );
				block = document()->findBlockByNumber( first );
				block = block.next();
				while( block.isValid() and block.blockNumber() < last ){
					fout<<block.text()<<"\n";
					block = block.next();
				}
			}else{
				QMessageBox::about( this, "DaoStudio", 
						tr("Failed to open file") + "\n" + tr("Output will not be saved") );
			}
		}
	}
	if( last < first + 5 ) return;
	int nn = last - first - 11;
	int mm = 0;
	int cpos = textCursor().position();
	block = document()->findBlockByNumber( last-5 );
	block.setUserData( new DaoCodeLineData(false, CLS_OUTPUT, block.blockNumber()+1) );
	block = document()->findBlockByNumber( first+5 );
	block.setUserData( new DaoCodeLineData(false, CLS_OUTPUT, block.blockNumber()+1) );
	QTextCursor c1( block );
	block = document()->findBlockByNumber( last-11 );
	QTextCursor c2( block );
	c1.movePosition( QTextCursor::EndOfBlock );
	c2.movePosition( QTextCursor::EndOfBlock );
	c1.setPosition( c2.position(), QTextCursor::KeepAnchor );
	mm = c1.selectionEnd() - c1.selectionStart();
	c1.removeSelectedText();
	for(i=0; i<promptBlocks.size(); i++){
		if( promptBlocks[i] >= last ) promptBlocks[i] -= nn;
	}
	setUndoRedoEnabled( false );
	setUndoRedoEnabled( true );
	cursorBound -= mm;
	outputBound -= mm;
	c1.setPosition( cpos - mm );
	setTextCursor( c1 );
	codehl.SetSkip( cursorBound );
	codehl.SetState( DAO_HLSTATE_NORMAL );
}
void DaoConsole::slotBoundCursor()
{
	QTextCursor cursor = textCursor();
	setReadOnly( cursor.position() < cursorBound );
	if( cursor.hasSelection() && cursor.selectionStart() < cursorBound ) setReadOnly(1);
}
void DaoConsole::RunScript( const QString & src, bool debug )
{
	setUndoRedoEnabled( false );
	setUndoRedoEnabled( true );
	stdinSocket = NULL;
	scriptSocket.connectToServer( DaoStudioSettings::socket_script );
	if( scriptSocket.waitForConnected( 1000 ) ==0 ){
		insertPlainText( tr( "ERROR: DaoMonitor is not running!" ) );
		PrintPrompt();
		return;
	}
	state = DAOCON_RUN;
	studio->SetState( DAOCON_RUN );
	//printf( "socket: %i  %s\n", socket.isValid(), src.toUtf8().data() );
	debug = false;
	scriptSocket.putChar( debug ? DAO_DEBUG_SCRIPT : DAO_RUN_SCRIPT );
	scriptSocket.write( src.toUtf8().data() );
	scriptSocket.flush();
	/* Maybe necessary on Windows: */
	scriptSocket.waitForBytesWritten( 1000 );
	scriptSocket.disconnectFromServer();
	studio->ResetTimer();
	clearScreenBound = textCursor().position();
}
void DaoConsole::InsertScript( const QString & code )
{
	AppendCmdHistory( code.trimmed() );
	ClearCommand();
	insertPlainText( code );
	codehl.SetState(0);
	RunScript( code );
}
void DaoConsole::LoadScript( const QString & file, bool debug )
{
	QString code = "std.load( \"" + file + "\", 0, 1 )\n";
	AppendCmdHistory( code.trimmed() );
	ClearCommand();
	insertPlainText( code );
	codehl.SetState(0);
	RunScript( code, debug );
}
void DaoConsole::PrintPrompt()
{
	const QString & txt = toPlainText();
	moveCursor( QTextCursor::End );
	if( txt[ txt.size()-1 ] != '\n' ) insertPlainText( "\n" );
	codehl.SetState(DAO_HLSTATE_PROMPT);
	codehl.language = NULL;
	insertPlainText( "(dao)" );
	promptBlocks.append( textCursor().block().blockNumber() );
	codehl.SetState( DAO_HLSTATE_NORMAL );
	cursorBound = textCursor().position();
	codehl.SetSkip( cursorBound + 1 );
	insertPlainText( " " );
	cursorBound = outputBound = textCursor().position();
	setUndoRedoEnabled( false );
	setUndoRedoEnabled( true );
	codehl.SetSkip( cursorBound );
	codehl.SetState( DAO_HLSTATE_NORMAL );
	insertPlainText( script );
	stdoutSocket = NULL;

	QScrollBar *bar = verticalScrollBar();
	bar->setSliderPosition( bar->maximum() );
	bar = horizontalScrollBar();
	bar->setSliderPosition( 0 );
}
void DaoConsole::slotPrintOutput( const QString & output )
{
	QScrollBar *bar = verticalScrollBar();
	int i, k, sp = bar->sliderPosition();
	bool scroll = sp+1 >= bar->maximum();
	//printf( "%i %s\n\n\n", output.size(), output.toLocal8Bit().data() );
	//printf( "======= %i %i\n\n\n", shellTop, output.indexOf( "Processes" ) );
	if( shellTop ){
		QTextCursor cursor = textCursor();
		cursor.setPosition( outputBound );
		if( output.indexOf( "Processes" ) ==0 ){
			cursor.setPosition( clearScreenBound );
		}
		cursor.movePosition( QTextCursor::Right, QTextCursor::KeepAnchor, output.size() );
		cursor.insertText( output );
		setTextCursor( cursor );
	}else{
		QStringList parts = output.split( '\1' );
		moveCursor( QTextCursor::End );
#if 1
		for(i=0; i<parts.size(); i++){
			QString part = parts[i];
			k = part.indexOf( '\2' );
			if( k >=0 && k < 16 ){
				QString name = part.left( k );
				if( name.indexOf( ':' ) >= 0 && charFormats.contains( name ) ){
					codehl.language = DaoBasicSyntax::console;
					setCurrentCharFormat( charFormats[ name ] );
				}else if( codehl.languages.contains( name ) ){
					codehl.language = codehl.languages[ name ];
				}else{
					codehl.language = NULL;
					if( name.size() ) insertPlainText( '\1' + name + '\2' );
				}
				insertPlainText( part.mid( k + 1 ) );
			}else{
				if( i ) part = '\1' + part;
				insertPlainText( part );
			}
		}
#else
		insertPlainText( output );
#endif
	}
	setUndoRedoEnabled( false );
	setUndoRedoEnabled( true );
	cursorBound = outputBound = textCursor().position();
	if( shellTop ){
		bar->setSliderPosition( sp );
		return;
	}
	bar->setSliderPosition( scroll ? bar->maximum() : sp );
}
void DaoConsole::slotReadStdOut()
{
	//QMessageBox::about( this, "test", "test" );

#if 1
	QByteArray output = monitor->readAllStandardOutput();
	QString text = QString::fromUtf8( output.data(), output.size() );
	//printf( "error=%i\n", monitor->error() );
	slotPrintOutput( text );
#elif 0
	QByteArray output;
	while( not monitor->atEnd() ) output += monitor->readLine();
	QString text = QString::fromUtf8( output.data(), output.size() );
	slotPrintOutput( text );
#elif 0
	monitor->setReadChannel( QProcess::StandardOutput );
	while( not monitor->atEnd() ){
		monitor->setReadChannel( QProcess::StandardOutput );
		QByteArray output = monitor->readLine();
		QString text = QString::fromUtf8( output.data(), output.size() );
		slotPrintOutput( text );
		monitor->setReadChannel( QProcess::StandardError );
		output = monitor->readLine();
		//output = monitor->readAllStandardError();
		text = QString::fromUtf8( output.data(), output.size() );
		slotPrintOutput( text );
	}
#else
	//monitor->setReadChannel( QProcess::StandardOutput );
	while( not monitor->atEnd() ){
		QByteArray output = monitor->readLine();
		QString text = QString::fromUtf8( output.data(), output.size() );
		slotPrintOutput( text );
	}
	//monitor->setReadChannel( QProcess::StandardError );
	while( not monitor->atEnd() ){
		QByteArray output = monitor->readLine();
		QString text = QString::fromUtf8( output.data(), output.size() );
		slotPrintOutput( text );
	}
#endif
}
void DaoConsole::slotReadStdError()
{
	//printf( "hello\n" );
	//QMessageBox::about( this, "test", "test" );

#if 0
	QByteArray output = monitor->readAllStandardError();
	QString text = QString::fromUtf8( output.data(), output.size() );
	slotPrintOutput( text );
#elif 0
	QByteArray output;
	while( not monitor->atEnd() ) output += monitor->readLine();
	QString text = QString::fromUtf8( output.data(), output.size() );
	slotPrintOutput( text );
#else
	//monitor->setReadChannel( QProcess::StandardError );
	while( not monitor->atEnd() ){
		QByteArray output = monitor->readLine();
		QString text = QString::fromUtf8( output.data(), output.size() );
		slotPrintOutput( text );
		//output = monitor->readAllStandardError();
		//text = QString::fromUtf8( output.data(), output.size() );
		//slotPrintOutput( text );
	}
#endif
}
void DaoConsole::slotScriptFinished()
{
	executeSocket = NULL;
	slotStdoutFromSocket();

	// Do not call the following, otherwise the stdout cannot be read after a timeout!
	//monitor->waitForReadyRead( TIME_YIELD + 20 );

	// Wait sometime for the stdout of "monitor", to make sure
	// the command prompt is displayed after the outputs.
	QTime dieTime= QTime::currentTime().addMSecs(100);
	while( QTime::currentTime() < dieTime )
		QCoreApplication::processEvents( QEventLoop::AllEvents, 100 );

	while( monitor->bytesAvailable() ) slotReadStdOut();
	//slotReadStdError();
	shellTop = false;
	PrintPrompt();
	studio->SetState( DAOCON_READY );
	state = DAOCON_READY;
	shell = false;
}
void DaoConsole::slotSocketStdin()
{
	state = DAOCON_STDIN;
	studio->SetState( DAOCON_STDIN );
	stdinSocket = stdinServer.nextPendingConnection();
	stdinSocket->waitForReadyRead();
	stdinCount = stdinSocket->readAll().toInt();
}
void DaoConsole::slotSocketStdout()
{
	//slotPrintOutput( "connect socket\n" );
	stdoutSocket = stdoutServer.nextPendingConnection();
	connect( stdoutSocket, SIGNAL(readyRead()), this, SLOT(slotStdoutFromSocket()) );
	//connect( stdoutSocket, SIGNAL(disconnected()), this, SLOT(slotScriptFinished()) );
	//printf( "connect stdout socket %p\n", stdoutSocket );
}
void DaoConsole::slotSocketStderr()
{
	return;
	//slotPrintOutput( "connect socket\n" );
	stdoutSocket = stderrServer.nextPendingConnection();
	connect( stdoutSocket, SIGNAL(readyRead()), this, SLOT(slotStdoutFromSocket()) );
	//printf( "connect stderr socket %p\n", stdoutSocket );
}
void DaoConsole::slotSocketExecution()
{
	executeSocket = executionServer.nextPendingConnection();
	connect( executeSocket, SIGNAL(disconnected()), this, SLOT(slotScriptFinished()) );
}
void DaoConsole::slotStdoutFromSocket()
{
	//slotPrintOutput( "read socket\n" );
	//printf( "socket %p\n", stdoutSocket );
	if( stdoutSocket == NULL ) return;
	QByteArray output = stdoutSocket->readAll();
	QString text = QString::fromUtf8( output.data(), output.size() );
	//printf( "socket %i %i\n", output.size(), text.size() );
	slotPrintOutput( text );
}
void DaoConsole::slotSocketLogger()
{
	//slotPrintOutput( "connect socket\n" );
	//printf( "connect socket\n" );
	loggerSocket = loggerServer.nextPendingConnection();
	connect( loggerSocket, SIGNAL(readyRead()), this, SLOT(slotLogFromSocket()) );
}
void DaoConsole::slotLogFromSocket()
{
	//slotPrintOutput( "read socket\n" );
	//printf( "socket\n" );
	if( loggerSocket == NULL ) return;
	//printf( "socket %i %i\n", output.size(), text.size() );
	QByteArray output = loggerSocket->readAll();
	QList<QByteArray> data = output.split( '\1' );
	if( data.size() != 2 ){ 
		studio->slotWriteLog( QString::fromUtf8( output.data(), output.size() ) );
		return;
	}
	QString time = QString::fromUtf8( data[1].data(), data[1].size() );
	int status = data[0].toInt();
	QString log;
	if( shell and status == QProcess::CrashExit ){
		log = tr("shell execution crashed unexpectly");
		slotPrintOutput( log );
	}else if( shell or status ){
		log += tr("execution terminated normally");
	}else{
		log += tr("execution failed");
	}
	studio->slotWriteLog( log + " (" + time + ")" );
}
void DaoConsole::slotSocketDebug()
{
	//QString info = tr("Entering debug mode, please click the debug button to resume");
	//slotPrintOutput( info + " ...\n" );
	//emit signalStateChanged( DAOCON_DEBUG, 0 );

	editor = NULL;
	state = DAOCON_DEBUG;
	studio->SetState( DAOCON_DEBUG );
	debugSocket = debugServer.nextPendingConnection();

	QByteArray sdata;
	QTime dieTime= QTime::currentTime().addMSecs(1000);
	while( debugSocket->isValid() && sdata.count( '\0' ) < 3 ){
		if( QTime::currentTime() > dieTime ) break;
		debugSocket->waitForReadyRead();
		sdata += debugSocket->readAll();
	}
	QList<QByteArray> data = sdata.split( '\0' );
	if( data.size() < 4 ) return;

	QString name = data[0];
	int start = data[1].toInt();
	int end = data[2].toInt();
	int entry = data[3].toInt();
	int i, n = tabWidget->count();
	for(i=ID_EDITOR; i<n; i++){
		if( tabWidget->tabToolTip(i) == name ){
			editor = (DaoEditor*) tabWidget->widget( i );
			break;
		}
	}
	if( editor == NULL ) return;
	editor->SetEditLines( start, end );
	editor->SetExePoint( entry );
}
void DaoConsole::Resume()
{
	studio->SetState( DAOCON_RUN );
	if( debugSocket == NULL ) return;
	if( editor ){
		bool recompile = editor->EditContinue2();
		QByteArray data = QByteArray::number( editor->newEntryLine );
		if( recompile ){
			QByteArray size1 = QByteArray::number( editor->lineMap.size() );
			QByteArray size2 = QByteArray::number( editor->newCodes.size() );
			QByteArray size3 = QByteArray::number( editor->routCodes.size() );
			QByteArray line, newcodes, oldcodes;
			int i, n = editor->lineMap.size();
			for(i=0; i<n; i++){
				line.append( QByteArray::number( editor->lineMap[i] ) );
				line.append( '\n' );
			}
			newcodes.append( editor->newCodes.join( "\n" ) );
			oldcodes.append( editor->routCodes.join( "\n" ) );
			data += '\n' + size1 + '\n' + size2 + '\n' + size3;
			data += '\n' + line + newcodes + '\n' + oldcodes;
		}
		debugSocket->write( data );
		debugSocket->flush();
		/* Maybe necessary on Windows: */
		debugSocket->waitForBytesWritten( 1000 );
	}
	debugSocket->disconnectFromServer();
	debugSocket = NULL;
}
void DaoConsole::Stop()
{
	if( state == DAOCON_READY ) return;
	if( stdinSocket ) stdinSocket->disconnectFromServer();
	if( stdoutSocket ) stdoutSocket->disconnectFromServer();
	if( debugSocket ) debugSocket->disconnectFromServer();
	stdinSocket = NULL;
	stdoutSocket = NULL;
	debugSocket = NULL;
	studio->slotWriteLog( tr("execution cancelled") );
}
void DaoConsole::Quit()
{
	if( executeSocket ){
		disconnect( executeSocket, SIGNAL( disconnected() ), this, SLOT( slotScriptFinished() ) );
		executeSocket = NULL;
	}
	if( scriptSocket.isValid() ) scriptSocket.disconnectFromServer();
	Stop();
}
void DaoConsole::slotProcessFinished( int code, QProcess::ExitStatus status )
{
}
