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
#include<QThread>
#include<QProcess>

#include<daoCodeSHL.h>
#include<daoDebugger.h>
#include<daoStudioMain.h>

class DaoMonitor;
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
	void (*debug)( DaoEventHandler *self, DaoContext *context );
	void (*breaks)( DaoEventHandler *self, DaoRoutine *breaks );
	void (*Called)( DaoEventHandler *self, DaoRoutine *caller, DaoRoutine *callee );
	void (*Returned)( DaoEventHandler *self, DaoRoutine *caller, DaoRoutine *callee );
	void (*InvokeHost)( DaoEventHandler *self, DaoContext *context );
	DaoMonitor   *monitor;
  DaoVmProcess *process;
  QLocalSocket  socket;
  DaoDebugger   debugger;
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

	DaoNameSpace *nameSpace;

	DaoBase *currentObject;
	DValue  *currentValue;

	DArray  *tokens;
	DString *daoString;
	DLong   *daoLong;

	int vmcEntry;
	int vmcNewEntry;

	void EnableNoneTable();
	void EnableOneTable( DaoBase *p );
	void EnableTwoTable( DaoBase *p );
	void EnableThreeTable( DaoBase *p );

	// for DaoMap, whose values are not in a array
	QVector<DValue*> itemValues;

	QListWidget *wgtDataList;
	QString itemName;

	void RoutineInfo( DaoRoutine *routine, void *address );
	void FillTable( QTableWidget *table, DValue **data, int size, DArray *type, DMap *names, int fileter );
	void FillTable2( QTableWidget *table, DValue **data, int size, DArray *type, DMap *names );
	void ViewVmCodes( QTableWidget *table, DaoRoutine *routine );
	void ResetExecutionPoint(int row, int col);

	QString StringAddress( void *p ){ return "0x"+QString::number( (size_t) p, 16 ); }

	public:

	DaoDataWidget();
	~DaoDataWidget();

	void ViewValue( DValue *value );
	void ViewArray( DaoArray *array );
	void ViewList( DaoList *list );
	void ViewMap( DaoMap *map );
	void ViewTuple( DaoTuple *tuple );
	void ViewClass( DaoClass *klass );
	void ViewObject( DaoObject *object );
	void ViewFunction( DaoFunction *function );
	void ViewRoutine( DaoRoutine *routine );
	void ViewContext( DaoContext *context );
	void ViewNameSpace( DaoNameSpace *nspace );
	void ViewProcess( DaoVmProcess *process );

	void SetDataList( QListWidget *list ){ wgtDataList = list; }

protected slots:
	void slotDataTableClicked(int, int);
	void slotInfoTableClicked(int, int);
	void slotExtraTableClicked(int, int);
	void slotElementChanged(int, int);
	void slotUpdateValue();

signals:
	void signalViewElement( DaoValueItem *, DValue * );
	void signalViewElement( DaoValueItem *, DaoBase * );
};

union DValueRef
{
	DValue       *value; // = 0
	DaoBase      *base;
	DaoArray     *array; // = DAO_ARRAY
	DaoList      *list; // = DAO_LIST
	DaoMap       *map;  // = DAO_MAP
	DaoTuple     *tuple; // = DAO_TUPLE
	DaoClass     *klass; // = DAO_CLASS
	DaoObject    *object; // = DAO_OBJECT
	DaoRoutine   *routine;
	DaoContext   *context;
	DaoNameSpace *nspace;
	DaoVmProcess *process;
};
class DaoValueItem : public QListWidgetItem
{
	public:
	DaoValueItem( DValue *value, QListWidget *parent=0 );
	DaoValueItem( DaoBase *value, QListWidget *parent=0 );

	short type;
	union {
		DValue       *value; // = 0
		DaoBase      *base;
		DaoArray     *array; // = DAO_ARRAY
		DaoList      *list; // = DAO_LIST
		DaoMap       *map;  // = DAO_MAP
		DaoTuple     *tuple; // = DAO_TUPLE
		DaoClass     *klass; // = DAO_CLASS
		DaoObject    *object; // = DAO_OBJECT
		DaoRoutine   *routine;
		DaoContext   *context;
		DaoNameSpace *nspace;
		DaoVmProcess *process;
	};

	DaoValueItem  *parent;
	DaoDataWidget *dataWidget;
};

class DaoMonitor : public QMainWindow, private Ui::DaoMonitor
{
Q_OBJECT

  QLocalServer   server;
  QLocalServer   server2;
	DaoDataWidget *dataWidget;

	QString  locale;
	QString  program;
	QString  programPath;

  QProcess *shell;

	QLineEdit  *wgtFind;
	QLineEdit  *wgtReplace;
	QCheckBox  *chkReplaceAll;
	QCheckBox  *chkCaseSensitive;
	QComboBox  *wgtFontSize;
	QComboBox  *wgtEditorColor;

	QString     script;
	QString     pathWorking;
	QString     pathImage;
	DaoVmProcess *debugProcess;

	DaoEventHandler  handler;
  QLocalSocket  *scriptSocket;
  QTimer timer;

	QStack<DValue*> viewValues;
	QVector<DValue*> itemValues2;
	DString *daoString;
	DLong   *daoLong;
	DValue  *itemValues;
	DArray  *tokens;
	int itemCount;
	int vmcEntry;
	int vmcNewEntry;
	int index;
	int vmState;

public:
	DaoMonitor( const char *program=NULL );
	~DaoMonitor();

  bool          waiting;
  unsigned int  time;
	DaoContext   *viewContext;
	DaoVmSpace   *vmSpace;

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
