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

#ifndef _DAO_STUDIOMAIN_H_
#define _DAO_STUDIOMAIN_H_

#include<QLocalSocket>

#define TIME_YIELD 20
#define TIME_EVENT 5

#define CODE_ITEM_TYPE   "tuple<code:int,a:int,b:int,c:int,line:int,annot:string>"
#define VALUE_ITEM_TYPE  "tuple<name:string,type:string,value:string>"
#define EXTRA_ITEM_TYPE  "tuple<string,string,string>"

#define VALUE_INFO_TYPE \
"tuple<init:int,type:int,address:int,name:string,info:string,value:string," \
"numbers:array<int>|array<float>|array<float>|array<double>|array<complex>," \
"extras:list<tuple<string,string,string>>," \
"consts:list<tuple<name:string,type:string,value:string>>," \
"vars:list<tuple<name:string,type:string,value:string>>," \
"codes:list<tuple<code:int,a:int,b:int,c:int,line:int,annot:string>>," \
"entry:int,top:int>"

#define INDEX_INIT    0
#define INDEX_TYPE    1
#define INDEX_ADDR    2
#define INDEX_NAME    3
#define INDEX_INFO    4
#define INDEX_VALUE   5
#define INDEX_NUMBERS 6
#define INDEX_EXTRAS  7
#define INDEX_CONSTS  8
#define INDEX_VARS    9
#define INDEX_CODES  10
#define INDEX_ENTRY  11
#define INDEX_TOP    12

#define DATA_REQUEST_TYPE "tuple<object:string,table:int,row:int,column:int>"
#define INFO_TABLE 1
#define DATA_TABLE 2
#define CODE_TABLE 3

#define DAO_FRAME_ROUT (END_NOT_TYPES + 1)
#define DAO_FRAME_FUNC (END_NOT_TYPES + 2)

enum
{
	DAO_RUN_SCRIPT ,
	DAO_DEBUG_SCRIPT ,
	DAO_SET_PATH
};


class DaoStudioSettings
{
public:

	static QFont   codeFont;
	static QString locale;
	static QString program;
	static QString program_path;
	static QString socket_suffix;
	static QString socket_monitor;
	static QString socket_data;
	static QString socket_script;
	static QString socket_stdin;
	static QString socket_stdout;
	static QString socket_logger;
	static QString socket_debug;
	static QString socket_breakpoints;

	static QLocalSocket  monitor_socket;

	static void SetProgramPath( const char *cmd, const char *sock_suffix=NULL );
	static void AppendSuffix( const QString & suffix );
};


#endif
