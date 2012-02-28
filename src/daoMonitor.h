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

#include<QScrollArea>
#include<QTextBrowser>
#include<QLocalServer>
#include<QLocalSocket>

#include<daoCodeSHL.h>
#include<daoDebugger.h>
#include<daoInterpreter.h>
#include<daoStudioMain.h>

class DaoStudio;

class DaoMonitor : public QWidget, private Ui::DaoMonitor
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

	daoint currentAddress;
	int currentType;
	int currentEntry;
	int currentTop;

	void EnableNoneTable();
	void EnableOneTable( DaoValue *p );
	void EnableTwoTable( DaoValue *p );
	void EnableThreeTable( DaoValue *p );

	void FillTable( QTableWidget *table, DaoList *list );
	void FillCodes( QTableWidget *table, DaoList *list );

	void SendDataRequest();

	public:

	DaoStudio  *studio;

	DaoMonitor( QWidget *parent = NULL );
	~DaoMonitor();

	void SetNamespace( DaoNamespace *ns ){ mainNamespace = ns; }

	void ClearDataStack();
	void ViewArray( DaoTuple *tuple );
	void ViewList( DaoTuple *tuple );
	void ViewMap( DaoTuple *tuple );
	void ViewTuple( DaoTuple *tuple );
	void ViewRoutine( DaoTuple *tuple );
	void ViewFunction( DaoTuple *tuple );
	void ViewFunctree( DaoTuple *tuple );
	void ViewClass( DaoTuple *tuple );
	void ViewObject( DaoTuple *tuple );
	void ViewNamespace( DaoTuple *tuple );
	void ViewProcess( DaoTuple *tuple );
	void ViewStackFrame( DaoTuple *tuple );

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
	void signalFocusIn();
};

#endif
