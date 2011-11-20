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

#ifndef _DAO_INTERPRETER_H_
#define _DAO_INTERPRETER_H_

#include"ui_daoMonitor.h"
#include"ui_daoAbout.h"
#include"ui_daoDataWidget.h"

#include<QMainWindow>
#include<QTime>
#include<QTimer>
#include<QLabel>
#include<QStack>
#include<QProgressBar>
#include<QRegExp>
#include<QScrollArea>
#include<QTextBrowser>
#include<QFileSystemWatcher>
#include<QCheckBox>
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

struct DaoEventHandler
{
	void (*stdRead)( DaoEventHandler *self, DString *buf, int count );
	void (*stdWrite)( DaoEventHandler *self, DString *str );
	void (*stdFlush)( DaoEventHandler *self );
	void (*debug)( DaoEventHandler *self, DaoProcess *process );
	void (*breaks)( DaoEventHandler *self, DaoRoutine *breaks );
	void (*Called)( DaoEventHandler *self, DaoRoutine *caller, DaoRoutine *callee );
	void (*Returned)( DaoEventHandler *self, DaoRoutine *caller, DaoRoutine *callee );
	void (*InvokeHost)( DaoEventHandler *self, DaoProcess *process );
	DaoInterpreter	*interpreter;
	DaoProcess   *process;
	QLocalSocket  socket;
	QLocalSocket  socket2;
	DaoDebugger	  debugger;
	DaoTimer      timer;
	unsigned int  time;
};

}

class DaoValueItem;

#if 0
class DaoDataWidget : public QWidget, private Ui::DaoDataWidget
{
Q_OBJECT

	DaoCodeSHL *hlDataInfo;
	DaoCodeSHL *hlDataValue;

	DaoNamespace *nameSpace;

	DaoValue      *currentValue;
	DaoStackFrame *currentFrame;

	DArray	*tokens;
	DString *daoString;
	DLong	*daoLong;

	int vmcEntry;
	int vmcNewEntry;

	void EnableNoneTable();
	void EnableOneTable( DaoValue *p );
	void EnableTwoTable( DaoValue *p );
	void EnableThreeTable( DaoValue *p );

	// for DaoMap, whose values are not in a array
	QVector<DaoValue*> itemValues;

	QListWidget *wgtDataList;
	QString itemName;

	void RoutineInfo( DaoRoutine *routine, void *address );
	void FillTable( QTableWidget *table, DaoValue **data, int size, DArray *type, DMap *names, int fileter );
	void FillTable2( QTableWidget *table, DaoValue **data, int size, DArray *type, DMap *names );
	void ViewVmCodes( QTableWidget *table, DaoRoutine *routine );
	void ResetExecutionPoint(int row, int col);

	QString StringAddress( void *p ){ return "0x"+QString::number( (size_t) p, 16 ); }

	public:

	DaoDataWidget();
	~DaoDataWidget();

	void ViewValue( DaoValue *value );
	void ViewArray( DaoArray *array );
	void ViewList( DaoList *list );
	void ViewMap( DaoMap *map );
	void ViewTuple( DaoTuple *tuple );
	void ViewClass( DaoClass *klass );
	void ViewObject( DaoObject *object );
	void ViewFunction( DaoFunction *function );
	void ViewRoutine( DaoRoutine *routine );
	void ViewStackFrame( DaoStackFrame *frame, DaoProcess *process );
	void ViewNamespace( DaoNamespace *nspace );
	void ViewProcess( DaoProcess *process, DaoStackFrame *frame = NULL );

	void SetDataList( QListWidget *list ){ wgtDataList = list; }

protected slots:
	void slotDataTableClicked(int, int);
	void slotInfoTableClicked(int, int);
	void slotExtraTableClicked(int, int);
	void slotElementChanged(int, int);
	void slotUpdateValue();

signals:
	void signalViewElement( DaoValueItem *, DaoValue * );
};

class DaoValueItem : public QListWidgetItem
{
	public:
	DaoValueItem( DaoValue *value, QListWidget *parent=0 );

	short type;
	union {
		DaoValue     *value;
		DaoArray     *array; // = DAO_ARRAY
		DaoList      *list; // = DAO_LIST
		DaoMap       *map;	// = DAO_MAP
		DaoTuple     *tuple; // = DAO_TUPLE
		DaoClass     *klass; // = DAO_CLASS
		DaoObject    *object; // = DAO_OBJECT
		DaoRoutine	 *routine;
		DaoNamespace *nspace;
		DaoProcess   *process;
	};
	DaoStackFrame *frame;

	DaoValueItem  *parent;
	DaoDataWidget *dataWidget;
};
#endif

class DaoInterpreter : public QObject
{
Q_OBJECT

	QString	locale;
	QString	program;
	QString	programPath;

	QProcess *shell;

	QString  script;
	QString  pathWorking;
	QString  pathImage;

	DaoEventHandler	handler;
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
	DArray   *extraStack;

	DaoValue *itemValues;
	DString  *daoString;
	DLong    *daoLong;
	DArray   *tokens;
	int itemCount;
	int vmcEntry;
	int vmcNewEntry;
	int index;
	int vmState;

	QString StringAddress( void *p ){ return "0x"+QString::number( (size_t) p, 16 ); }

	void InitMessage( DaoValue *value );
	void ViewValue( DaoValue *value );
	void ViewArray( DaoArray *array );
	void ViewList( DaoList *list );
	void ViewMap( DaoMap *map );
	void ViewTuple( DaoTuple *tuple );
	void ViewNamespace( DaoNamespace *nspace );
	void ViewProcess( DaoProcess *process );
	void ViewStackFrame( DaoStackFrame *frame, DaoProcess *process );

	void MakeList( DaoList *list, DaoValue **data, int size, DArray *type, DMap *names, int filter );
	void ViewVmCodes( DaoList *list, DaoRoutine *routine );
	QString RoutineInfo( DaoRoutine *routine, void *address );

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

	//void ViewValue( DaoDataWidget *dataView, DaoValueItem *it );
	//void ReduceValueItems( QListWidgetItem *item );
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
	//void slotValueActivated(QListWidgetItem*);

signals:

};


#endif
