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

#ifndef _DAO_CODESHL_H_
#define _DAO_CODESHL_H_

#include<QTabWidget>
#include<QPlainTextEdit>
#include<QSyntaxHighlighter>
#include<QTextBrowser>
#include<QComboBox>
#include<QLabel>
#include<QMenu>
#include<QAction>
#include<QFileSystemWatcher>

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
#include<daoMap.h>
#include<daoRegex.h>
#include<daoSched.h>
}

#include"daoStudioMain.h"

#define ZOOM_FONT 2

enum DaoCodeSHLState
{
	DAO_HLSTATE_NONE ,
	DAO_HLSTATE_PROMPT ,
	DAO_HLSTATE_NORMAL ,
	DAO_HLSTATE_MBS ,
	DAO_HLSTATE_WCS ,
	DAO_HLSTATE_COMMENT ,
	DAO_HLSTATE_CMT_OPEN1 ,
	DAO_HLSTATE_CMT_OPEN2 ,
	DAO_HLSTATE_REDO
};

enum CodeLineStates
{
	CLS_NORMAL ,
	CLS_CHANGED ,
	CLS_READONLY ,
	CLS_COMMAND ,
	CLS_OUTPUT
};
enum DaoSyntaxIndentMode1
{
	DS_IDT_NEXT_SAME ,
	DS_IDT_NEXT_BACK ,
	DS_IDT_NEXT_MORE ,
	DS_IDT_NEXT_MORE1 ,
	DS_IDT_NEXT_MORE2 ,
	DS_IDT_THIS_SAME ,
	DS_IDT_THIS_ZERO ,
	DS_IDT_THIS_BACK ,
	DS_IDT_ALL_BACK 
};
enum DaoSyntaxIndentState
{
	DS_IDT_SAME ,
	DS_IDT_ZERO ,
	DS_IDT_BACK ,
	DS_IDT_MORE
};

class DaoCodeLineData : public QTextBlockUserData
{
	public:
		DaoCodeLineData( bool b, enum CodeLineStates s=CLS_NORMAL, int l=0 ){
			state = s; 
			line = l; 
			breaking = b;
			rehighlight = true;
			def_class = def_method = false;
			leadTabs = leadSpaces = 0;
			firstToken = 0;
			incomplete = false;
			extend_line = false;
			open_token = false;
			reference_line = true;
			token_line = false;
			brace_count = 0;
			font_size = 0;
			font_size2 = 0;
		}

		unsigned short line;
		unsigned char  leadTabs; /* count */
		unsigned char  leadSpaces; /* count */
		unsigned char  firstToken; /* token type */
		unsigned char  state;
		unsigned char  font_size;
		unsigned char  font_size2;

		bool   rehighlight;
		bool   breaking;
		bool   def_class;
		bool   def_method;
		bool   incomplete; // incomplete line.
		bool   extend_line; // follow up line of a incomplete line.
		bool   open_token; // incomplete string or comment token
		bool   reference_line; // line as indentation reference for following up lines.
		bool   token_line; // line that is a part of an open token;
		short  brace_count;
};

struct DaoSyntaxPattern
{
	DaoRegex *pattern;
	short	 groups;
	short	 color;
};

struct DaoBasicSyntax
{
	QString  language;

	DMap	*keyStruct;
	DMap	*keyStorage;
	DMap	*keyStatement;
	DMap	*keyConstant;
	DMap	*keyOthers;
	DString *cmtLine1;
	DString *cmtLine2;
	DString *cmtOpen1;
	DString *cmtOpen2;
	DString *cmtClose1;
	DString *cmtClose2;
	DString *mbs;
	DString *wcs;
	bool hasDaoLineComment;
	bool hasDaoBlockComment;
	bool singleQuotation;
	bool doubleQuotation;
	bool shortQuotation;
	bool caseInsensitive;
	bool isLatex;

	DArray *tokens;

	DaoRegex *func_regex;
	DaoRegex *class_regex;
	QList<DaoSyntaxPattern> patterns;

	QList<DaoRegex*> noneIndents; // TODO delete
	QList<DaoRegex*> lessIndents;
	QList<DaoRegex*> moreIndents;
	QList<DaoRegex*> moreIndents2;
	
	static DaoBasicSyntax *dao;
	static DaoBasicSyntax *python;

	DaoBasicSyntax( const QString & lang );
	~DaoBasicSyntax();

	bool IndentNone( const QString & codes );
	bool IndentLess( const QString & codes );
	bool IndentMore( const QString & codes );
	bool IndentMore2( const QString & codes );

	void AddKeywordStruct( const char *keyword );
	void AddKeywordStorage( const char *keyword );
	void AddKeywordStatement( const char *keyword );
	void AddKeywordConstant( const char *keyword );
	void AddKeywordOthers( const char *keyword );
	void AddSingleLineComment( const char *tok );
	void AddMultiLineComment( const char *open, const char *close );

	void AddPattern( const char *pat, int group, int color );
	void SetClassPattern( const char *pat );
	void SetMethodPattern( const char *pat );

	void AddNoneIndentPattern( const char *pat );
	void AddLessIndentPattern( const char *pat );
	void AddMoreIndentPattern( const char *pat );
	void AddMoreIndentPattern2( const char *pat );

	/* Note: the token field "name" will not be valid! */
	int Tokenize( DArray *tokens, const char *source );
};
struct DaoLanguages
{
	DaoLanguages();
};

class DaoTokenFormat : public QTextCharFormat
{
	public:
		DaoTokenFormat();
};

enum DaoTextCharType
{
	TXT_CHAR_OUTPUT = 0x1000 , // required by Qt
	TXT_CHAR_PROMPT ,
	TXT_CHAR_COMMENT ,
	TXT_CHAR_CONSTANT ,
	TXT_CHAR_IDENTIFIER ,
	TXT_CHAR_SYMBOL 
};

class DaoCodeSHL : public QSyntaxHighlighter
{ Q_OBJECT
	friend class DaoConsole;
	friend class DaoTextEdit;
	
	DaoTokenFormat formatOutput;
	DaoTokenFormat formatPrompt;
	DaoTokenFormat formatComment;
	DaoTokenFormat formatStorage;
	DaoTokenFormat formatConstant;
	DaoTokenFormat formatTypeStruct;
	DaoTokenFormat formatStmtKey;
	DaoTokenFormat formatStdobj;
	DaoTokenFormat formatBracket;
	DaoTokenFormat formatSBracket;
	DaoTokenFormat formatCBracket;
	DaoTokenFormat formatInitype; // @T
	DaoTokenFormat formatSymbol; // $E
	
	QColor plainColor;
	QColor tabColor[2];
	QColor searchColor;
	
	QString oldBlock;
	int oldBlockStart;
	short toktype;
	short entered;
	short hlstate;
	short scheme;
	short tabVisibility;
	short fontSize;
	int textSkip;
	DArray  *tokens;
	DArray  *toks;
	DString *mbs;
	DString *wcs;
	QString cmt;

	QHash<QString,bool> words;

	DaoRegex *pattern;
	DaoBasicSyntax *language;

	public:
	DaoCodeSHL( QTextDocument * parent );
	~DaoCodeSHL();

	QFont GetFont()const{ return formatOutput.font(); }
	void SetSkip( int skip ){ textSkip = skip; }
	void SetState( int state = -1 );
	void SetFontSize( int size );
	void SetFontFamily( const QString & family );
	void SetColorScheme( int scheme );
	void SetTabVisibility( int vid );

	DaoTokenFormat ColorGroup( int group );

	QString SetLanguage( const QString &lang );
	DaoRegex* SetPattern( DaoRegex *pat );

	QList<QString> GetWords(){ return words.keys(); }

	static QMap<QString,DaoBasicSyntax*> languages;

	//protected:
	void SetIndentationData( DaoCodeLineData *ud, DArray *tokens );
	void HighlightSearch( const QString & text );
	void HighlightNormal( const QString & text );
	void highlightBlock( const QString & text );
	//void SetCharType( int pos, int count, int type );

signals:
	void signalOutlineChanged();
};

#endif
