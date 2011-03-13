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

#define TIME_YIELD 20
#define TIME_EVENT 5

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
