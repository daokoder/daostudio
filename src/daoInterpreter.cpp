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

#include<daoConsole.h>
#include<daoDebugger.h>
#include<daoInterpreter.h>

extern "C"{
extern void DaoDebugger_Debug( DaoDebugger *self, DaoProcess *proc, DaoStream *stream );
}
typedef void (*DaoVmDebugger_Debug)( DaoVmDebugger *self, DaoProcess *process, DaoStream *stream );

static int DaoEditContinueData( QByteArray &data, QList<int> & lineMap, QStringList & newCodes, QStringList & routCodes )
{
	QList<QByteArray> lines = data.split( '\n' );
	if( lines.size() <= 2 ) return lines[0].toInt();

	//printf( "lines: %i\n", lines.size() );

	int entry = lines[0].toInt();
	int size1 = lines[1].toInt();
	int size2 = lines[2].toInt();
	int size3 = lines[3].toInt();
	int i;

	lineMap.clear();
	newCodes.clear();
	routCodes.clear();
	for(i=0; i<size1; i++) lineMap.append( lines[i+4].toInt() );
	for(i=0; i<size2; i++) newCodes.append( lines[i+4+size1] );
	for(i=0; i<size3; i++) routCodes.append( lines[i+4+size1+size2] );
	return entry;
}
static void DaoResetExecution( DaoProcess *process, int line, int offset=0 )
{
	DaoRoutine *routine = process->activeRoutine;
	DaoVmCodeX **annotCodes = routine->body->annotCodes->items.pVmc;
	int i = 0, n = routine->body->annotCodes->size;
	while( i < n && annotCodes[i]->line < line ) ++i;
	while( i < n && offset >0 && annotCodes[i]->line == line){
		++i;
		--offset;
	}
	process->status = DAO_PROCESS_STACKED;
	process->topFrame->entry = i;
	//DaoVmCodeX_Print( *annotCodes[i], NULL );
	//printf( "entry: %s\n", annotCodes[i]->annot->chars->data );
}
static void DaoStdioRead( DaoConsoleStream *self, DString *buf, int count )
{
	fflush( stdout );
	self->interpreter->mutex.lock();
	self->interpreter->waiting = true;
	self->socket2.flush();
	self->socket.connectToServer( DaoStudioSettings::socket_stdin );
	self->socket.waitForConnected( 1000 );
	QObject::connect( & self->socket, SIGNAL(disconnected()),
			self->interpreter, SLOT(slotExitWaiting()) );
	self->socket.write( QByteArray::number( count ) );
	self->socket.flush();
	self->socket.waitForBytesWritten( 1000 );
	QByteArray data;
	while( self->interpreter->waiting && self->interpreter->vmSpace->stopit == 0 ){
		self->socket.waitForReadyRead( 100 );
		data += self->socket.readAll();
		QCoreApplication::processEvents( QEventLoop::AllEvents, 1000 );
		fflush( stdout );
	}
	if( self->interpreter->vmSpace->stopit ){
		self->interpreter->mutex.unlock();
		return;
	}
	if( self->socket.state() == QLocalSocket::ConnectedState ){
		self->socket.waitForReadyRead( 1000 );
		data += self->socket.readAll();
	}
	//QString s = QString::fromUtf8( data.data(), data.size() );
	DString_SetBytes( buf, data.data(), data.size() );
	self->interpreter->mutex.unlock();
}
static void DaoStdioWrite( DaoConsoleStream *self, DString *buf )
{
#if 0
	FILE *fout = fopen( "output.txt", "a+" );
	time_t t = time(NULL);
	fprintf( fout, "%s: ", ctime( & t ) );
	fprintf( fout, "%s\n", DString_GetData( buf ) );
	fclose( fout );
#endif
	DaoFile_WriteString( stdout, buf );
	fflush( stdout );
	fflush( stderr );
	return;
	self->interpreter->mutex.lock();
	self->socket2.write( DString_GetData( buf ), DString_Size( buf ) );
	self->socket2.flush();
	self->interpreter->mutex.unlock();
}
static void DaoSudio_SetColor( DaoConsoleStream *self, const char *fgcolor, const char *bgcolor )
{
	char buf[32];
	int n = sprintf( buf, "\1%s:%s\2", fgcolor?fgcolor:"", bgcolor?bgcolor:"" );
	printf( "%s", buf );
	fflush( stdout );
	fflush( stderr );
	return;
	self->interpreter->mutex.lock();
	self->socket2.write( buf, n );
	self->socket2.flush();
	self->interpreter->mutex.unlock();
}
static void DaoConsDebug( DaoVmDebugger *self, DaoProcess *process, DaoStream *stream )
{
	self->interpreter->mutex.lock();

	DaoVmCode *codes = process->activeRoutine->body->vmCodes->data.codes;
	DaoVmCodeX **annots = process->activeRoutine->body->annotCodes->items.pVmc;
	int oldline = annots[ process->activeCode - codes + 1]->line;
	QFileInfo fi( process->activeRoutine->nameSpace->name->chars );
	QString name = fi.absoluteFilePath();
	QByteArray start = QByteArray::number( process->activeRoutine->body->codeStart+1 );
	QByteArray end = QByteArray::number( process->activeRoutine->body->codeEnd-1 );
	QByteArray next = QByteArray::number( oldline );
	QByteArray send;
	send.append( name );
	send += '\0' + start + '\0' + end + '\0' + next;

	QString info = QObject::tr("Entering debug mode, please click the debug button to resume");
	info += " ...";
	printf( "%s\n", info.toUtf8().data() );
	fflush( stdout );
	self->socket.connectToServer( DaoStudioSettings::socket_debug );
	self->socket.waitForConnected( 1000 );
	QObject::connect( & self->socket, SIGNAL(disconnected()),
			self->interpreter, SLOT(slotExitWaiting()) );
	self->socket.write( send );
	self->socket.flush();
	/* Maybe necessary on Windows: */
	self->socket.waitForBytesWritten( 1000 );
	QByteArray data;
	self->interpreter->waiting = true;
	self->interpreter->debugProcess = process;
	self->interpreter->InitDataBrowser();
	while( self->interpreter->waiting && self->interpreter->vmSpace->stopit == 0 ){
		self->socket.waitForReadyRead( 100 );
		data += self->socket.readAll();
		QCoreApplication::processEvents( QEventLoop::AllEvents, 100 );
		fflush( stdout );
	}
	//while( self->socket.state() != QLocalSocket::UnconnectedState ){
	if( self->socket.state() == QLocalSocket::ConnectedState ){
		self->socket.waitForReadyRead( 1000 );
		data += self->socket.readAll();
	}
	//self->socket.waitForReadyRead();
	//data += self->socket.readAll();
	//}
	//printf( "all:\n%s\n", QString(data).toUtf8().data() );

	DaoxDebugger *debugger = self->debugger;
	QList<int> lineMap;
	QStringList newCodes, routCodes;
	DaoRoutine *old = process->activeRoutine;
	int entry = DaoEditContinueData( data, lineMap, newCodes, routCodes );
	//printf( "all:\n%s\n", QString(data).toUtf8().data() );
	//printf( "new:\n%s\n", newCodes.join("\n").toUtf8().data() );
	//printf( "old:\n%s\n", routCodes.join("\n").toUtf8().data() );
	if( debugger->EditContinue( process, entry, lineMap, newCodes, routCodes ) ){
		if( old == process->activeRoutine && entry != oldline )
			DaoResetExecution( process, entry );
	}
	//printf( "%i %s\n", data.size(), data.data() );
	//QMessageBox::about( self->interpreter, "test", "test" );

	self->interpreter->mutex.unlock();
}
static void DaoSetBreaks( DaoVmDebugger *self, DaoRoutine *routine )
{
	self->debugger->SetBreakPoints( routine );
}
static void DaoProcessEvents( DaoVmUserHandler *self, DaoProcess *process )
{
	if( self->timer.time > self->time ){
		//printf( "time: %i  %i\n", self->timer.time, self->time );
		//fflush( stdout );
		self->time = self->timer.time;
		QApplication::processEvents( QEventLoop::AllEvents, TIME_EVENT );
		fflush( stdout );
	}
}


DaoInterpreter::DaoInterpreter( const char *cmd ) : QObject()
{
	int i;
	DString *base = DString_New();
	DString *mod = DString_New();

	locale = DaoStudioSettings::locale;
	program = DaoStudioSettings::program;
	programPath = DaoStudioSettings::program_path;
	daoBinPath = programPath;
#ifdef MAC_OSX
	DString_SetChars( base, programPath.toLocal8Bit().data() );
	DString_SetChars( mod, "../Frameworks/bin/" );
	Dao_MakePath( base, mod );
	daoBinPath = QString::fromUtf8( mod->chars, mod->size );
#elif defined(WIN32)
	DString_SetChars( base, programPath.toLocal8Bit().data() );
	DString_SetChars( mod, "./bin/" );
	Dao_MakePath( base, mod );
	daoBinPath = QString::fromUtf8( mod->chars, mod->size );
#endif

	vmState = DAOCON_READY;
	debugProcess = NULL;
	daoString = DString_New();
	tokens = DArray_New( DAO_DATA_TOKEN );

	dataSocket = NULL;
	scriptSocket = NULL;

	handler.time = 0;
	handler.InvokeHost = DaoProcessEvents;
	handler.timer.start();

	stdioStream.stdRead = DaoStdioRead;
	stdioStream.stdWrite = DaoStdioWrite;
	stdioStream.stdFlush = NULL;
	stdioStream.SetColor = DaoSudio_SetColor;
	stdioStream.interpreter = this;
	errorStream.stdRead = DaoStdioRead;
	errorStream.stdWrite = DaoStdioWrite;
	errorStream.stdFlush = NULL;
	errorStream.SetColor = DaoSudio_SetColor;
	errorStream.interpreter = this;
	guiDebugger.debugger = & this->debugger;
	guiDebugger.interpreter = this;
	guiDebugger.debug = DaoConsDebug;
	guiDebugger.breaks = DaoSetBreaks;
	cmdDebugger.debugger = & this->debugger;
	cmdDebugger.interpreter = this;
	cmdDebugger.debug = (DaoVmDebugger_Debug) DaoDebugger_Debug;
	cmdDebugger.breaks = DaoSetBreaks;

	vmSpace = DaoInit( cmd );
	vmSpace->options |= DAO_OPTION_IDE | DAO_OPTION_INTERUN;
	vmSpace->mainNamespace->options |= DAO_OPTION_IDE | DAO_OPTION_INTERUN;
	nameSpace = vmSpace->mainNamespace;
	guiDebugger.process = DaoVmSpace_MainProcess( vmSpace );
	cmdDebugger.process = DaoVmSpace_MainProcess( vmSpace );
	handler.process = DaoVmSpace_MainProcess( vmSpace );
	profiler = DaoxProfiler_New();
	DaoVmSpace_SetUserStdio( vmSpace, (DaoUserStream*) & stdioStream );
	//DaoVmSpace_SetUserStdError( vmSpace, (DaoUserStream*) & errorStream );
	DaoVmSpace_SetUserStdError( vmSpace, (DaoUserStream*) & stdioStream );
	//DaoVmSpace_SetUserDebugger( vmSpace, (DaoDebugger*) & guiDebugger );
	DaoVmSpace_SetUserHandler( vmSpace, (DaoUserHandler*) & handler );

	DaoNamespace *ns = vmSpace->mainNamespace;
	DString_SetChars( ns->name, "interactive codes" );
	codeType = DaoParser_ParseTypeName( CODE_ITEM_TYPE, ns, NULL );
	valueType = DaoParser_ParseTypeName( VALUE_ITEM_TYPE, ns, NULL );
	extraType = DaoParser_ParseTypeName( EXTRA_ITEM_TYPE, ns, NULL );
	messageType = DaoParser_ParseTypeName( VALUE_INFO_TYPE, ns, NULL );
	codeTuple = DaoTuple_Create( codeType, 0, 1 );
	valueTuple = DaoTuple_Create( valueType, 0, 1 );
	extraTuple = DaoTuple_Create( extraType, 0, 1 );
	messageTuple = DaoTuple_Create( messageType, 0, 1 );
	numArray = DaoArray_New( DAO_INTEGER );
	extraList = DaoList_New();
	constList = DaoList_New();
	varList = DaoList_New();
	codeList = DaoList_New();
	valueStack = DaoList_New();
	extraStack = DArray_New(0);

	DaoValue_MarkConst( (DaoValue*)codeTuple );
	DaoValue_MarkConst( (DaoValue*)valueTuple );
	DaoValue_MarkConst( (DaoValue*)extraTuple );
	DaoTuple_SetItem( messageTuple, (DaoValue*)numArray, INDEX_NUMBERS );
	DaoTuple_SetItem( messageTuple, (DaoValue*)extraList, INDEX_EXTRAS );
	DaoTuple_SetItem( messageTuple, (DaoValue*)constList, INDEX_CONSTS );
	DaoTuple_SetItem( messageTuple, (DaoValue*)varList, INDEX_VARS );
	DaoTuple_SetItem( messageTuple, (DaoValue*)codeList, INDEX_CODES );

	SetPathWorking( "." );
#ifdef MAC_OSX
	DString_SetChars( base, programPath.toLocal8Bit().data() );
	DString_SetChars( mod, "../Frameworks/lib/dao/modules/" );
	Dao_MakePath( base, mod );
	DaoVmSpace_AddPath( vmSpace, mod->chars );
#else
	DaoVmSpace_AddPath( vmSpace, programPath.toLocal8Bit().data() );
#endif

	if( QFile::exists( DaoStudioSettings::socket_data ) )
		QFile::remove( DaoStudioSettings::socket_data );
	dataServer.listen( DaoStudioSettings::socket_data );
	connect( &dataServer, SIGNAL(newConnection()), this, SLOT(slotServeData()));

	if( QFile::exists( DaoStudioSettings::socket_script ) )
		QFile::remove( DaoStudioSettings::socket_script );
	scriptServer.listen( DaoStudioSettings::socket_script );
	connect( &scriptServer, SIGNAL(newConnection()), this, SLOT(slotStartExecution()));

	time = 0;
	//connect( &timer, SIGNAL(timeout()), this, SLOT(slotFlushStdout()));
	//timer.start( 100 );

	QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
	env.insert( "daostudio", "yes" );

	shell = new QProcess();
	shell->setProcessEnvironment(env);
	shell->setProcessChannelMode(QProcess::MergedChannels);
	connect( shell, SIGNAL(readyReadStandardOutput()), this, SLOT(slotReadStdOut()));
	connect( shell, SIGNAL(finished(int, QProcess::ExitStatus)),
			this, SLOT(slotShellFinished(int, QProcess::ExitStatus)));

	DString_Delete( base );
	DString_Delete( mod );

	monitorSocket.connectToServer( DaoStudioSettings::socket_monitor );
	if( monitorSocket.waitForConnected( 1000 ) ==0 ){
		printf( "cannot connect to the virtual machine monitor\n" );
		fflush( stdout );
		return;
	}
}

DaoInterpreter::~DaoInterpreter()
{
	monitorSocket.disconnectFromServer();
	DaoxProfiler_Delete( profiler );
	shell->close();
}

void DaoInterpreter::SetPathWorking( const QString & path )
{
	QDir dir( path );
	pathWorking = dir.absolutePath() + "/";
	DaoVmSpace_SetPath( vmSpace, pathWorking.toLocal8Bit().data() );
}
void DaoInterpreter::slotReadStdOut()
{
	while( not shell->atEnd() ){
		QByteArray output = shell->readLine();
		DString_Reset( daoString, 0 );
		DString_AppendBytes( daoString, output.data(), output.size() );
		DaoFile_WriteString( stdout, daoString );
		fflush( stdout );
#if 0
		stdioStream.socket2.write( output.data(), output.size() );
		stdioStream.socket2.flush();
#endif
	}
}
void DaoInterpreter::slotShellFinished(int, QProcess::ExitStatus)
{
	vmSpace->stopit = 1;
}
void DaoInterpreter::slotServeData()
{
	dataSocket = dataServer.nextPendingConnection();
	dataSocket->waitForReadyRead(1000);
	QByteArray data = dataSocket->readAll();
	if( data.size() ==0 ){
		dataSocket->disconnectFromServer();
		return;
	}
	if( vmSpace == NULL ) return;
	DaoValue *value = NULL;
	DaoNamespace *nspace = vmSpace->mainNamespace;
	DaoProcess *process = vmSpace->mainProcess;
	DString_SetBytes( daoString, data.data(), data.size() );
	if( DaoValue_Deserialize( & value, daoString, nspace, process ) == 0 ) return;

	//printf( "__LINE__ %i: %i %i\n", __LINE__, DaoList_Size( valueStack ), extraStack->size );
	DaoTuple *request = (DaoTuple*) value;
	if( request->values[1]->xInteger.value == DATA_STACK ){
		int erase = request->values[2]->xInteger.value;
		DArray_Erase( valueStack->value, 0, erase );
		DArray_Erase( extraStack, 0, erase );
		value = valueStack->value->items.pValue[0];
		if( extraStack->items.pValue[0] ){
			DaoStackFrame *frame = (DaoStackFrame*)extraStack->items.pValue[0];
			DaoList_PopFront( valueStack );
			DArray_PopFront( extraStack );
			ViewStackFrame( frame, (DaoProcess*)value );
			SendDataInformation();
		}else{
			DaoList_PopFront( valueStack );
			DArray_PopFront( extraStack );
			ViewValue( value );
		}
		return;
	}

	value = valueStack->value->items.pValue[0];
	switch( value->type ){
	case DAO_PROCESS :
		if( extraStack->items.pValue[0] ){
			ViewStackData( (DaoProcess*)value, (DaoStackFrame*)extraStack->items.pValue[0], request );
		}else{
			ViewProcessStack( (DaoProcess*)value, request );
		}
		break;
	case DAO_ROUTINE : 
		if( value->xRoutine.overloads ){
			ViewRoutinesData( (DaoRoutine*)value, request );
		}else{
			ViewRoutineData( (DaoRoutine*)value, request );
		}
		break;
	case DAO_TUPLE : ViewTupleData( (DaoTuple*)value, request ); break;
	case DAO_LIST : ViewListData( (DaoList*)value, request ); break;
	case DAO_MAP : ViewMapData( (DaoMap*)value, request ); break;
	case DAO_CLASS : ViewClassData( (DaoClass*)value, request ); break;
	case DAO_OBJECT : ViewObjectData( (DaoObject*)value, request ); break;
	case DAO_NAMESPACE : ViewNamespaceData( (DaoNamespace*)value, request ); break;
	default : printf( "not viewable data!" ); break;
	}
}
void DaoInterpreter::slotStartExecution()
{
	if( vmState == DAOCON_RUN ){
		printf( "is running\n" );
		fflush( stdout );
		return;
	}

	scriptSocket = scriptServer.nextPendingConnection();
	scriptSocket->waitForReadyRead(1000);
	QByteArray script = scriptSocket->readAll();
	if( script.size() ==0 ){
		scriptSocket->disconnectFromServer();
		return;
	}
	char info = script[0];
	script.remove(0,1);
	if( info == DAO_SET_PATH ){
		//DaoVmSpace_SetPath( vmSpace, script.data() );
		QDir::setCurrent( QString::fromUtf8( script.data(), script.size() ) );
		scriptSocket->disconnectFromServer();
		return;
	}else if( info == DAO_DEBUGGER_SWITCH ){
		char id = script[0];
		switch( id ){
		case 0 :
			DaoVmSpace_SetUserDebugger( vmSpace, NULL );
			break;
		case 1 :
			DaoVmSpace_SetUserDebugger( vmSpace, (DaoDebugger*) & cmdDebugger );
			break;
		case 2 :
			DaoVmSpace_SetUserDebugger( vmSpace, (DaoDebugger*) & guiDebugger );
			break;
		}
		scriptSocket->disconnectFromServer();
		return;
	}else if( info == DAO_PROFILER_SWITCH ){
		char checked = script[0];
		DaoVmSpace_SetUserProfiler( vmSpace, checked ? (DaoProfiler*) profiler : NULL );
		scriptSocket->disconnectFromServer();
		return;
	}
	QLocalSocket exeSocket;
	exeSocket.connectToServer( DaoStudioSettings::socket_execution );
	stdioStream.socket2.connectToServer( DaoStudioSettings::socket_stdout );
	if( stdioStream.socket2.waitForConnected( 1000 ) ==0 ){
		printf( "cannot connect to console stdout\n" );
		fflush( stdout );
		scriptSocket->disconnectFromServer();
		return;
	}
	if( not monitorSocket.isValid() ){
		monitorSocket.connectToServer( DaoStudioSettings::socket_monitor );
		if( monitorSocket.waitForConnected( 1000 ) ==0 ){
			printf( "cannot connect to the virtual machine monitor\n" );
			fflush( stdout );
			return;
		}
	}
#if 0
	errorStream.socket2.connectToServer( DaoStudioSettings::socket_stderr );
	if( errorStream.socket2.waitForConnected( 1000 ) ==0 ){
		printf( "cannot connect to console stderr\n" );
		fflush( stderr );
		scriptSocket->disconnectFromServer();
		return;
	}
#endif


	if( vmSpace->debugger )
		vmSpace->options |= DAO_OPTION_DEBUG;
	else
		vmSpace->options &= ~DAO_OPTION_DEBUG;

	vmState = DAOCON_RUN;
	DaoProcess *vmp = DaoVmSpace_MainProcess( vmSpace );
	DaoNamespace *ns = DaoVmSpace_MainNamespace( vmSpace );
	ns->options |= DAO_OPTION_IDE | DAO_NS_AUTO_GLOBAL;

	connect( & stdioStream.socket2, SIGNAL(disconnected()), this, SLOT(slotStopExecution()) );
	connect( & stdioStream.socket2, SIGNAL(disconnected()), this, SLOT(slotExitWaiting()) );

	vmSpace->stopit = 0;
	guiDebugger.process = vmp;
	cmdDebugger.process = vmp;
	QTime time;
	time.start();
	int res = 0;
	if( script[0] == '\\' ){
		QString command = script.mid(1);
		bool daobin = command.indexOf( "dao " ) == 0;
		daobin |= command.indexOf( "daomake " ) == 0;
		daobin |= command.indexOf( "daotest " ) == 0;
		daobin |= command.indexOf( "clangdao " ) == 0;
		daobin |= command.indexOf( "dao\t" ) == 0;
		daobin |= command.indexOf( "daomake\t" ) == 0;
		daobin |= command.indexOf( "daotest\t" ) == 0;
		daobin |= command.indexOf( "clangdao\t" ) == 0;
		if( daobin ) command = daoBinPath + command;
		shell->start( command, QIODevice::ReadWrite | QIODevice::Unbuffered );
		/* BUG
		   while( not shell->waitForFinished( 100 ) ){
		   QApplication::processEvents( QEventLoop::AllEvents, 100 );
		   }
		   program run in QProcess will crash!
		 */
		while(1){// not shell->waitForFinished( 100 ) )
			//usleep( 100 );
			//waitForReadyRead
			//shell->waitForReadyRead( 100 );
			//DaoProcessEvents( & guiDebugger );
			//QApplication::processEvents( QEventLoop::WaitForMoreEvents, 100 );
			//Sleeper::Sleep( 10000 );
			QApplication::processEvents( QEventLoop::AllEvents, 20 );
			if( vmSpace->stopit ){
				shell->kill();
				break;
			}
		}
		slotReadStdOut();
		res = shell->exitStatus();
		//printf( "stop: %i, status: %i, code: %i, error: %i\n", 
		//	vmSpace->stopit, res, shell->exitCode(), shell->error() );
	}else{
		DString *pathWorking = vmSpace->pathWorking;
		QString oldPath = QString::fromUtf8( pathWorking->chars, pathWorking->size );
		DString *mbs = DString_New();
		DString_AppendBytes( mbs, script.data(), script.size() );
		res = (int) DaoProcess_Eval( vmp, ns, mbs->chars );
		DaoCallServer_Join();
		DString_Delete( mbs );

		QString newPath = QString::fromUtf8( pathWorking->chars, pathWorking->size );
		if( newPath != oldPath ){
			QLocalSocket socket;
			socket.connectToServer( DaoStudioSettings::socket_path );
			socket.waitForConnected( 1000 );
			socket.write( newPath.toUtf8() );
			socket.flush();
			/* Maybe necessary on Windows: */
			socket.waitForBytesWritten( 1000 );
			socket.disconnectFromServer();
		}

		DaoProfiler *profiler = vmSpace->profiler;
		if( profiler ){
			if( profiler->Report ) profiler->Report( profiler, vmSpace->stdioStream );
			if( profiler->Reset ) profiler->Reset( profiler );
		}
	}
	fflush( stdout );
	stdioStream.socket2.flush();
	stdioStream.socket2.disconnectFromServer();
	exeSocket.disconnectFromServer();
#if 0
	errorStream.socket2.flush();
	errorStream.socket2.disconnectFromServer();
#endif

	QLocalSocket socket;
	socket.connectToServer( DaoStudioSettings::socket_logger );
	socket.waitForConnected( 1000 );
	int ms = time.elapsed();
	int sec = ms / 1000;
	int min = sec / 60;
	int hr = min / 60;
	char buf[100];
	ms -= sec * 1000;
	sec -= min * 60;
	min -= hr * 60;
	sprintf( buf, "%s: %02i:%02i:%02i.%03i", 
			tr("execution time").toUtf8().data(), hr, min, sec, ms );
	QString status = QString::number( res ) + '\1';
	status += QString::fromUtf8( buf, strlen( buf ) );
	socket.write( status.toUtf8() );
	socket.flush();
	/* Maybe necessary on Windows: */
	socket.waitForBytesWritten( 1000 );
	socket.disconnectFromServer();
	//XXX ReduceValueItems( wgtDataList->item( wgtDataList->count()-1 ) );
	EraseDebuggingProcess();
	ViewNamespace( vmSpace->mainNamespace );
	messageTuple->values[INDEX_INIT]->xInteger.value = 1;
	SendDataInformation();
	//ViewValue( dataWidget, (DaoValueItem*) wgtDataList->item( 0 ) );
	//connect( &scriptServer, SIGNAL(newConnection()), this, SLOT(slotStartExecution()));
	vmState = DAOCON_READY;

#if 0
	FILE *fout = fopen( "debug.txt", "a+" );
	fprintf( fout, "%s\n", buf );
	fclose( fout );
#endif
}
void DaoInterpreter::slotStopExecution()
{
	//printf( "DaoInterpreter::slotStopExecution()\n" );
	//QMessageBox::about( this, "","" );
	vmSpace->stopit = 1;
}
void DaoInterpreter::slotExitWaiting()
{
	waiting = false;
}
void DaoInterpreter::slotFlushStdout()
{
	time += 1;
	fflush( stdout );
}
void DaoInterpreter::SendDataInformation()
{
	if( not monitorSocket.isValid() ){
		printf( "cannot connect to the virtual machine monitor\n" );
		fflush( stdout );
		return;
	}
	if( vmSpace == NULL ) return;
	DaoNamespace *nspace = vmSpace->mainNamespace;
	DaoProcess *process = vmSpace->mainProcess;
	if( DaoValue_Serialize( (DaoValue*) messageTuple, daoString, nspace, process )){
		//printf( "%s\n\n", daoString->chars ); fflush(stdout);
		monitorSocket.write( daoString->chars );
		monitorSocket.putChar( '\0' );
		monitorSocket.flush();
	}
}
void DaoInterpreter::InitDataBrowser()
{
	EraseDebuggingProcess();
	ViewNamespace( vmSpace->mainNamespace );
	messageTuple->values[INDEX_INIT]->xInteger.value = 1;
	SendDataInformation();
	ViewProcess( debugProcess );
	messageTuple->values[INDEX_INIT]->xInteger.value = 0;
	SendDataInformation();
}
void DaoInterpreter::EraseDebuggingProcess()
{
	DaoList_Clear( valueStack );
	DArray_Clear( extraStack );
}

void DaoInterpreter::InitMessage( DaoValue *value )
{
	DaoList_PushFront( valueStack, value );
	DArray_PushFront( extraStack, NULL );
	messageTuple->values[INDEX_INIT]->xInteger.value = 0;
	messageTuple->values[INDEX_ADDR]->xInteger.value = (daoint) value;
	messageTuple->values[INDEX_TYPE]->xInteger.value = value->type;
	messageTuple->values[INDEX_SUBTYPE]->xInteger.value = value->xBase.subtype;
	DaoArray_ResizeVector( numArray, 0 );
	DaoTuple_SetItem( messageTuple, (DaoValue*)numArray, INDEX_NUMBERS );
	DaoList_Clear( extraList );
	DaoList_Clear( constList );
	DaoList_Clear( varList );
	DaoList_Clear( codeList );
	DString_Clear( messageTuple->values[INDEX_VALUE]->xString.value );
}
void DaoInterpreter::ViewValue( DaoValue *value )
{
	int ok = true;
	if( value == NULL ) return;

	DaoType *type = DaoNamespace_GetType( vmSpace->mainNamespace, value );
	QString info = "# address:\n" + StringAddress( value ) + "\n# type:\n";
	QString itemName = type ? type->name->chars : "Value";
	if( itemName == "?" ) itemName = "Value";
	itemName += "[" + StringAddress( value ) + "]";
	itemName[0] = itemName[0].toUpper();
	info += type ? type->name->chars : "?";
	if( value->type == DAO_STRING )
		info += "\n# length:\n" + QString::number( DString_Size( value->xString.value ) );

	switch( value->type ){
	case DAO_NONE :
	case DAO_INTEGER :
	case DAO_FLOAT :
	case DAO_DOUBLE :
	case DAO_COMPLEX :
	case DAO_STRING :
		InitMessage( value );
		DaoValue_GetString( value, daoString );
		DString_SetChars( messageTuple->values[INDEX_NAME]->xString.value, itemName.toUtf8().data() );
		DString_SetChars( messageTuple->values[INDEX_INFO]->xString.value, info.toUtf8().data() );
		DString_SetChars( messageTuple->values[INDEX_VALUE]->xString.value, DString_GetData( daoString ) );
		break;
	case DAO_ROUTINE :
		if( value->xRoutine.overloads ){
			ViewRoutines( (DaoRoutine*)value );
		}else{
			ViewRoutine( (DaoRoutine*)value );
		}
		break;
	case DAO_ARRAY : ViewArray( (DaoArray*)value ); break;
	case DAO_TUPLE : ViewTuple( (DaoTuple*)value ); break;
	case DAO_LIST : ViewList( (DaoList*)value ); break;
	case DAO_MAP : ViewMap( (DaoMap*)value ); break;
	case DAO_CLASS : ViewClass( (DaoClass*)value ); break;
	case DAO_OBJECT : ViewObject( (DaoObject*)value ); break;
	case DAO_NAMESPACE : ViewNamespace( (DaoNamespace*)value ); break;
	case DAO_PROCESS : ViewProcess( (DaoProcess*)value ); break;
	default : ok = false; break;
	}
	if( ok ){
		messageTuple->values[INDEX_INIT]->xInteger.value = 0;
		SendDataInformation();
	}
}
void DaoInterpreter::ViewArray( DaoArray *array )
{
	DaoType *type = DaoNamespace_GetType( nameSpace, (DaoValue*)array );
	QString itemName = "Array[" + StringAddress(  array ) + "]";
	QString info = "# address:\n" + StringAddress( array ) + "\n# type:\n";
	daoint i;

	info += type ? type->name->chars : "?";
	info += "\n# elements:\n" + QString::number( array->size );
	info += "\n# shape:\n[ ";
	for(i=0; i<array->ndim; i++){
		if( i ) info += ", ";
		info += QString::number( array->dims[i] );
	}
	info += " ]";

	InitMessage( (DaoValue*)array );
	DString_SetChars( messageTuple->values[INDEX_NAME]->xString.value, itemName.toUtf8().data() );
	DString_SetChars( messageTuple->values[INDEX_INFO]->xString.value, info.toUtf8().data() );

	DaoTuple_SetItem( messageTuple, (DaoValue*)array, INDEX_NUMBERS );
}
void DaoInterpreter::ViewList( DaoList *list )
{
	DaoType *itp, *type = DaoNamespace_GetType( nameSpace, (DaoValue*)list );
	QString itemName = "List[" + StringAddress(  list ) + "]";
	QString info = "# address:\n" + StringAddress( list ) + "\n# type:\n";
	size_t i, n = list->value->size;
	bool sametype = false;

	info += type ? type->name->chars : "?";
	info += "\n# items:\n" + QString::number( n );

	InitMessage( (DaoValue*)list );
	DString_SetChars( messageTuple->values[INDEX_NAME]->xString.value, itemName.toUtf8().data() );
	DString_SetChars( messageTuple->values[INDEX_INFO]->xString.value, info.toUtf8().data() );

	if( type && type->nested->size ){
		itp = type->nested->items.pType[0];
		if( itp->tid >= DAO_INTEGER && itp->tid <= DAO_COMPLEX ){
			type = itp;
			sametype = true;
		}
	}
	valueTuple->values[2]->xString.value->size = 0;
	for(i=0; i<n; i++){
		DaoValue *val = list->value->items.pValue[i];
		itp = sametype ? type : DaoNamespace_GetType( nameSpace, val );
		valueTuple->values[0]->xString.value->size = 0;
		valueTuple->values[1]->xString.value->size = 0;
		if( itp ) DString_SetChars( valueTuple->values[0]->xString.value, itp->name->chars );
		DaoValue_GetString( val, daoString );
		DString_SetChars( valueTuple->values[1]->xString.value, daoString->chars );
		DaoList_PushBack( varList, (DaoValue*)valueTuple );
	}
}
void DaoInterpreter::ViewMap( DaoMap *map )
{
	DaoType *itp, *type = DaoNamespace_GetType( nameSpace, (DaoValue*)map );
	QString itemName = "Map[" + StringAddress(  map ) + "]";
	QString info = "# address:\n" + StringAddress( map ) + "\n# type:\n";
	DNode *node;
	size_t i, n = map->value->size;

	info += type ? type->name->chars : "?";
	info += "\n# items:\n" + QString::number( n );

	InitMessage( (DaoValue*)map );
	DString_SetChars( messageTuple->values[INDEX_NAME]->xString.value, itemName.toUtf8().data() );
	DString_SetChars( messageTuple->values[INDEX_INFO]->xString.value, info.toUtf8().data() );

	valueTuple->values[2]->xString.value->size = 0;
	for(node=DMap_First(map->value); node!=NULL; node=DMap_Next(map->value,node) ){
		itp = DaoNamespace_GetType( nameSpace, node->key.pValue );
		if( itp ) DString_SetChars( valueTuple->values[0]->xString.value, itp->name->chars );
		DaoValue_GetString( node->key.pValue, daoString );
		DString_SetChars( valueTuple->values[1]->xString.value, daoString->chars );
		DaoList_PushBack( constList, (DaoValue*)valueTuple );
		itp = DaoNamespace_GetType( nameSpace, node->value.pValue );
		if( itp ) DString_SetChars( valueTuple->values[0]->xString.value, itp->name->chars );
		DaoValue_GetString( node->value.pValue, daoString );
		DString_SetChars( valueTuple->values[1]->xString.value, daoString->chars );
		DaoList_PushBack( varList, (DaoValue*)valueTuple );
	}
}
void DaoInterpreter::ViewTuple( DaoTuple *tuple )
{
	DaoType *type = DaoNamespace_GetType( nameSpace, (DaoValue*)tuple );
	QString itemName = "Tuple[" + StringAddress(  tuple ) + "]";
	QString info = "# address:\n" + StringAddress( tuple ) + "\n# type:\n";
	QMap<int,QString> idnames;
	DNode *node;
	int i;

	info += type ? type->name->chars : "?";
	info += "\n# items:\n" + QString::number( tuple->size );

	InitMessage( (DaoValue*)tuple );
	DString_SetChars( messageTuple->values[INDEX_NAME]->xString.value, itemName.toUtf8().data() );
	DString_SetChars( messageTuple->values[INDEX_INFO]->xString.value, info.toUtf8().data() );

	MakeList( varList, tuple->values, tuple->size, NULL, type->mapNames, 0 );
}
void DaoInterpreter::ViewRoutine( DaoRoutine *routine )
{
	DMap *map = DMap_New(DAO_DATA_STRING,0);

	if( routine->body ){
		while( routine->body->revised ) routine = routine->body->revised;
		DaoToken **tokens = routine->body->defLocals->items.pToken;
		int i, n = routine->body->defLocals->size;
		for(i=0; i<n; i++) if( tokens[i]->type ==0 )
			DMap_Insert( map, & tokens[i]->string, (void*)(size_t)tokens[i]->index );
	}

	QString itemName = "Routine[" + QString( routine->routName->chars ) + "]";
	QString info = RoutineInfo( routine, routine );

	InitMessage( (DaoValue*)routine );
	DString_SetChars( messageTuple->values[INDEX_NAME]->xString.value, itemName.toUtf8().data() );
	DString_SetChars( messageTuple->values[INDEX_INFO]->xString.value, info.toUtf8().data() );

	extraTuple->values[1]->xString.value->size = 0;
	extraTuple->values[2]->xString.value->size = 0;

	info = "Namespace[" + StringAddress( routine->nameSpace ) + "]";
	DString_SetChars( extraTuple->values[0]->xString.value, info.toUtf8().data() );
	DaoList_PushBack( extraList, (DaoValue*)extraTuple );
	if( routine->routHost && routine->routHost->tid == DAO_OBJECT ){
		info = "Class[" + StringAddress( routine->routHost->aux ) + "]";
		DString_SetChars( extraTuple->values[0]->xString.value, info.toUtf8().data() );
		DaoList_PushBack( extraList, (DaoValue*)extraTuple );
	}

	MakeList( constList, routine->routConsts->value->items.pValue, routine->routConsts->value->size, NULL, map, 0 );
	if( routine->body ) ViewVmCodes( codeList, routine );
	DMap_Delete( map );
}
void DaoInterpreter::ViewRoutines( DaoRoutine *routines )
{
	QString itemName = "Routines[" + QString( routines->routName->chars ) + "]";
	QString info = "# address:\n" + StringAddress( routines );
	info += "\n# type: routine";
	info += "\n# number of overload:\n" + QString::number( routines->overloads->routines->size );

	InitMessage( (DaoValue*)routines );
	DString_SetChars( messageTuple->values[INDEX_NAME]->xString.value, itemName.toUtf8().data() );
	DString_SetChars( messageTuple->values[INDEX_INFO]->xString.value, info.toUtf8().data() );

	extraTuple->values[1]->xString.value->size = 0;
	extraTuple->values[2]->xString.value->size = 0;

	for(size_t i=0; i<routines->overloads->routines->size; i++){
		DaoRoutine *rout = routines->overloads->routines->items.pRoutine[i];
		info = "";
		if( rout->body ){
			info = "Routine[";
		}else{
			info = "Function[";
		}
		info += StringAddress( rout ) + "]";
		DString_SetChars( extraTuple->values[0]->xString.value, info.toUtf8().data() );
		DString_SetChars( extraTuple->values[1]->xString.value, rout->routType->name->chars );
		DaoList_PushBack( extraList, (DaoValue*)extraTuple );
	}
}
void DaoInterpreter::ViewClass( DaoClass *klass )
{
	size_t i;
	QString itemName = "Class[" + QString( klass->className->chars ) + "]";
	QString info, data;

	InitMessage( (DaoValue*)klass );
	DString_SetChars( messageTuple->values[INDEX_NAME]->xString.value, itemName.toUtf8().data() );

	extraTuple->values[1]->xString.value->size = 0;
	extraTuple->values[2]->xString.value->size = 0;

	data = "# address:\n" + StringAddress( klass );
	data += "\n# type\n" + QString( klass->clsType->name->chars );
	data += "\n# constants\n" + QString::number( klass->constants->size );
	data += "\n# statics\n" + QString::number( klass->variables->size );
	data += "\n# variables\n" + QString::number( klass->objDataName->size );
	if( klass->allBases->size ) data += "\n# parent(s)\n";

	for(i=0; i<klass->allBases->size; i++){
		DaoValue *sup = klass->allBases->items.pValue[i];
		info = "Class[" + StringAddress( sup ) + "]";
		if( sup->type == DAO_CLASS ){
			data += sup->xClass.className->chars + QString("\n");
		}else{
			data += sup->xCtype.ctype->name->chars + QString("\n");
		}
		DString_SetChars( extraTuple->values[0]->xString.value, info.toUtf8().data() );
		DaoList_PushBack( extraList, (DaoValue*)extraTuple );
	}
	DString_SetChars( messageTuple->values[INDEX_INFO]->xString.value, data.toUtf8().data() );

	MakeList( constList, klass->constants->items.pValue,
			klass->constants->size, NULL, klass->lookupTable, DAO_CLASS_CONSTANT );
	MakeList( varList, klass->variables->items.pValue, klass->variables->size, 
			NULL, klass->lookupTable, DAO_CLASS_VARIABLE );
}
void DaoInterpreter::ViewObject( DaoObject *object )
{
	DaoClass *klass = object->defClass;
	QString itemName = "Object[" + QString( object->defClass->className->chars ) + "]";
	QString info, data;
	size_t i;

	data = "# address:\n" + StringAddress( object );
	data += "\n# type\n" + QString( klass->className->chars );
	data += "\n# variables\n" + QString::number( klass->objDataName->size );
	if( klass->parent ) data += "\n# parent(s)\n1";

	InitMessage( (DaoValue*)object );
	DString_SetChars( messageTuple->values[INDEX_NAME]->xString.value, itemName.toUtf8().data() );
	DString_SetChars( messageTuple->values[INDEX_INFO]->xString.value, data.toUtf8().data() );

	extraTuple->values[1]->xString.value->size = 0;
	extraTuple->values[2]->xString.value->size = 0;

	info = "Class[" + StringAddress( klass ) + "]";
	DString_SetChars( extraTuple->values[0]->xString.value, info.toUtf8().data() );
	DaoList_PushBack( extraList, (DaoValue*)extraTuple );
	if( object->parent ){
		info = "Object[" + StringAddress( object->parent ) + "]";
		DString_SetChars( extraTuple->values[0]->xString.value, info.toUtf8().data() );
		DaoList_PushBack( extraList, (DaoValue*)extraTuple );
	}

	MakeList( varList, object->objValues, klass->objDataName->size,
			klass->instvars, klass->lookupTable, DAO_OBJECT_VARIABLE );
}
void DaoInterpreter::ViewNamespace( DaoNamespace *nspace )
{
	size_t i;
	QString info, itemName = "Namespace[" + StringAddress( nspace ) + "]";
	info += "# address:\n" + StringAddress( nspace );
	info += "\n# constants:\n";
	info += QString::number( nspace->constants->size );
	info += "\n# variables:\n";
	info += QString::number( nspace->variables->size );
	if( nspace->file->chars ){
		info += "\n\n# path:\n\"" + QString( nspace->path->chars );
		info += "\"\n# file:\n\"" + QString( nspace->file->chars ) + "\"";
	}
	InitMessage( (DaoValue*)nspace );
	DString_SetChars( messageTuple->values[INDEX_NAME]->xString.value, itemName.toUtf8().data() );
	DString_SetChars( messageTuple->values[INDEX_INFO]->xString.value, info.toUtf8().data() );

	extraTuple->values[2]->xString.value->size = 0;
	for(i=0; i<nspace->namespaces->size; i++){
		DaoNamespace *ns = nspace->namespaces->items.pNS[i];
		QString itname = "Namespace[" + StringAddress(ns) + "]";
		DString_SetChars( extraTuple->values[0]->xString.value, itname.toUtf8().data() );
		DString_Assign( extraTuple->values[1]->xString.value, ns->name );
		DaoList_PushBack( extraList, (DaoValue*)extraTuple );
	}

	MakeList( constList, nspace->constants->items.pValue, nspace->constants->size,
			NULL, nspace->lookupTable, DAO_GLOBAL_CONSTANT );
	MakeList( varList, nspace->variables->items.pValue, nspace->variables->size,
			NULL, nspace->lookupTable, DAO_GLOBAL_VARIABLE );

#if 0
	for(i=0; i<nspace->constants->size; i++){
		DaoValue *val = nspace->constants->items.pValue[i];
		it = wgtDataTable->item(i,0);
		if( it == NULL ) continue;
		if( val->type == DAO_CLASS && val->xClass.classRoutine->nameSpace == nspace )
			it->setBackground( QColor(200,250,200) );
		else if( val->type == DAO_ROUTINE && val->xRoutine.nameSpace == nspace )
			it->setBackground( QColor(200,250,200) );
	}
#endif
}
void DaoInterpreter::ViewProcess( DaoProcess *process )
{
	int i, index = 0;
	DaoStackFrame *frame = process->topFrame;
	while(frame && frame != process->firstFrame) {
		frame = frame->prev;
		index++;
	}

	QString info, itemName = "Process[" + StringAddress( process ) + "]";
	info = "# address:\n" + StringAddress( process );
	info += "\n# type:\nVM Process\n# frames:\n";
	info += QString::number( process->topFrame ? index : 10 );

	InitMessage( (DaoValue*)process );
	DString_SetChars( messageTuple->values[INDEX_NAME]->xString.value, itemName.toUtf8().data() );
	DString_SetChars( messageTuple->values[INDEX_INFO]->xString.value, info.toUtf8().data() );

	if( process->topFrame == NULL ) return;

	valueTuple->values[0]->xString.value->size = 0;
	valueTuple->values[1]->xString.value->size = 0;
	valueTuple->values[2]->xString.value->size = 0;

	QTableWidgetItem *it = NULL;
	frame = process->topFrame;
	for(i=0; frame && frame!=process->firstFrame; i++, frame=frame->prev){
		DString_SetChars( valueTuple->values[0]->xString.value, frame->routine->routName->chars );
		DString_SetChars( valueTuple->values[1]->xString.value, frame->routine->routType->name->chars );
		DaoList_PushBack( constList, (DaoValue*)valueTuple );
	}
}
void DaoInterpreter::MakeList( DaoList *list, DaoValue **data, int size, DArray *type, DMap *names, int filter )
{
	DNode *node;
	DaoType *itp = NULL;
	QMap<int,QString> idnames;
	if( names ){
		for(node=DMap_First(names); node!=NULL; node=DMap_Next(names,node)){
			if( filter && LOOKUP_ST( node->value.pInt ) != filter ) continue;
			if( LOOKUP_UP( node->value.pInt ) ) continue;
			// LOOKUP_ID: get the last 16 bits
			idnames[ LOOKUP_ID( node->value.pInt ) ] = node->key.pString->chars;
		}
	}
	DaoList_Clear( list );
	for(int i=0; i<size; i++){
		DaoValue *val = data[i];
		valueTuple->values[0]->xString.value->size = 0;
		valueTuple->values[1]->xString.value->size = 0;
		valueTuple->values[2]->xString.value->size = 0;
		if( val && (val->type == DAO_CONSTANT || val->type == DAO_VARIABLE) )
			val = val->xConst.value;
		if( val == NULL ){
			DString_SetChars( valueTuple->values[0]->xString.value, "none" );
		}else if( idnames.find(i) != idnames.end() ){
			DString_SetChars( valueTuple->values[0]->xString.value, idnames[i].toUtf8().data() );
		}else if( val->type == DAO_CLASS ){
			DString_SetChars( valueTuple->values[0]->xString.value, val->xClass.className->chars );
		}else if( val->type == DAO_ROUTINE ){
			DString_SetChars( valueTuple->values[0]->xString.value, val->xRoutine.routName->chars );
		}
		itp = type ? type->items.pType[i] : NULL;
		if( itp && type && itp->type == DAO_VARIABLE ) itp = type->items.pVar[i]->dtype;
		if( data[i] && data[i]->type == DAO_VARIABLE ) itp = data[i]->xVar.dtype;
		if( itp == NULL ) itp = DaoNamespace_GetType( vmSpace->mainNamespace, val );
		if( itp ) DString_SetChars( valueTuple->values[1]->xString.value, itp->name->chars );
		DString *daoString = valueTuple->values[2]->xString.value;
		if( val ) DaoValue_GetString( val, daoString );
		if( DString_Size( daoString ) > 50 ) DString_Erase( daoString, 50, (size_t)-1 );
		DaoList_PushBack( list, (DaoValue*)valueTuple );
	}
}
void DaoInterpreter::ViewTupleData( DaoTuple *tuple, DaoTuple *request )
{
	int table = request->values[1]->xInteger.value;
	int row = request->values[2]->xInteger.value;
	int col = request->values[3]->xInteger.value;
	if( table == DATA_TABLE ) ViewValue( tuple->values[row] );
}
void DaoInterpreter::ViewListData( DaoList *list, DaoTuple *request )
{
	int table = request->values[1]->xInteger.value;
	int row = request->values[2]->xInteger.value;
	int col = request->values[3]->xInteger.value;
	if( table == DATA_TABLE ) ViewValue( list->value->items.pValue[row] );
}
void DaoInterpreter::ViewMapData( DaoMap *map, DaoTuple *request )
{
	int table = request->values[1]->xInteger.value;
	int row = request->values[2]->xInteger.value;
	int col = request->values[3]->xInteger.value;
}
void DaoInterpreter::ViewRoutineData( DaoRoutine *routine, DaoTuple *request )
{
	int table = request->values[1]->xInteger.value;
	int row = request->values[2]->xInteger.value;
	int col = request->values[3]->xInteger.value;
	if( table == INFO_TABLE ){
		switch( row ){
		case 0 : ViewValue( (DaoValue*)routine->nameSpace ); break;
		case 1 : ViewValue( routine->routHost->aux ); break;
		}
	}else if( table == DATA_TABLE ){
		ViewValue( routine->routConsts->value->items.pValue[row] );
	}
}
void DaoInterpreter::ViewRoutinesData( DaoRoutine *routines, DaoTuple *request )
{
	int table = request->values[1]->xInteger.value;
	int row = request->values[2]->xInteger.value;
	int col = request->values[3]->xInteger.value;
	if( table == INFO_TABLE ){
	}else if( table == DATA_TABLE ){
		ViewValue( routines->overloads->routines->items.pValue[row] );
	}
}
void DaoInterpreter::ViewClassData( DaoClass *klass, DaoTuple *request )
{
	int table = request->values[1]->xInteger.value;
	int row = request->values[2]->xInteger.value;
	int col = request->values[3]->xInteger.value;
	if( table == INFO_TABLE ){
		ViewValue( klass->allBases->items.pValue[row] );
	}else if( table == DATA_TABLE ){
		ViewValue( klass->constants->items.pConst[row]->value );
	}else if( table == CODE_TABLE ){
		ViewValue( klass->variables->items.pVar[row]->value );
	}
}
void DaoInterpreter::ViewObjectData( DaoObject *object, DaoTuple *request )
{
	int table = request->values[1]->xInteger.value;
	int row = request->values[2]->xInteger.value;
	int col = request->values[3]->xInteger.value;
	if( table == INFO_TABLE ){
		switch( row ){
		case 0 : ViewValue( (DaoValue*)object->defClass ); break;
		default : ViewValue( object->parent ); break;
		}
	}else if( table == DATA_TABLE ){
		ViewValue( object->objValues[row] );
	}
}
void DaoInterpreter::ViewNamespaceData( DaoNamespace *nspace, DaoTuple *request )
{
	int table = request->values[1]->xInteger.value;
	int row = request->values[2]->xInteger.value;
	int col = request->values[3]->xInteger.value;
	if( table == INFO_TABLE ){
		ViewValue( nspace->namespaces->items.pValue[row] );
	}else if( table == DATA_TABLE ){
		ViewValue( nspace->constants->items.pConst[row]->value );
	}else if( table == CODE_TABLE ){
		ViewValue( nspace->variables->items.pVar[row]->value );
	}
}
void DaoInterpreter::ViewProcessStack( DaoProcess *process, DaoTuple *request )
{
	int table = request->values[1]->xInteger.value;
	int row = request->values[2]->xInteger.value;
	int col = request->values[3]->xInteger.value;
	if( table == DATA_TABLE ){
		DaoStackFrame *frame = process->topFrame;
		while( (row--) >0 ) frame = frame->prev;
		ViewStackFrame( frame, process );
		messageTuple->values[INDEX_INIT]->xInteger.value = 0;
		SendDataInformation();
	}
}
void DaoInterpreter::ViewStackData( DaoProcess *proc, DaoStackFrame *frame, DaoTuple *request )
{
	int table = request->values[1]->xInteger.value;
	int row = request->values[2]->xInteger.value;
	int col = request->values[3]->xInteger.value;
	if( table == INFO_TABLE ){
		DaoStackFrame *frame = (DaoStackFrame*)extraStack->items.pVoid[0];
		switch( row ){
		case 0 : ViewValue( (DaoValue*)frame->routine->nameSpace ); break;
		case 1 : ViewValue( (DaoValue*)frame->routine ); break;
		case 2 : ViewValue( (DaoValue*)frame->object ); break;
		}
	}else if( table == DATA_TABLE ){
		ViewValue( proc->stackValues[ frame->stackBase + row ] );
	}else if( table == CODE_TABLE ){
		if( col ) return;
		if( row >= frame->entry ) return;
		while( frame != proc->topFrame ) DaoProcess_PopFrame( proc );
		proc->topFrame->entry = row;
		proc->status = DAO_PROCESS_STACKED;
	}
}
QString DaoInterpreter::RoutineInfo( DaoRoutine *routine, void *address )
{
	DaoNamespace *ns = routine->nameSpace;
	QString info;
	info += "# address:\n" + StringAddress( address );
	info += "\n# function:\n";
	info += routine->routName->chars;
	info += "\n# type:\n";
	info += routine->routType->name->chars;
	if( routine->body ){
		info += "\n\n# instructions:\n";
		info += QString::number(routine->body->vmCodes->size);
		info += "\n# variables:\n";
		info += QString::number(routine->body->regCount);
	}
	if( ns->file->chars ){
		info += "\n\n# path:\n\"" + QString( ns->path->chars );
		info += "\"\n# file:\n\"" + QString( ns->file->chars ) + "\"";
	}
	return info;
}
void DaoInterpreter::ViewStackFrame( DaoStackFrame *frame, DaoProcess *process )
{
	DaoRoutine *routine = frame->routine;
	DMap *map = DMap_New(DAO_DATA_STRING,0);
	DaoToken **tokens = routine->body->defLocals->items.pToken;
	int i, entry, n = routine->body->defLocals->size;
	for(i=0; i<n; i++) if( tokens[i]->type )
		DMap_Insert( map, & tokens[i]->string, (void*)(size_t)tokens[i]->index );

	QString info = RoutineInfo( routine, frame );
	QString itemName = "StackFrame[" + StringAddress( frame ) + "]";

	InitMessage( (DaoValue*)process );
	entry = frame == process->topFrame ? process->activeCode - frame->codes : frame->entry;
	extraStack->items.pVoid[0] = frame;
	messageTuple->values[INDEX_ADDR]->xInteger.value = (daoint) frame;
	messageTuple->values[INDEX_TYPE]->xInteger.value = DAO_FRAME_ROUT;
	messageTuple->values[INDEX_ENTRY]->xInteger.value = entry;
	DString_SetChars( messageTuple->values[INDEX_NAME]->xString.value, itemName.toUtf8().data() );
	DString_SetChars( messageTuple->values[INDEX_INFO]->xString.value, info.toUtf8().data() );

	extraTuple->values[1]->xString.value->size = 0;
	extraTuple->values[2]->xString.value->size = 0;

	QString itname = "Namespace[" + StringAddress( routine->nameSpace ) + "]";
	DString_SetChars( extraTuple->values[0]->xString.value, itname.toUtf8().data() );
	DaoList_PushBack( extraList, (DaoValue*)extraTuple );
	itname = "Routine[" + StringAddress( routine ) + "]";
	DString_SetChars( extraTuple->values[0]->xString.value, itname.toUtf8().data() );
	DaoList_PushBack( extraList, (DaoValue*)extraTuple );
	if( frame->object ){
		itname = "Object[" + StringAddress( frame->object ) + "]";
		DString_SetChars( extraTuple->values[0]->xString.value, itname.toUtf8().data() );
		DaoList_PushBack( extraList, (DaoValue*)extraTuple );
	}

	if( routine->body->vmCodes->size ==0 ) return;

	MakeList( varList, process->stackValues + frame->stackBase, routine->body->regCount, 
			routine->body->regType, map, 0 );
	ViewVmCodes( codeList, frame->routine );
	DMap_Delete( map );
}
void DaoInterpreter::ViewVmCodes( DaoList *list, DaoRoutine *routine )
{
	int i, n = routine->body->vmCodes->size;
	for(i=0; i<n; i++){
		DaoVmCode vmc0 = routine->body->vmCodes->data.codes[i];
		DaoVmCodeX *vmc = routine->body->annotCodes->items.pVmc[i];
		codeTuple->values[0]->xInteger.value = vmc0.code;
		codeTuple->values[1]->xInteger.value = vmc->a;
		codeTuple->values[2]->xInteger.value = vmc->b;
		codeTuple->values[3]->xInteger.value = vmc->c;
		codeTuple->values[4]->xInteger.value = vmc->line;
		DaoLexer_AnnotateCode( routine->body->source, *vmc, daoString, 32 );
		DString_SetChars( codeTuple->values[5]->xString.value, daoString->chars );
		DaoList_PushBack( codeList, (DaoValue*)codeTuple );
	}
}
