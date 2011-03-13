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

#ifndef _DAO_CONSOLE_H_
#define _DAO_CONSOLE_H_

#include<QRegExp>
#include<QPlainTextEdit>
#include<QTextCursor>
#include<QSyntaxHighlighter>
#include<QTextCharFormat>
#include<QThread>
#include<QMutex>
#include<QProcess>
#include<QWaitCondition>
#include<QLocalServer>
#include<QLocalSocket>

#include<daoEditor.h>

class Sleeper : public QThread
{
	public:
		static void Sleep( int us ){ QThread::usleep( us ); }
};

enum
{
	DVM_STATE_SUCCESSFUL ,
	DVM_STATE_CANCELLED ,
	DVM_STATE_ERROR
};

enum
{
	ID_LOGVIEW ,
	ID_DOCVIEW ,
	ID_EDITOR
};


enum DaoConsoleState
{
	DAOCON_READY ,
	DAOCON_RUN ,
	DAOCON_STDIN ,
	DAOCON_DEBUG ,
	DAOCON_SHELL
};

class DaoWordList;
class DaoWordPopup;

class DaoConsole : public DaoTextEdit
{
	Q_OBJECT

		DaoWordList wordList;

	QRegExp  rgxDaoFile;
	QRegExp  rgxShellCmd;
	QRegExp  rgxPrintRes;
	QRegExp  rgxHttpURL;
	QString  prompt;
	QString  script;
	QProcess shellProcess;
	int cursorBound;
	int outputBound;
	int clearScreenBound;
	int state;
	int count;
	int stdinCount;
	bool shellTop;
	bool shell;

	int idCommand;
	QStringList  commandHistory;

	QLocalServer  debugServer;
	QLocalServer  stdinServer;
	QLocalServer  stdoutServer;
	QLocalServer  loggerServer;

	DArray  *tokens;

	QList<int> promptBlocks;

	static bool appendToFile;
	static QStringList oldComdHist;

	void LoadCmdHistory();
	void SaveCmdHistory();
	void AppendCmdHistory( const QString &comd );
	void ClearCommand();

	public:
	DaoConsole( QWidget *parent=NULL );
	~DaoConsole();

	void SetColorScheme( int scheme );
	void PrintPrompt();
	void SetVmSpace( DaoVmSpace *vms );
	void SetState( int state ){ this->state = state; }
	void RunScript( const QString & src, bool debug=false );
	void LoadScript( const QString & file, bool debug=false );
	void Resume();
	void Stop();
	void Quit();

	DaoVmSpace   *vmSpace;
	DaoEditor	*editor;
	DaoContext   *context;
	DaoTabEditor *tabWidget;
	QLocalSocket *debugSocket;
	QLocalSocket *stdinSocket;
	QLocalSocket *stdoutSocket;
	QLocalSocket *loggerSocket;
	QLocalSocket  scriptSocket;
	QProcess	 *monitor;

protected:
	void AdjustCursor( QTextCursor *cursor, QTextCursor::MoveMode m );
	void keyPressEvent ( QKeyEvent * e );
	void focusInEvent ( QFocusEvent * event );
	void mousePressEvent ( QMouseEvent * event );
	int ShowClearActionMessage( bool extra=false );

public slots:
	void slotPrintOutput( const QString & );

protected slots:

	void slotBoundCursor();
	void slotScriptFinished();
	void slotProcessFinished( int,QProcess::ExitStatus );
	void slotSocketDebug();
	void slotSocketStdin();
	void slotSocketStdout();
	void slotSocketLogger();
	void slotReadStdOut();
	void slotReadStdError();
	void slotStdoutFromSocket();
	void slotLogFromSocket();
	void slotFoldUnfoldOutput();
	void slotSaveAll();
	void slotClearAll();
	void slotClearPart();

signals:
	void signalFocusIn();
	void signalLoadURL( const QString & );
};

#endif
