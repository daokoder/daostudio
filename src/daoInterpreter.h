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

#ifndef _DAO_INTERPRETER_H_
#define _DAO_INTERPRETER_H_

#include<QTime>
#include<QTimer>
#include<QLabel>
#include<QStack>
#include<QLocalServer>
#include<QLocalSocket>
#include<QMutex>
#include<QThread>
#include<QProcess>

#include<daoCodeSHL.h>
#include<daoDebugger.h>
#include<daoStudioMain.h>

class DaoInterpreter;

class DaoTimer : public QThread
{
	public:
	DaoTimer(){ time = 0; }

	unsigned int time;

	protected:
	void run(){
		while(1){
			msleep( TIME_YIELD );
			time += 1;
		}
	}
};

extern "C"{

struct DaoConsoleStream
{
	DaoStream *stream;
	void (*stdRead)( DaoConsoleStream *self, DString *buf, int count );
	void (*stdWrite)( DaoConsoleStream *self, DString *str );
	void (*stdFlush)( DaoConsoleStream *self );
	void (*SetColor)( DaoConsoleStream *self, const char *fgcolor, const char *bgcolor );
	DaoInterpreter	*interpreter;
	DaoProcess   *process;
	QLocalSocket  socket;
	QLocalSocket  socket2;
	DaoxDebugger *debugger;
};
struct DaoVmDebugger
{
	void (*debug)( DaoVmDebugger *self, DaoProcess *process, DaoStream *stream );
	void (*breaks)( DaoVmDebugger *self, DaoRoutine *breaks );
	DaoInterpreter	*interpreter;
	DaoProcess   *process;
	QLocalSocket  socket;
	DaoxDebugger *debugger;
};

struct DaoVmUserHandler
{
	void (*InvokeHost)( DaoVmUserHandler *self, DaoProcess *process );
	DaoProcess   *process;
	DaoTimer      timer;
	unsigned int  time;
};

}


class DaoInterpreter : public QObject
{
Q_OBJECT

	QString	locale;
	QString	program;
	QString	programPath;
	QString daoBinPath;

	QProcess *shell;

	QString  script;
	QString  pathWorking;
	QString  pathImage;

	DaoxDebugger   debugger;
	DaoVmDebugger  guiDebugger;
	DaoVmDebugger  cmdDebugger;
	DaoVmUserHandler handler;
	DaoxProfiler *profiler;
	DaoConsoleStream stdioStream;
	DaoConsoleStream errorStream;
	QLocalServer  dataServer;
	QLocalServer  scriptServer;
	QLocalSocket *dataSocket;
	QLocalSocket *scriptSocket;
	QLocalSocket  monitorSocket;
	QTimer timer;

	DaoType  *codeType;
	DaoType  *valueType;
	DaoType  *extraType;
	DaoType  *messageType;
	DaoTuple *codeTuple;
	DaoTuple *valueTuple;
	DaoTuple *extraTuple;
	DaoTuple *messageTuple;
	DaoArray *numArray;
	DaoList  *extraList;
	DaoList  *constList;
	DaoList  *varList;
	DaoList  *codeList;

	DaoList  *valueStack;
	DList   *extraStack;

	DString  *daoString;
	DList   *tokens;
	int vmState;

	QString StringAddress( void *p ){ return "0x"+QString::number( (size_t) p, 16 ); }

	void InitMessage( DaoValue *value );
	void ViewValue( DaoValue *value );
	void ViewArray( DaoArray *array );
	void ViewList( DaoList *list );
	void ViewMap( DaoMap *map );
	void ViewTuple( DaoTuple *tuple );
	void ViewRoutine( DaoRoutine *routine );
	void ViewRoutines( DaoRoutine *routines );
	void ViewClass( DaoClass *klass );
	void ViewObject( DaoObject *object );
	void ViewNamespace( DaoNamespace *nspace );
	void ViewProcess( DaoProcess *process );
	void ViewStackFrame( DaoStackFrame *frame, DaoProcess *process );

	void MakeList( DaoList *list, DaoValue **data, int size, DList *type, DMap *names, int filter );
	void ViewVmCodes( DaoList *list, DaoRoutine *routine );
	QString RoutineInfo( DaoRoutine *routine, void *address );

	void ViewTupleData( DaoTuple *tuple, DaoTuple *request );
	void ViewListData( DaoList *list, DaoTuple *request );
	void ViewMapData( DaoMap *map, DaoTuple *request );
	void ViewRoutineData( DaoRoutine *routine, DaoTuple *request );
	void ViewRoutinesData( DaoRoutine *routines, DaoTuple *request );
	void ViewClassData( DaoClass *klass, DaoTuple *request );
	void ViewObjectData( DaoObject *object, DaoTuple *request );
	void ViewNamespaceData( DaoNamespace *ns, DaoTuple *request );
	void ViewProcessStack( DaoProcess *proc, DaoTuple *request );
	void ViewStackData( DaoProcess *proc, DaoStackFrame *frame, DaoTuple *request );

public:
	DaoInterpreter( const char *program=NULL );
	~DaoInterpreter();

	bool waiting;
	unsigned int time;
	QMutex  mutex;
	DaoProcess  *debugProcess;
	DaoVmSpace  *vmSpace;
	DaoNamespace *nameSpace;

	void SetPathWorking( const QString & path );

	void EraseDebuggingProcess();
	void InitDataBrowser();
	void SendDataInformation();

protected:

public slots:
	void slotExitWaiting();
	void slotFlushStdout();
	void slotReadStdOut();
	void slotShellFinished(int, QProcess::ExitStatus);
protected slots:
	void slotServeData();
	void slotStartExecution();
	void slotStopExecution();

signals:

};


#endif
