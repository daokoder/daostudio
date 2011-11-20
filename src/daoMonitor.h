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

#ifndef _DAO_MONITOR_H_
#define _DAO_MONITOR_H_

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
#include<daoInterpreter.h>
#include<daoStudioMain.h>

class DaoMonitor;
class DaoStudio;

#if 0
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
#endif

extern "C"{

struct DaoEventHandler2
{
	void (*stdRead)( DaoEventHandler2 *self, DString *buf, int count );
	void (*stdWrite)( DaoEventHandler2 *self, DString *str );
	void (*stdFlush)( DaoEventHandler2 *self );
	void (*debug)( DaoEventHandler2 *self, DaoProcess *process );
	void (*breaks)( DaoEventHandler2 *self, DaoRoutine *breaks );
	void (*Called)( DaoEventHandler2 *self, DaoRoutine *caller, DaoRoutine *callee );
	void (*Returned)( DaoEventHandler2 *self, DaoRoutine *caller, DaoRoutine *callee );
	void (*InvokeHost)( DaoEventHandler2 *self, DaoProcess *process );
	DaoMonitor	 *monitor;
	DaoProcess   *process;
	QLocalSocket  socket;
	QLocalSocket  socket2;
	DaoDebugger	  debugger;
	DaoTimer      timer;
	unsigned int  time;
};

}

class DaoValueItem;

class DaoDataWidget : public QWidget, private Ui::DaoDataWidget
{
Q_OBJECT

	DaoCodeSHL *hlDataInfo;
	DaoCodeSHL *hlDataValue;

	DaoNamespace *mainNamespace;
	DaoNamespace *nameSpace;

	DaoValue      *currentValue;
	DaoStackFrame *currentFrame;

	DaoTuple *requestTuple;

	DArray	*tokens;
	DString *daoString;
	DLong	*daoLong;

	QLocalServer  monitorServer;
	QLocalSocket *monitorSocket;

	int vmcEntry;
	int vmcNewEntry;

	dint currentAddress;
	int currentType;
	int currentEntry;
	int currentTop;

	void EnableNoneTable();
	void EnableOneTable( DaoValue *p );
	void EnableTwoTable( DaoValue *p );
	void EnableThreeTable( DaoValue *p );

	// for DaoMap, whose values are not in a array
	QVector<DaoValue*> itemValues;

	QString itemName;

	void FillTable( QTableWidget *table, DaoList *list );
	void FillCodes( QTableWidget *table, DaoList *list );

	void RoutineInfo( DaoRoutine *routine, void *address );
	void FillTable( QTableWidget *table, DaoValue **data, int size, DArray *type, DMap *names, int fileter );
	void FillTable2( QTableWidget *table, DaoValue **data, int size, DArray *type, DMap *names );
	void ViewVmCodes( QTableWidget *table, DaoRoutine *routine );
	void ResetExecutionPoint(int row, int col);

	QString StringAddress( void *p ){ return "0x"+QString::number( (size_t) p, 16 ); }
	void SendDataRequest();

	public:

	DaoStudio  *studio;
	QTabWidget *scriptTab;

	DaoDataWidget( QWidget *parent = NULL );
	~DaoDataWidget();

	void SetNamespace( DaoNamespace *ns ){ mainNamespace = ns; }

	void ClearDataStack();
	void ViewArray( DaoTuple *tuple );
	void ViewList( DaoTuple *tuple );
	void ViewMap( DaoTuple *tuple );
	void ViewTuple( DaoTuple *tuple );
	void ViewNamespace( DaoTuple *tuple );
	void ViewProcess( DaoTuple *tuple );
	void ViewStackFrame( DaoTuple *tuple );

	void ViewValue( DaoValue *value );
	void ViewArray( DaoArray *array );
	void ViewList( DaoList *list );
	void ViewMap( DaoMap *map );
	void ViewTuple2( DaoTuple *tuple );
	void ViewClass( DaoClass *klass );
	void ViewObject( DaoObject *object );
	void ViewFunction( DaoFunction *function );
	void ViewRoutine( DaoRoutine *routine );
	void ViewStackFrame( DaoStackFrame *frame, DaoProcess *process );
	void ViewNamespace( DaoNamespace *nspace );
	void ViewProcess( DaoProcess *process, DaoStackFrame *frame = NULL );

protected:
	void keyPressEvent( QKeyEvent * e );
	void focusInEvent( QFocusEvent * event );

protected slots:
	void slotDataTableClicked(int, int);
	void slotInfoTableClicked(int, int);
	void slotExtraTableClicked(int, int);
	void slotElementChanged(int, int);
	void slotUpdateValue();

	void slotUpdateMonitor();
	void slotValueActivated(QListWidgetItem*);

signals:
	void signalViewElement( DaoValueItem *, DaoValue * );
	void signalFocusIn();
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

class DaoMonitor : public QMainWindow, private Ui::DaoMonitor
{
Q_OBJECT

	QLocalServer   server;
	DaoDataWidget *dataWidget;

	QString	locale;
	QString	program;
	QString	programPath;

	QProcess *shell;

	QLineEdit	*wgtFind;
	QLineEdit	*wgtReplace;
	QCheckBox	*chkReplaceAll;
	QCheckBox	*chkCaseSensitive;
	QComboBox	*wgtFontSize;
	QComboBox	*wgtEditorColor;

	QString		 script;
	QString		 pathWorking;
	QString		 pathImage;

	DaoEventHandler2	handler;
	QLocalSocket	*scriptSocket;
	QTimer timer;

	DaoValue *itemValues;
	DString  *daoString;
	DLong    *daoLong;
	DArray   *tokens;
	int itemCount;
	int vmcEntry;
	int vmcNewEntry;
	int index;
	int vmState;

public:
	DaoMonitor( const char *program=NULL );
	~DaoMonitor();

	bool waiting;
	unsigned int time;
	QMutex  mutex;
	DaoProcess  *debugProcess;
	DaoVmSpace  *vmSpace;

	void SetPathWorking( const QString & path );

	void ViewValue( DaoDataWidget *dataView, DaoValueItem *it );
	void ReduceValueItems( QListWidgetItem *item );
	void EraseDebuggingProcess();
	void InitDataBrowser();

protected:
	void closeEvent ( QCloseEvent *e );

public slots:
	void slotExitWaiting();
	void slotFlushStdout();
	void slotReadStdOut();
	void slotShellFinished(int, QProcess::ExitStatus);
protected slots:
	void slotStartExecution();
	void slotStopExecution();
	void slotValueActivated(QListWidgetItem*);

signals:

};


#endif
