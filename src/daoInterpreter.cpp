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


#if 0
extern "C"{
//DaoValue* DaoParseNumber( DaoToken *tok, DLong *bigint, DaoComplex *buffer );
}
void DaoDataWidget::slotUpdateValue()
{
	if( currentValue ==NULL ) return;
	DaoToken *tok = NULL;
	DaoValue *number, *value = currentValue;
	QByteArray text = wgtDataValue->toPlainText().toUtf8();
	bool ok = true;
	if( value->type && (value->type <= DAO_DOUBLE || value->type == DAO_LONG) ){
		DaoToken_Tokenize( tokens, text.data(), 0, 1, 0 );
		if( tokens->size ) tok = tokens->items.pToken[0];
		if( tokens->size !=1 || tok->type <DTOK_DIGITS_HEX || tok->type >DTOK_NUMBER_SCI ){
			QMessageBox::warning( this, tr("DaoStudio"),
					tr("Invalid value for the data type!"), QMessageBox::Cancel );
			return;
		}
		//XXX number = DaoParseNumber( tok, daoLong );
	}
	switch( value->type ){
	case DAO_INTEGER : value->xInteger.value = DaoValue_GetInteger( number ); break;
	case DAO_FLOAT   : value->xFloat.value = DaoValue_GetFloat( number ); break;
	case DAO_DOUBLE  : value->xDouble.value = DaoValue_GetDouble( number ); break;
	case DAO_COMPLEX :
					  break;
	case DAO_STRING :
					  DString_SetDataMBS( value->xString.data, text.data(), text.size() );
					  break;
	case DAO_LONG :
					  break;
	default : break;
	}
	if( ok ){
		ViewValue( currentValue );
	}else{
		QMessageBox::warning( this, tr("DaoStudio"),
				tr("Invalid value for the data type!"), QMessageBox::Cancel );
	}
}
void DaoDataWidget::slotElementChanged(int row, int col)
{
	if( currentValue == NULL || currentValue->type != DAO_ARRAY ) return;
	int id = row * wgtDataTable->columnCount() + col;
	DaoValue *number;
	DaoToken *tok = NULL;
	DaoArray *array = (DaoArray*) currentValue;
	QByteArray text = wgtDataTable->item(row,col)->text().toLocal8Bit();
	DaoToken_Tokenize( tokens, text.data(), 0, 1, 0 );
	if( array->etype >= DAO_INTEGER && array->etype <= DAO_DOUBLE ){
		if( tokens->size ) tok = tokens->items.pToken[0];
		if( tokens->size !=1 || tok->type <DTOK_DIGITS_HEX || tok->type >DTOK_NUMBER_SCI ){
			QMessageBox::warning( this, tr("DaoStudio"),
					tr("Invalid value for the data type!"), QMessageBox::Cancel );
			return;
		}
		//XXX number = DaoParseNumber( tok, daoLong );
	}
	switch( array->etype ){
	case DAO_INTEGER :
		array->data.i[id] = DaoValue_GetInteger( number );
		break;
	case DAO_FLOAT :
		array->data.f[id] = DaoValue_GetFloat( number );
		break;
	case DAO_DOUBLE :
		array->data.d[id] = DaoValue_GetDouble( number );
		break;
	case DAO_COMPLEX : // TODO
		break;
	}
}
#endif

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
	//printf( "entry: %s\n", annotCodes[i]->annot->mbs->data );
}
static void DaoStdioRead( DaoConsoleStream *self, DString *buf, int count )
{
	self->interpreter->mutex.lock();
	fflush( stdout );
	self->socket2.flush();
	self->socket.connectToServer( DaoStudioSettings::socket_stdin );
	self->socket.write( QByteArray::number( count ) );
	self->socket.flush();
	QObject::connect( & self->socket, SIGNAL(disconnected()),
			self->interpreter, SLOT(slotExitWaiting()) );
	QByteArray data;
	self->interpreter->waiting = true;
	while( self->interpreter->waiting ){
		QCoreApplication::processEvents( QEventLoop::AllEvents, 1000 );
		fflush( stdout );
		if( self->process->stopit ) break;
	}
	self->socket.waitForReadyRead();
	data += self->socket.readAll();
	//QString s = QString::fromUtf8( data.data(), data.size() );
	DString_SetDataMBS( buf, data.data(), data.size() );
	self->interpreter->mutex.unlock();
}
static void DaoStdioWrite( DaoConsoleStream *self, DString *buf )
{
#if 0
	FILE *fout = fopen( "output.txt", "a+" );
	time_t t = time(NULL);
	fprintf( fout, "%s: ", ctime( & t ) );
	fprintf( fout, "%s\n", DString_GetMBS( buf ) );
	fclose( fout );
#endif
	self->interpreter->mutex.lock();
	self->socket2.write( DString_GetMBS( buf ), DString_Size( buf ) );
	self->socket2.flush();
	self->interpreter->mutex.unlock();
}
static void DaoSudio_SetColor( DaoConsoleStream *self, const char *fgcolor, const char *bgcolor )
{
	char buf[32];
	int n = sprintf( buf, "\1%s:%s\2", fgcolor?fgcolor:"", bgcolor?bgcolor:"" );
	self->interpreter->mutex.lock();
	self->socket2.write( buf, n );
	self->socket2.flush();
	self->interpreter->mutex.unlock();
}
static void DaoConsDebug( DaoEventHandler *self, DaoProcess *process )
{
	self->interpreter->mutex.lock();

	DaoVmCode *codes = process->activeRoutine->body->vmCodes->data.codes;
	DaoVmCodeX **annots = process->activeRoutine->body->annotCodes->items.pVmc;
	int oldline = annots[ process->activeCode - codes + 1]->line;
	QFileInfo fi( process->activeRoutine->nameSpace->name->mbs );
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
	self->socket.write( send );
	self->socket.flush();
	QObject::connect( & self->socket, SIGNAL(disconnected()),
			self->interpreter, SLOT(slotExitWaiting()) );
	QByteArray data;
	self->interpreter->waiting = true;
	self->interpreter->debugProcess = process;
	self->interpreter->InitDataBrowser();
	while( self->interpreter->waiting ){
		QCoreApplication::processEvents( QEventLoop::AllEvents, 1000 );
		fflush( stdout );
	}
	//while( self->socket.state() != QLocalSocket::UnconnectedState ){
	self->socket.waitForReadyRead();
	data += self->socket.readAll();
	//self->socket.waitForReadyRead();
	//data += self->socket.readAll();
	//}
	//printf( "all:\n%s\n", QString(data).toUtf8().data() );

	DaoxDebugger & debugger = self->debugger;
	QList<int> lineMap;
	QStringList newCodes, routCodes;
	DaoRoutine *old = process->activeRoutine;
	int entry = DaoEditContinueData( data, lineMap, newCodes, routCodes );
	//printf( "all:\n%s\n", QString(data).toUtf8().data() );
	//printf( "new:\n%s\n", newCodes.join("\n").toUtf8().data() );
	//printf( "old:\n%s\n", routCodes.join("\n").toUtf8().data() );
	if( debugger.EditContinue( process, entry, lineMap, newCodes, routCodes ) ){
		if( old == process->activeRoutine && entry != oldline )
			DaoResetExecution( process, entry );
	}
	//printf( "%i %s\n", data.size(), data.data() );
	//QMessageBox::about( self->interpreter, "test", "test" );

	self->interpreter->mutex.unlock();
}
static void DaoSetBreaks( DaoEventHandler *self, DaoRoutine *routine )
{
	self->debugger.SetBreakPoints( routine );
}
static void DaoProcessMonitor( DaoEventHandler *self, DaoProcess *process )
{
	if( self->timer.time > self->time ){
		//printf( "time: %i  %i\n", self->timer.time, self->time );
		//fflush( stdout );
		self->time = self->timer.time;
		if( process != self->process ) return;
		QApplication::processEvents( QEventLoop::AllEvents, TIME_EVENT );
		fflush( stdout );
	}
}


DaoInterpreter::DaoInterpreter( const char *cmd ) : QObject()
{
	int i;

	locale = DaoStudioSettings::locale;
	program = DaoStudioSettings::program;
	programPath = DaoStudioSettings::program_path;

	vmState = DAOCON_READY;
	debugProcess = NULL;
	daoString = DString_New(1);
	daoLong = DLong_New();
	tokens = DArray_New( D_TOKEN );

	dataSocket = NULL;
	scriptSocket = NULL;

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
	handler.time = 0;
	handler.interpreter = this;
	handler.debug = DaoConsDebug;
	handler.breaks = DaoSetBreaks;
	//handler.Called = NULL;
	//handler.Returned = NULL;
	//handler.InvokeHost = DaoProcessMonitor;
	handler.timer.start();

	vmSpace = DaoInit( NULL ); //XXX
	vmSpace->options |= DAO_OPTION_IDE | DAO_OPTION_INTERUN;
	vmSpace->mainNamespace->options |= DAO_OPTION_IDE | DAO_OPTION_INTERUN;
	nameSpace = vmSpace->mainNamespace;
	handler.process = DaoVmSpace_MainProcess( vmSpace );
	profiler = DaoxProfiler_New();
	DaoVmSpace_SetUserStdio( vmSpace, (DaoUserStream*) & stdioStream );
	DaoVmSpace_SetUserStdError( vmSpace, (DaoUserStream*) & errorStream );
	DaoVmSpace_SetUserDebugger( vmSpace, (DaoDebugger*) & handler );
	DaoVmSpace_SetUserProfiler( vmSpace, (DaoProfiler*) profiler );

	DaoNamespace *ns = vmSpace->mainNamespace;
	DString_SetMBS( ns->name, "interactive codes" );
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
	DString *base = DString_New(1);
	DString *mod = DString_New(1);
	DString_SetMBS( base, programPath.toLocal8Bit().data() );
	DString_SetMBS( mod, "../Frameworks/lib/dao/modules/" );
	Dao_MakePath( base, mod );
	DaoVmSpace_AddPath( vmSpace, mod->mbs );
	DString_Delete( base );
	DString_Delete( mod );
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
	connect( shell, SIGNAL(readyReadStandardOutput()),
			this, SLOT(slotReadStdOut()));
	//connect( shell, SIGNAL(readyReadStandardError()),
	//	this, SLOT(slotReadStdOut()));
	connect( shell, SIGNAL(finished(int, QProcess::ExitStatus)),
			this, SLOT(slotShellFinished(int, QProcess::ExitStatus)));
}

DaoInterpreter::~DaoInterpreter()
{
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
	shell->setReadChannel( QProcess::StandardOutput );
	while( not shell->atEnd() ){
		QByteArray output = shell->readLine();
#if 0
		char *data = output.data();
		int i, n = output.size();
		for(i=0; i<n; i++) printf( "%c", data[i] );
		fflush( stdout );
#endif
		stdioStream.socket2.write( output.data(), output.size() );
		stdioStream.socket2.flush();
	}
	shell->setReadChannel( QProcess::StandardError );
	while( not shell->atEnd() ){
		QByteArray output = shell->readLine();
#if 0
		char *data = output.data();
		int i, n = output.size();
		for(i=0; i<n; i++) printf( "%c", data[i] );
		fflush( stdout );
#endif
		errorStream.socket2.write( output.data(), output.size() );
		errorStream.socket2.flush();
	}
}
void DaoInterpreter::slotShellFinished(int, QProcess::ExitStatus)
{
	handler.process->stopit = 1;
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
	DString_SetDataMBS( daoString, data.data(), data.size() );
	if( DaoValue_Deserialize( & value, daoString, nspace, process ) == 0 ) return;

	DaoTuple *request = (DaoTuple*) value;
	if( request->items[1]->xInteger.value == DATA_STACK ){
		int erase = request->items[2]->xInteger.value;
		DArray_Erase( & valueStack->items, 0, erase );
		DArray_Erase( extraStack, 0, erase );
		value = valueStack->items.items.pValue[0];
		if( extraStack->items.pValue[0] ){
			DaoList_PopFront( valueStack );
			DArray_PopFront( extraStack );
			ViewStackData( (DaoProcess*)value, (DaoStackFrame*)extraStack->items.pValue[0], request );
		}else{
			DaoList_PopFront( valueStack );
			DArray_PopFront( extraStack );
			ViewValue( value );
		}
		return;
	}

	value = valueStack->items.items.pValue[0];
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
		ViewFunctreeData( (DaoRoutine*)value, request );
		}else if( value->xRoutine.body ){
			ViewRoutineData( (DaoRoutine*)value, request );
		}else{
			ViewFunctionData( (DaoRoutine*)value, request );
		}
		break;
	case DAO_TUPLE : ViewTupleData( (DaoTuple*)value, request ); break;
	case DAO_LIST : ViewListData( (DaoList*)value, request ); break;
	case DAO_MAP : ViewMapData( (DaoMap*)value, request ); break;
	case DAO_CLASS : ViewClassData( (DaoClass*)value, request ); break;
	case DAO_OBJECT : ViewObjectData( (DaoObject*)value, request ); break;
	case DAO_NAMESPACE : ViewNamespaceData( (DaoNamespace*)value, request ); break;
	}
}
void DaoInterpreter::slotStartExecution()
{
	if( vmState == DAOCON_RUN ){
		//printf( "is running\n" );
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
		DaoVmSpace_SetPath( vmSpace, script.data() );
		QDir::setCurrent( QString::fromUtf8( script.data(), script.size() ) );
		scriptSocket->disconnectFromServer();
		return;
	}
	stdioStream.socket2.connectToServer( DaoStudioSettings::socket_stdout );
	if( stdioStream.socket2.waitForConnected( 1000 ) ==0 ){
		printf( "cannot connect to console stdout\n" );
		fflush( stdout );
		scriptSocket->disconnectFromServer();
		return;
	}
	errorStream.socket2.connectToServer( DaoStudioSettings::socket_stderr );
	if( errorStream.socket2.waitForConnected( 1000 ) ==0 ){
		printf( "cannot connect to console stderr\n" );
		fflush( stderr );
		scriptSocket->disconnectFromServer();
		return;
	}


	if( info )
		vmSpace->options |= DAO_OPTION_DEBUG;
	else
		vmSpace->options &= ~DAO_OPTION_DEBUG;

	vmState = DAOCON_RUN;
	DaoProcess *vmp = DaoVmSpace_MainProcess( vmSpace );
	DaoNamespace *ns = DaoVmSpace_MainNamespace( vmSpace );
	ns->options |= DAO_OPTION_IDE | DAO_NS_AUTO_GLOBAL;

	connect( & stdioStream.socket2, SIGNAL( disconnected() ), this, SLOT( slotStopExecution() ) );

	vmp->stopit = 0;
	handler.process = vmp;
	QTime time;
	time.start();
	int res = 0;
	if( script[0] == '\\' ){
		shell->start( script.mid(1).data(), QIODevice::ReadWrite | QIODevice::Unbuffered );
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
			//DaoProcessMonitor( & handler );
			//QApplication::processEvents( QEventLoop::WaitForMoreEvents, 100 );
			Sleeper::Sleep( 10000 );
			QApplication::processEvents( QEventLoop::AllEvents, 20 );
			if( vmp->stopit ){
				shell->kill();
				break;
			}
		}
		slotReadStdOut();
		res = shell->exitStatus();
		//printf( "stop: %i, status: %i, code: %i, error: %i\n", 
		//	vmSpace->stopit, res, shell->exitCode(), shell->error() );
	}else{
		DString *mbs = DString_New(1);
		DString_AppendDataMBS( mbs, script.data(), script.size() );
		res = (int) DaoProcess_Eval( vmp, ns, mbs->mbs );
		DaoCallServer_Join();
		DString_Delete( mbs );

		DaoProfiler *profiler = vmSpace->profiler;
		if( profiler->Report ) profiler->Report( profiler, vmSpace->stdioStream );
		if( profiler->Reset ) profiler->Reset( profiler );
	}
	fflush( stdout );
	stdioStream.socket2.flush();
	stdioStream.socket2.disconnectFromServer();
	errorStream.socket2.flush();
	errorStream.socket2.disconnectFromServer();

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
	socket.disconnectFromServer();
	//XXX ReduceValueItems( wgtDataList->item( wgtDataList->count()-1 ) );
	EraseDebuggingProcess();
	ViewNamespace( vmSpace->mainNamespace );
	messageTuple->items[INDEX_INIT]->xInteger.value = 1;
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
	//QMessageBox::about( this, "","" );
	DaoProcess *vmp = DaoVmSpace_MainProcess( vmSpace );
	vmp->stopit = 1;
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
	monitorSocket.connectToServer( DaoStudioSettings::socket_monitor );
	if( monitorSocket.waitForConnected( 1000 ) ==0 ){
		printf( "cannot connect to the virtual machine monitor\n" );
		fflush( stdout );
		return;
	}
	if( vmSpace == NULL ) return;
	DaoNamespace *nspace = vmSpace->mainNamespace;
	DaoProcess *process = vmSpace->mainProcess;
	if( DaoValue_Serialize( (DaoValue*) messageTuple, daoString, nspace, process )){
		//printf( "%s\n", daoString->mbs ); fflush(stdout);
		monitorSocket.write( daoString->mbs );
		monitorSocket.flush();
		monitorSocket.disconnectFromServer();
	}
}
void DaoInterpreter::InitDataBrowser()
{
	ViewNamespace( vmSpace->mainNamespace );
	messageTuple->items[INDEX_INIT]->xInteger.value = 1;
	SendDataInformation();
	ViewProcess( debugProcess );
	messageTuple->items[INDEX_INIT]->xInteger.value = 0;
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
	messageTuple->items[INDEX_ADDR]->xInteger.value = (daoint) value;
	messageTuple->items[INDEX_TYPE]->xInteger.value = value->type;
	DaoArray_ResizeVector( numArray, 0 );
	DaoTuple_SetItem( messageTuple, (DaoValue*)numArray, INDEX_NUMBERS );
	DaoList_Clear( extraList );
	DaoList_Clear( constList );
	DaoList_Clear( varList );
	DaoList_Clear( codeList );
	DString_Clear( messageTuple->items[INDEX_VALUE]->xString.data );
}
void DaoInterpreter::ViewValue( DaoValue *value )
{
	int ok = true;
	if( value == NULL ) return;

	DaoType *type = DaoNamespace_GetType( vmSpace->mainNamespace, value );
	QString info = "# address:\n" + StringAddress( value ) + "\n# type:\n";
	QString itemName = type ? type->name->mbs : "Value";
	if( itemName == "?" ) itemName = "Value";
	itemName += "[" + StringAddress( value ) + "]";
	itemName[0] = itemName[0].toUpper();
	info += type ? type->name->mbs : "?";
	if( value->type == DAO_STRING )
		info += "\n# length:\n" + QString::number( DString_Size( value->xString.data ) );

	switch( value->type ){
	case DAO_NONE :
	case DAO_INTEGER :
	case DAO_FLOAT :
	case DAO_DOUBLE :
	case DAO_COMPLEX :
	case DAO_LONG :
	case DAO_STRING :
		InitMessage( value );
		DaoValue_GetString( value, daoString );
		DString_SetMBS( messageTuple->items[INDEX_NAME]->xString.data, itemName.toUtf8().data() );
		DString_SetMBS( messageTuple->items[INDEX_INFO]->xString.data, info.toUtf8().data() );
		DString_SetMBS( messageTuple->items[INDEX_VALUE]->xString.data, DString_GetMBS( daoString ) );
		break;
	case DAO_ROUTINE :
		if( value->xRoutine.overloads ){
			ViewFunctree( (DaoRoutine*)value );
		}else if( value->xRoutine.body ){
			ViewRoutine( (DaoRoutine*)value );
		}else{
			ViewFunction( (DaoRoutine*)value );
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
		messageTuple->items[INDEX_INIT]->xInteger.value = 0;
		SendDataInformation();
	}
}
void DaoInterpreter::ViewArray( DaoArray *array )
{
	DaoType *type = DaoNamespace_GetType( nameSpace, (DaoValue*)array );
	QString itemName = "Array[" + StringAddress(  array ) + "]";
	QString info = "# address:\n" + StringAddress( array ) + "\n# type:\n";
	daoint i;

	info += type ? type->name->mbs : "?";
	info += "\n# elements:\n" + QString::number( array->size );
	info += "\n# shape:\n[ ";
	for(i=0; i<array->ndim; i++){
		if( i ) info += ", ";
		info += QString::number( array->dims[i] );
	}
	info += " ]";

	InitMessage( (DaoValue*)array );
	DString_SetMBS( messageTuple->items[INDEX_NAME]->xString.data, itemName.toUtf8().data() );
	DString_SetMBS( messageTuple->items[INDEX_INFO]->xString.data, info.toUtf8().data() );

	DaoTuple_SetItem( messageTuple, (DaoValue*)array, INDEX_NUMBERS );
}
void DaoInterpreter::ViewList( DaoList *list )
{
	DaoType *itp, *type = DaoNamespace_GetType( nameSpace, (DaoValue*)list );
	QString itemName = "List[" + StringAddress(  list ) + "]";
	QString info = "# address:\n" + StringAddress( list ) + "\n# type:\n";
	size_t i, n = list->items.size;
	bool sametype = false;

	info += type ? type->name->mbs : "?";
	info += "\n# items:\n" + QString::number( n );

	InitMessage( (DaoValue*)list );
	DString_SetMBS( messageTuple->items[INDEX_NAME]->xString.data, itemName.toUtf8().data() );
	DString_SetMBS( messageTuple->items[INDEX_INFO]->xString.data, info.toUtf8().data() );

	if( type && type->nested->size ){
		itp = type->nested->items.pType[0];
		if( itp->tid >= DAO_INTEGER && itp->tid <= DAO_LONG ){
			type = itp;
			sametype = true;
		}
	}
	valueTuple->items[2]->xString.data->size = 0;
	for(i=0; i<n; i++){
		DaoValue *val = list->items.items.pValue[i];
		itp = sametype ? type : DaoNamespace_GetType( nameSpace, val );
		valueTuple->items[0]->xString.data->size = 0;
		valueTuple->items[1]->xString.data->size = 0;
		if( itp ) DString_SetMBS( valueTuple->items[0]->xString.data, itp->name->mbs );
		DaoValue_GetString( val, daoString );
		DString_SetMBS( valueTuple->items[1]->xString.data, daoString->mbs );
		DaoList_PushBack( varList, (DaoValue*)valueTuple );
	}
}
void DaoInterpreter::ViewMap( DaoMap *map )
{
	DaoType *itp, *type = DaoNamespace_GetType( nameSpace, (DaoValue*)map );
	QString itemName = "Map[" + StringAddress(  map ) + "]";
	QString info = "# address:\n" + StringAddress( map ) + "\n# type:\n";
	DNode *node;
	size_t i, n = map->items->size;

	info += type ? type->name->mbs : "?";
	info += "\n# items:\n" + QString::number( n );

	InitMessage( (DaoValue*)map );
	DString_SetMBS( messageTuple->items[INDEX_NAME]->xString.data, itemName.toUtf8().data() );
	DString_SetMBS( messageTuple->items[INDEX_INFO]->xString.data, info.toUtf8().data() );

	valueTuple->items[2]->xString.data->size = 0;
	for(node=DMap_First(map->items); node!=NULL; node=DMap_Next(map->items,node) ){
		itp = DaoNamespace_GetType( nameSpace, node->key.pValue );
		if( itp ) DString_SetMBS( valueTuple->items[0]->xString.data, itp->name->mbs );
		DaoValue_GetString( node->key.pValue, daoString );
		DString_SetMBS( valueTuple->items[1]->xString.data, daoString->mbs );
		DaoList_PushBack( constList, (DaoValue*)valueTuple );
		itp = DaoNamespace_GetType( nameSpace, node->value.pValue );
		if( itp ) DString_SetMBS( valueTuple->items[0]->xString.data, itp->name->mbs );
		DaoValue_GetString( node->value.pValue, daoString );
		DString_SetMBS( valueTuple->items[1]->xString.data, daoString->mbs );
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

	info += type ? type->name->mbs : "?";
	info += "\n# items:\n" + QString::number( tuple->size );

	InitMessage( (DaoValue*)tuple );
	DString_SetMBS( messageTuple->items[INDEX_NAME]->xString.data, itemName.toUtf8().data() );
	DString_SetMBS( messageTuple->items[INDEX_INFO]->xString.data, info.toUtf8().data() );

	MakeList( varList, tuple->items, tuple->size, NULL, type->mapNames, 0 );
}
void DaoInterpreter::ViewRoutine( DaoRoutine *routine )
{
	while( routine->body->revised ) routine = routine->body->revised;
	DMap *map = DMap_New(D_STRING,0);
	DaoToken **tokens = routine->body->defLocals->items.pToken;
	int i, n = routine->body->defLocals->size;
	for(i=0; i<n; i++) if( tokens[i]->type ==0 )
		DMap_Insert( map, & tokens[i]->string, (void*)(size_t)tokens[i]->index );

	QString itemName = "Routine[" + QString( routine->routName->mbs ) + "]";
	QString info = RoutineInfo( routine, routine );

	InitMessage( (DaoValue*)routine );
	DString_SetMBS( messageTuple->items[INDEX_NAME]->xString.data, itemName.toUtf8().data() );
	DString_SetMBS( messageTuple->items[INDEX_INFO]->xString.data, info.toUtf8().data() );

	extraTuple->items[1]->xString.data->size = 0;
	extraTuple->items[2]->xString.data->size = 0;

	info = "Namespace[" + StringAddress( routine->nameSpace ) + "]";
	DString_SetMBS( extraTuple->items[0]->xString.data, info.toUtf8().data() );
	DaoList_PushBack( extraList, (DaoValue*)extraTuple );
	if( routine->routHost && routine->routHost->tid == DAO_OBJECT ){
		info = "Class[" + StringAddress( routine->routHost->aux ) + "]";
		DString_SetMBS( extraTuple->items[0]->xString.data, info.toUtf8().data() );
		DaoList_PushBack( extraList, (DaoValue*)extraTuple );
	}

	MakeList( constList, routine->routConsts->items.items.pValue, routine->routConsts->items.size, NULL, map, 0 );
	ViewVmCodes( codeList, routine );
	DMap_Delete( map );
}
void DaoInterpreter::ViewFunction( DaoRoutine *function )
{
}
void DaoInterpreter::ViewFunctree( DaoRoutine *functree )
{
	QString itemName = "Functree[" + QString( functree->routName->mbs ) + "]";
	QString info = "# address:\n" + StringAddress( functree );
	info += "\n# number of overload:\n" + QString::number( functree->overloads->routines->size );

	InitMessage( (DaoValue*)functree );
	DString_SetMBS( messageTuple->items[INDEX_NAME]->xString.data, itemName.toUtf8().data() );
	DString_SetMBS( messageTuple->items[INDEX_INFO]->xString.data, info.toUtf8().data() );

	extraTuple->items[1]->xString.data->size = 0;
	extraTuple->items[2]->xString.data->size = 0;

	for(size_t i=0; i<functree->overloads->routines->size; i++){
		DaoRoutine *rout = functree->overloads->routines->items.pRoutine[i];
		info = "";
		if( rout->body ){
			info = "Routine[";
		}else{
			info = "Function[";
		}
		info += StringAddress( rout ) + "]";
		DString_SetMBS( extraTuple->items[0]->xString.data, info.toUtf8().data() );
		DString_SetMBS( extraTuple->items[1]->xString.data, rout->routType->name->mbs );
		DaoList_PushBack( extraList, (DaoValue*)extraTuple );
	}
}
void DaoInterpreter::ViewClass( DaoClass *klass )
{
	size_t i;
	QString itemName = "Class[" + QString( klass->className->mbs ) + "]";
	QString info, data;

	InitMessage( (DaoValue*)klass );
	DString_SetMBS( messageTuple->items[INDEX_NAME]->xString.data, itemName.toUtf8().data() );

	extraTuple->items[1]->xString.data->size = 0;
	extraTuple->items[2]->xString.data->size = 0;

	data = "# address:\n" + StringAddress( klass );
	data += "\n# type\n" + QString( klass->clsType->name->mbs );
	data += "\n# constants\n" + QString::number( klass->constants->size );
	data += "\n# globals\n" + QString::number( klass->variables->size );
	data += "\n# variables\n" + QString::number( klass->objDataName->size );
	if( klass->parent ){
		data += "\n# parent\n";
		info = "Class[" + StringAddress( klass->parent ) + "]";
		if( klass->parent->type == DAO_CLASS ){
			data += klass->parent->xClass.className->mbs + QString("\n");
		}else{
			data += klass->parent->xCtype.ctype->name->mbs + QString("\n");
		}
		DString_SetMBS( extraTuple->items[0]->xString.data, info.toUtf8().data() );
		DaoList_PushBack( extraList, (DaoValue*)extraTuple );
	}
	DString_SetMBS( messageTuple->items[INDEX_INFO]->xString.data, data.toUtf8().data() );

	MakeList( constList, klass->constants->items.pValue,
			klass->constants->size, NULL, klass->lookupTable, DAO_CLASS_CONSTANT );
	MakeList( varList, klass->variables->items.pValue, klass->variables->size, 
			NULL, klass->lookupTable, DAO_CLASS_VARIABLE );
#if 0
	for(i=0; i<klass->constants->size; i++){
		DaoValue *val = klass->constants->items.pValue[i];
		//XXX if( val.t != DAO_ROUTINE || val.v.routine->tidHost != DAO_CLASS ) continue;
		//XXX if( val.v.routine->routHost->aux.v.klass == klass )
		//XXX	wgtDataTable->item(i,0)->setBackground( QColor(200,250,200) );
	}
#endif
}
void DaoInterpreter::ViewObject( DaoObject *object )
{
	DaoClass *klass = object->defClass;
	QString itemName = "Object[" + QString( object->defClass->className->mbs ) + "]";
	QString info, data;
	size_t i;

	data = "# address:\n" + StringAddress( object );
	data += "\n# type\n" + QString( klass->className->mbs );
	data += "\n# variables\n" + QString::number( klass->objDataName->size );
	if( klass->parent ) data += "\n# parent(s)\n1";

	InitMessage( (DaoValue*)object );
	DString_SetMBS( messageTuple->items[INDEX_NAME]->xString.data, itemName.toUtf8().data() );
	DString_SetMBS( messageTuple->items[INDEX_INFO]->xString.data, data.toUtf8().data() );

	extraTuple->items[1]->xString.data->size = 0;
	extraTuple->items[2]->xString.data->size = 0;

	info = "Class[" + StringAddress( klass ) + "]";
	DString_SetMBS( extraTuple->items[0]->xString.data, info.toUtf8().data() );
	DaoList_PushBack( extraList, (DaoValue*)extraTuple );
	if( object->parent ){
		info = "Object[" + StringAddress( object->parent ) + "]";
		DString_SetMBS( extraTuple->items[0]->xString.data, info.toUtf8().data() );
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
	if( nspace->file->mbs ){
		info += "\n\n# path:\n\"" + QString( nspace->path->mbs );
		info += "\"\n# file:\n\"" + QString( nspace->file->mbs ) + "\"";
	}
	InitMessage( (DaoValue*)nspace );
	DString_SetMBS( messageTuple->items[INDEX_NAME]->xString.data, itemName.toUtf8().data() );
	DString_SetMBS( messageTuple->items[INDEX_INFO]->xString.data, info.toUtf8().data() );

	extraTuple->items[2]->xString.data->size = 0;
	for(i=0; i<nspace->namespaces->size; i++){
		DaoNamespace *ns = nspace->namespaces->items.pNS[i];
		QString itname = "Namespace[" + StringAddress(ns) + "]";
		DString_SetMBS( extraTuple->items[0]->xString.data, itname.toUtf8().data() );
		DString_Assign( extraTuple->items[1]->xString.data, ns->name );
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
	DString_SetMBS( messageTuple->items[INDEX_NAME]->xString.data, itemName.toUtf8().data() );
	DString_SetMBS( messageTuple->items[INDEX_INFO]->xString.data, info.toUtf8().data() );

	if( process->topFrame == NULL ) return;

	valueTuple->items[0]->xString.data->size = 0;
	valueTuple->items[1]->xString.data->size = 0;
	valueTuple->items[2]->xString.data->size = 0;

	QTableWidgetItem *it = NULL;
	frame = process->topFrame;
	for(i=0; frame && frame!=process->firstFrame; i++, frame=frame->prev){
		DString_SetMBS( valueTuple->items[0]->xString.data, frame->routine->routName->mbs );
		DString_SetMBS( valueTuple->items[1]->xString.data, frame->routine->routType->name->mbs );
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
			idnames[ LOOKUP_ID( node->value.pInt ) ] = node->key.pString->mbs;
		}
	}
	DaoList_Clear( list );
	for(int i=0; i<size; i++){
		DaoValue *val = data[i];
		if( val->type == DAO_CONSTANT || val->type == DAO_VARIABLE ) val = val->xConst.value;
		valueTuple->items[0]->xString.data->size = 0;
		valueTuple->items[1]->xString.data->size = 0;
		valueTuple->items[2]->xString.data->size = 0;
		if( val == NULL ){
			DString_SetMBS( valueTuple->items[0]->xString.data, "none" );
		}else if( idnames.find(i) != idnames.end() ){
			DString_SetMBS( valueTuple->items[0]->xString.data, idnames[i].toUtf8().data() );
		}else if( val->type == DAO_CLASS ){
			DString_SetMBS( valueTuple->items[0]->xString.data, val->xClass.className->mbs );
		}else if( val->type == DAO_ROUTINE ){
			DString_SetMBS( valueTuple->items[0]->xString.data, val->xRoutine.routName->mbs );
		}
		itp = type ? type->items.pType[i] : NULL;
		if( itp && type && itp->type == DAO_VARIABLE ) itp = type->items.pVar[i]->dtype;
		if( data[i]->type == DAO_VARIABLE ) itp = data[i]->xVar.dtype;
		if( itp == NULL ) itp = DaoNamespace_GetType( vmSpace->mainNamespace, val );
		if( itp ) DString_SetMBS( valueTuple->items[1]->xString.data, itp->name->mbs );
		DString *daoString = valueTuple->items[2]->xString.data;
		if( val ) DaoValue_GetString( val, daoString );
		if( DString_Size( daoString ) > 50 ) DString_Erase( daoString, 50, (size_t)-1 );
		DaoList_PushBack( list, (DaoValue*)valueTuple );
	}
}
void DaoInterpreter::ViewTupleData( DaoTuple *tuple, DaoTuple *request )
{
	int table = request->items[1]->xInteger.value;
	int row = request->items[2]->xInteger.value;
	int col = request->items[3]->xInteger.value;
	if( table == DATA_TABLE ) ViewValue( tuple->items[row] );
}
void DaoInterpreter::ViewListData( DaoList *list, DaoTuple *request )
{
	int table = request->items[1]->xInteger.value;
	int row = request->items[2]->xInteger.value;
	int col = request->items[3]->xInteger.value;
	if( table == DATA_TABLE ) ViewValue( list->items.items.pValue[row] );
}
void DaoInterpreter::ViewMapData( DaoMap *map, DaoTuple *request )
{
	int table = request->items[1]->xInteger.value;
	int row = request->items[2]->xInteger.value;
	int col = request->items[3]->xInteger.value;
}
void DaoInterpreter::ViewRoutineData( DaoRoutine *routine, DaoTuple *request )
{
	int table = request->items[1]->xInteger.value;
	int row = request->items[2]->xInteger.value;
	int col = request->items[3]->xInteger.value;
	if( table == INFO_TABLE ){
		switch( row ){
		case 0 : ViewValue( (DaoValue*)routine->nameSpace ); break;
		case 1 : ViewValue( routine->routHost->aux ); break;
		}
	}else if( table == DATA_TABLE ){
		ViewValue( routine->routConsts->items.items.pValue[row] );
	}
}
void DaoInterpreter::ViewFunctionData( DaoRoutine *function, DaoTuple *request )
{
	int table = request->items[1]->xInteger.value;
	int row = request->items[2]->xInteger.value;
	int col = request->items[3]->xInteger.value;
}
void DaoInterpreter::ViewFunctreeData( DaoRoutine *functree, DaoTuple *request )
{
	int table = request->items[1]->xInteger.value;
	int row = request->items[2]->xInteger.value;
	int col = request->items[3]->xInteger.value;
	if( table == DATA_TABLE ) ViewValue( functree->overloads->routines->items.pValue[row] );
}
void DaoInterpreter::ViewClassData( DaoClass *klass, DaoTuple *request )
{
	int table = request->items[1]->xInteger.value;
	int row = request->items[2]->xInteger.value;
	int col = request->items[3]->xInteger.value;
	if( table == INFO_TABLE ){
		//ViewValue( klass->superClass->items.pValue[row] );
#warning "klass->superClass->items.pValue[row]"
	}else if( table == DATA_TABLE ){
		ViewValue( klass->constants->items.pValue[row] );
	}else if( table == CODE_TABLE ){
		ViewValue( klass->variables->items.pValue[row] );
	}
}
void DaoInterpreter::ViewObjectData( DaoObject *object, DaoTuple *request )
{
	int table = request->items[1]->xInteger.value;
	int row = request->items[2]->xInteger.value;
	int col = request->items[3]->xInteger.value;
	if( table == INFO_TABLE ){
		switch( row ){
		case 0 : ViewValue( (DaoValue*)object->defClass ); break;
		//default : ViewValue( object->parents[row-1] ); break;
#warning "object->parents[row-1]"
		}
	}else if( table == DATA_TABLE ){
		ViewValue( object->objValues[row] );
	}
}
void DaoInterpreter::ViewNamespaceData( DaoNamespace *nspace, DaoTuple *request )
{
	int table = request->items[1]->xInteger.value;
	int row = request->items[2]->xInteger.value;
	int col = request->items[3]->xInteger.value;
	if( table == INFO_TABLE ){
		ViewValue( nspace->namespaces->items.pValue[row] );
	}else if( table == DATA_TABLE ){
		ViewValue( nspace->constants->items.pValue[row] );
	}else if( table == CODE_TABLE ){
		ViewValue( nspace->variables->items.pValue[row] );
	}
}
void DaoInterpreter::ViewProcessStack( DaoProcess *process, DaoTuple *request )
{
	int table = request->items[1]->xInteger.value;
	int row = request->items[2]->xInteger.value;
	int col = request->items[3]->xInteger.value;
	if( table == DATA_TABLE ){
		DaoStackFrame *frame = process->topFrame;
		while( (row--) >0 ) frame = frame->prev;
		ViewStackFrame( frame, process );
		messageTuple->items[INDEX_INIT]->xInteger.value = 0;
		SendDataInformation();
	}
}
void DaoInterpreter::ViewStackData( DaoProcess *proc, DaoStackFrame *frame, DaoTuple *request )
{
	int table = request->items[1]->xInteger.value;
	int row = request->items[2]->xInteger.value;
	int col = request->items[3]->xInteger.value;
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
	info += routine->routName->mbs;
	info += "\n# type:\n";
	info += routine->routType->name->mbs;
	info += "\n\n# instructions:\n";
	info += QString::number(routine->body->vmCodes->size);
	info += "\n# variables:\n";
	info += QString::number(routine->body->regCount);
	if( ns->file->mbs ){
		info += "\n\n# path:\n\"" + QString( ns->path->mbs );
		info += "\"\n# file:\n\"" + QString( ns->file->mbs ) + "\"";
	}
	return info;
}
void DaoInterpreter::ViewStackFrame( DaoStackFrame *frame, DaoProcess *process )
{
	if( frame->routine == NULL ) return; // TODO: function

	DaoRoutine *routine = frame->routine;
	DMap *map = DMap_New(D_STRING,0);
	DaoToken **tokens = routine->body->defLocals->items.pToken;
	int i, n = routine->body->defLocals->size;
	for(i=0; i<n; i++) if( tokens[i]->type )
		DMap_Insert( map, & tokens[i]->string, (void*)(size_t)tokens[i]->index );

	QString info = RoutineInfo( routine, frame );
	QString itemName = "StackFrame[" + StringAddress( frame ) + "]";

	InitMessage( (DaoValue*)process );
	extraStack->items.pVoid[0] = frame;
	messageTuple->items[INDEX_ADDR]->xInteger.value = (daoint) frame;
	messageTuple->items[INDEX_TYPE]->xInteger.value = DAO_FRAME_ROUT;
	messageTuple->items[INDEX_ENTRY]->xInteger.value = frame->entry;
	DString_SetMBS( messageTuple->items[INDEX_NAME]->xString.data, itemName.toUtf8().data() );
	DString_SetMBS( messageTuple->items[INDEX_INFO]->xString.data, info.toUtf8().data() );

	extraTuple->items[1]->xString.data->size = 0;
	extraTuple->items[2]->xString.data->size = 0;

	QString itname = "Namespace[" + StringAddress( routine->nameSpace ) + "]";
	DString_SetMBS( extraTuple->items[0]->xString.data, itname.toUtf8().data() );
	DaoList_PushBack( extraList, (DaoValue*)extraTuple );
	itname = "Routine[" + StringAddress( routine ) + "]";
	DString_SetMBS( extraTuple->items[0]->xString.data, itname.toUtf8().data() );
	DaoList_PushBack( extraList, (DaoValue*)extraTuple );
	if( frame->object ){
		itname = "Object[" + StringAddress( frame->object ) + "]";
		DString_SetMBS( extraTuple->items[0]->xString.data, itname.toUtf8().data() );
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
		codeTuple->items[0]->xInteger.value = vmc0.code;
		codeTuple->items[1]->xInteger.value = vmc->a;
		codeTuple->items[2]->xInteger.value = vmc->b;
		codeTuple->items[3]->xInteger.value = vmc->c;
		codeTuple->items[4]->xInteger.value = vmc->line;
		DaoLexer_AnnotateCode( routine->body->source, *vmc, daoString, 32 );
		DString_SetMBS( codeTuple->items[5]->xString.data, daoString->mbs );
		DaoList_PushBack( codeList, (DaoValue*)codeTuple );
	}
}
