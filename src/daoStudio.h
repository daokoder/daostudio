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

#ifndef _DAO_STUDIO_H_
#define _DAO_STUDIO_H_

#include"ui_daoStudio.h"
#include"ui_daoAbout.h"
#include"ui_daoHelpVIM.h"

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
#include<QLocalSocket>

#include<daoEditor.h>

class DaoTextBrowser : public QTextBrowser
{
	Q_OBJECT

public:
	DaoTextBrowser( QWidget *parent );
	
	QVariant loadResource ( int type, const QUrl & name );
	
	DaoStudio *studio;
	QTabWidget *tabWidget;
	
protected:
	void mousePressEvent( QMouseEvent * event );
	void keyPressEvent( QKeyEvent * e );
	signals:
	void signalFocusIn();
};

class DaoLogBrowser : public DaoTextBrowser
{
public:
	DaoLogBrowser( QWidget *parent ) : DaoTextBrowser( parent ){}
};

class DaoDocViewer : public DaoTextBrowser
{
	QString docRoot;
	QString docLang;

public:
	DaoDocViewer( QWidget *parent, const QString & path="" );

	void setLanguage( const QString & lang );
};

class DaoStudio : public QMainWindow, private Ui::DaoStudio
{
	Q_OBJECT
	
	QLocalSocket   socket;
	QProcess	  *monitor;
	
	QTime  time;
	QTimer timer;
	QLabel *labTimer;
	
	DaoDocViewer  *docViewer;
	
	DaoWordList wordList;
	
	QHash<QString,int> pathUsage;
	QHash<QString,QVariant> lastCursor;
	
	QString  locale;
	QString  program;
	QString  programPath;
	
	QLineEdit  *wgtFind;
	QLineEdit  *wgtReplace;
	QCheckBox  *chkReplaceAll;
	QCheckBox  *chkCaseSensitive;
	
	QString	 pathWorking;
	QString	 pathBrowsing;
	
	QStringList fileFilters;
	
	DaoVmSpace *vmSpace;
	
	int vmState;
	int configured;

	QLocalServer  pathServer;
	
	QFileSystemWatcher watcher;
	
	void LoadPath( QListWidget *wgt, const QString &path, const QString &suffix );
	
	/*
	   void UpdateCombobox();
	   void UpdateTable();
	   void ShowWarning( const QString & message );
	 */
public:
	DaoStudio( const char *program=NULL );
	~DaoStudio();
	
	void SetPathWorking( const QString & path );
	void SendPathWorking();
	DaoEditor* NewEditor( const QString & name, const QString & tip="" );
	
	int  GetState()const{ return vmState; }
	void SetState( int state );
	void ResetTimer(){ time.restart(); }
	void RestartMonitor();
	
	DaoConsole* Console(){ return wgtConsole; }
	QString GetWorkingPath(){ return pathWorking; }
	QString GetBrowsingPath(){ return pathBrowsing; }
	
	void LoadSettings();
	void SaveSettings();
	
protected:
	void closeEvent ( QCloseEvent *e );
	void showEvent ( QShowEvent * event );
	
	public slots:
	void SetPathBrowsing( const QString & path );
	void slotWriteLog( const QString & );
	void slotSetPathWorking();
	void slotSetPathBrowsing();
	void slotSetPathWorking2();
	void slotMaxEditor( int id = 0 );
	void slotMaxMonitor( int id = 0 );
	void slotMaxConsole( int id = 0 );
	void slotSave();
	void slotSaveAs();
	protected slots:
	void slotRestartMonitor2();
	void slotRestartMonitor( int exitCode, QProcess::ExitStatus exitStatus );
	void slotLoadURL( const QString & );
	void slotFileActivated(QListWidgetItem*);
	void slotViewFrame(QListWidgetItem*);
	void slotViewDocument(QListWidgetItem*);
	void slotCloseEditor(int);
	void slotTextChanged(bool);
	void slotPathList(int);
	void slotTips();
	void slotDocs();
	void slotAbout();
	void slotNew();
	void slotOpen();
	void slotPrint();
	void slotUndo();
	void slotRedo();
	void slotCopy();
	void slotCut();
	void slotPaste();
	void slotFind();
	void slotReplace();
	void slotRestartMonitor();
	void slotStart();
	void slotDebug();
	void slotStop();
	void slotCompile();
	void slotDebugger(bool);
	void slotProfiler(bool);
	void slotMSplit(bool);
	void slotFontSize(int);
	void slotFileSuffix(int);
	void slotFontFamily(const QFont&);
	void slotEditorColor(int);
	void slotConsoleColor(int);
	void slotTabVisibility(int);
	void slotSwitchDebugger(int);
	void slotSwitchProfiler(int);
	void slotTimeOut();
	void slotHelpVIM();
	
	signals:
	
};

class DaoStudioAbout : public QDialog, private Ui::DaoStudioAbout
{
	Q_OBJECT
	
public:
	DaoStudioAbout( QWidget *parent=NULL );
};
class DaoHelpVIM : public QDialog, private Ui::DaoHelpVIM
{
	Q_OBJECT
	
public:
	DaoHelpVIM( QWidget *parent=NULL );
};
#endif
