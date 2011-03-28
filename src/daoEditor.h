/*=============================================================================
  This file is a part of Dao Studio
  Copyright (C) 2009-2011, Fu Limin. Email: limin.fu@yahoo.com, phoolimin@gmail.com

  Dao Studio is free software; you can redistribute it and/or modify it under the terms
  of the GNU General Public License as published by the Free Software Foundation;
  either version 2 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
  PARTICULAR PURPOSE. See the GNU General Public License for more details.
  ==============================================================================*/

#ifndef _DAO_EDITOR_H_
#define _DAO_EDITOR_H_

#include<QTabWidget>
#include<QPlainTextEdit>
#include<QSyntaxHighlighter>
#include<QTextBrowser>
#include<QComboBox>
#include<QLabel>
#include<QMenu>
#include<QAction>
#include<QTimer>
#include<QLineEdit>
#include<QFileSystemWatcher>

#include"daoCodeSHL.h"

#define DAO_DIRECT_API

extern "C"{ 
#include<dao.h>
#include<daoGC.h>
#include<daoArray.h>
#include<daoContext.h>
#include<daoProcess.h>
#include<daoRoutine.h>
#include<daoObject.h>
#include<daoNumtype.h>
#include<daoClass.h>
#include<daoParser.h>
#include<daoVmspace.h>
#include<daoNamespace.h>
#include<daoValue.h>
#include<daoRegex.h>
}

class DaoStudio;
class DaoTextEdit;

class DaoTabEditor : public QTabWidget
{
	Q_OBJECT
	public:
		DaoTabEditor( QWidget *parent ) : QTabWidget( parent ){};

	protected:
		void focusInEvent ( QFocusEvent * event );
		void mousePressEvent ( QMouseEvent * event );
signals:
		void signalFocusIn();
};

struct DaoWordNode
{
#if 0
	unsigned char  data;
	unsigned short count;
	DaoWordNode*   nexts[256];
#else
	QChar				   data;
	unsigned short		  count;
	QHash<QChar,DaoWordNode> nexts;
#endif

	DaoWordNode( const QString & word = "" );
	DaoWordNode( const QChar *chars, int n );
	~DaoWordNode();

	//void SetWord( const QString & word = "" );
};
struct DaoWordTree
{
};

class DaoWordList
{
	DArray  *tokens;
	DaoWordNode wordTree;
	QMap<QString,int>  wordList;
	int numComplete;
	int minLength;

	public:
	DaoWordList( int n=3, int m=4 );
	~DaoWordList(){ DArray_Delete( tokens ); }

	void Extract( const QString & source );
	void AddWord( const QString & word );
	void AddWords( const QList<QString> & words );
	QStringList GetWords( const QString & word );
};

class DaoWordPopup : public QWidget
{
	public:
		DaoWordPopup( DaoTextEdit *parent=NULL, DaoWordList *wl=NULL );

		DaoWordList	*wordList;
		DaoTextEdit	*editor;
		QList<QLabel*> labWords;
		bool isTip;

		bool IsTip()const{ return isTip; }
		void SetTip( const QString & tip );
		void SetWords( const QStringList & words );
		QString GetWord( int i );

		void TryCompleteWord( QKeyEvent *event, int xoffset );
		void EnsureVisible( int offset, bool exactpos=false );
};

enum KeyMode
{
	KEY_MODE_EDIT ,
	KEY_MODE_VIM ,
	KEY_MODE_UNKNOWN
};

class DaoEditWidget;

class DaoCommandEdit : public QLineEdit
{ Q_OBJECT

	DaoEditWidget *wgtParent;
	bool search;

	static void FilterHistory( QStringList & history, const QString prefix="" );

public:
	DaoCommandEdit( DaoEditWidget *parent = NULL );

	void SetMode( bool search=false ){ this->search = search; }

	static void FilterSearchHistory( const QString prefix="" );
	static void FilterCommandHistory( const QString prefix="" );

	static QStringList searchHistory;
	static QStringList commandHistory;
	static QStringList historyBrowsing;
	static int indexBrowsing;

protected:
	void keyPressEvent( QKeyEvent *event );

protected slots:
	void slotTextChanged(const QString &);
};

class DaoEditWidget : public QWidget
{
	// Q_OBJECT: if present, stylesheet will not work

	QLabel		 *labMessage;
	QLabel		 *labPrompt;
	DaoCommandEdit *cmdEdit;
	DaoTextEdit	*editor;

	public:
	DaoEditWidget( DaoTextEdit *parent=NULL );

	void StartSearch();
	void StartCommand();
	void EnsureVisible( bool select=true );
	void SetText( const QString & txt ){ cmdEdit->setText( txt ); }
	void HideMessage();
	void ShowMessage( const QString & txt );
	void AddMessage( const QString & txt );
	void SetPrompt( const QString & txt ){ labPrompt->setText( txt ); }
	QString GetPrompt()const{ return labPrompt->text(); }
	QString GetCommand()const{ return cmdEdit->text(); }
	static void AppendSearchHistory( const QString text="" );
	static void AppendCommandHistory( const QString text="" );

};
enum { BUF_EMPTY, BUF_BLOCKS, BUF_WORDS };

class DaoTextEdit : public QPlainTextEdit
{ Q_OBJECT

	protected:
		friend class DaoCursor;
		friend class DaoEditWidget;

		DaoCodeSHL  codehl;
		DaoWordList *wordList;
		DaoWordPopup *wgtWords;
		QComboBox *wgtEditMode;
		QLabel *labPopup;

		DaoEditWidget *cmdPrompt;
		QWidget *myCursor;
		QLabel  *labCursor;

		QTimer timer;

		QTextBlock blockEntry;
		QColor lineColor;
		int	 editMode; /* sepcified in wgtEditMode */
		KeyMode keyMode; /* key mode in use */
		QString keys;
		QString digits;

		DArray  *tokens;
		DString *mbs;
		DString *wcs;

		bool  showCursor;
		short tabWidth;

		QPoint menuPosition;
		QList<QAction*> extraMenuActions;

		void VimModeMove( int dir, int m );
		void VimModeCopyDelete( int dir, int m, bool copy=false );
		void VimModeMoveByWord( int dir, int m, bool token=false );
		void VimModeCopyDeleteByWord( int dir, int m, bool token=false, bool copy=false );
		void VimModeCommenting( int dir, int n, bool remove=false );

		static int buftype;
		static QString buffer;
		static QHash<QString,DaoRegex*> patterns;

	public:
		DaoTextEdit( QWidget *parent, DaoWordList *wlist );
		~DaoTextEdit();

		void Undo();
		void Redo();

		void RedoHighlight();
		void SetFontSize( int size );
		void SetFontFamily( const QString & family );
		void SetColorScheme( int scheme );
		void SetTabVisibility( int vid );
		void SetModeSelector( QComboBox *combo ){ wgtEditMode = combo; }
		void PreparePrinting( const QString &name );

		QFont GetFont()const{ return codehl.GetFont(); }
		void ExtractWords();

		void UpdateCursor( bool show );
		void HighlightFullLine( QTextCursor cursor );

		void HideMessage(){ labPopup->hide(); }
		void ShowMessage( const QString & msg );
		virtual int LeftMargin(){ return 0; }
		virtual void BeforeRedoHighlight(){}
		virtual void AfterRedoHighlight(){}

		DaoStudio	*studio;

	protected:
		void AdjustMove( QTextCursor *cursor, int key, QTextCursor::MoveMode m=QTextCursor::MoveAnchor );
		virtual void AdjustCursor( QTextCursor *cursor, QTextCursor::MoveMode m=QTextCursor::MoveAnchor ){};
		bool EditModeVIM( QKeyEvent * e );
		void keyPressEvent( QKeyEvent * e );
		void contextMenuEvent(QContextMenuEvent *event);

		int FindRangeIndex( const QTextBlock & block, int pos );
		int ForwardTokenEnd( int pos, int step );
		int BackwardTokenStart( int pos, int step );

		bool MoveSearchingCursor( QTextBlock block, int start=0 );
		void MoveSearchingCursor( int start=0 );

		void focusInEvent( QFocusEvent * event );
		void focusOutEvent( QFocusEvent * event );
		void scrollContentsBy( int dx, int dy );

		void JoinNextLine( QTextCursor cursor );
		void IndentLine( QTextCursor cursor, bool indent=true );
		void IndentLines( QTextCursor cursor, int n );
		DaoCodeLineData* SetIndentData( QTextBlock block );

		protected slots:
		void slotSearchOrReplace();
		void HighlightCurrentLine();
		void slotBlink();
};

class DaoNumbering;
class DaoLangLabels;
class DaoScriptEngine;

class DaoEditor : public DaoTextEdit
{ Q_OBJECT

	friend class DaoConsole;

	QFileSystemWatcher watcher;
	DaoTabEditor *tabWidget;
	DaoNumbering *wgtNumbering;
	DaoLangLabels *wgtLangLabels;

	QString name;
	QString fullName;
	QString textOnDisk;

	QStringList routCodes;
	QStringList newCodes;
	QList<int> lineMap;

	QMap<int,int> breakPoints;
	bool state;
	bool ready;

	int newEntryLine;

	public:
	DaoEditor( DaoTabEditor *parent, DaoWordList *wlist );

	void LoadFile( const QString & file );
	void Save( const QString & file = "" );
	bool TextChanged()const{ return state; }
	void SetExePoint( int entry );
	void SetState( bool state );
	void ResetBlockState();
	void SetEditLines( int start, int end );

	QList<int> BreakPoints(){ return breakPoints.keys(); }

	void BeforeRedoHighlight();
	void AfterRedoHighlight();

	bool EditContinue2();

	void SetFontSize( int size );
	void SetFontFamily( const QString & family );
	void SetColorScheme( int scheme );
	QString FileName(){ return name; }
	QString FilePath(){ return fullName; }
	void ChangeMark( int y );
	void MarkLine( int y );
	void PaintNumbering( QPaintEvent * event );
	void PaintQuickScroll( QPaintEvent * event );
	int NumberingWidth();

	QString GuessFileType( const QString & source );

	protected:
	void focusInEvent ( QFocusEvent * event );
	void keyPressEvent( QKeyEvent * e );
	void mousePressEvent ( QMouseEvent * event );
	void resizeEvent ( QResizeEvent * event );
	void scrollContentsBy( int dx, int dy );
	protected slots:
		void slotBoundEditor();
	void slotTextChanged();
	void UpdateNumberingWidth(int newBlockCount);
	void HighlightCurrentLine();
	void UpdateNumbering(const QRect &, int);
	void slotFileChanged( const QString & );
	void slotContentsChange( int, int, int );
	int LeftMargin(){ return NumberingWidth(); }
signals:
	void signalFocusIn();
	void signalTextChanged( bool );
};

class DaoNumbering : public QWidget
{
	DaoEditor *editor;

	public:
	DaoNumbering( DaoEditor *editor ) : QWidget( editor ){ this->editor = editor; }
	protected:
	void paintEvent ( QPaintEvent * event );
	void mouseDoubleClickEvent( QMouseEvent * event );
	void mousePressEvent ( QMouseEvent * event );
};

class DaoLangLabels : public QWidget
{
	public:
	DaoEditor *editor;
	bool entered;
	int zoom;
	int ymouse;
	int top;
	int bottom;

	DaoLangLabels( DaoEditor *editor ) : QWidget( editor ){
		entered = false;
		zoom = 0;
		top = bottom = 0;
		this->editor = editor;
	}
	protected:
	void paintEvent ( QPaintEvent * event );
	void mouseDoubleClickEvent( QMouseEvent * event );
	void mousePressEvent ( QMouseEvent * event );
	void mouseMoveEvent ( QMouseEvent * event );
	void enterEvent ( QEvent * event );
	void leaveEvent ( QEvent * event );
};


#endif
