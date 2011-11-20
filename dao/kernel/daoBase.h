/*=========================================================================================
  This file is a part of a virtual machine for the Dao programming language.
  Copyright (C) 2006-2011, Fu Limin. Email: fu@daovm.net, limin.fu@yahoo.com

  This software is free software; you can redistribute it and/or modify it under the terms 
  of the GNU Lesser General Public License as published by the Free Software Foundation; 
  either version 2.1 of the License, or (at your option) any later version.

  This software is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
  See the GNU Lesser General Public License for more details.
  =========================================================================================*/

#ifndef DAO_BASE_H
#define DAO_BASE_H

#include"dao.h"

#if defined (__GNUC__)

#if NO_FENV

#define dao_fe_clear()  0
#define dao_fe_status()  0

#else

#include"fenv.h"
#define dao_fe_clear()  feclearexcept( FE_ALL_EXCEPT )
#define dao_fe_status()  fetestexcept( FE_ALL_EXCEPT )

#endif

#ifdef FE_DIVBYZERO
#define dao_fe_divbyzero()  fetestexcept( FE_DIVBYZERO )
#else
#define dao_fe_divbyzero()  0
#endif

#ifdef FE_UNDERFLOW
#define dao_fe_underflow()  fetestexcept( FE_UNDERFLOW )
#else
#define dao_fe_underflow()  0
#endif

#ifdef FE_OVERFLOW
#define dao_fe_overflow()  fetestexcept( FE_OVERFLOW )
#else
#define dao_fe_overflow()  0
#endif

#ifdef FE_INVALID
#define dao_fe_invalid()  fetestexcept( FE_INVALID )
#else
#define dao_fe_invalid()  0
#endif


#elif defined (_MSC_VER)

#include"float.h"

#define dao_fe_clear()  _clearfp()
#define dao_fe_status()  _status87()
#define dao_fe_divbyzero()  (_status87() & _SW_ZERODIVIDE)
#define dao_fe_underflow()  (_status87() & _SW_UNDERFLOW)
#define dao_fe_overflow()  (_status87() & _SW_OVERFLOW)
#define dao_fe_invalid()  (_status87() & _SW_INVALID)

#define strtoll _strtoi64
#define wcstoll _wcstoi64

#endif

#if defined (WIN32) && !defined (__GNUC__)
#define snprintf _snprintf
#define popen _popen
#define pclose _pclose
#endif


typedef struct DRoutine   DRoutine;

typedef struct DaoFunCurry   DaoFunCurry;
typedef struct DaoCdataCore  DaoCdataCore;
typedef struct DaoTypeKernel DaoTypeKernel;

typedef struct DaoToken      DaoToken;
typedef struct DaoInode      DaoInode;

typedef struct DaoVmCode     DaoVmCode;
typedef struct DaoVmCodeX    DaoVmCodeX;
typedef struct DaoVmcArray   DaoVmcArray;

typedef struct DaoException  DaoException;
typedef struct DaoNameValue  DaoNameValue;

typedef struct DaoMacro      DaoMacro;
typedef struct DaoParser     DaoParser;

typedef struct DaoStackFrame    DaoStackFrame;

#define STRCMP( x, y ) strcmp( (x)->mbs, y )
#define TOKCMP( x, y ) strcmp( (x)->string->mbs, y )

/* bit structure of lookup index:
 * S4P2U12I16 = SSSSPPUUUUUUUUUUUUIIIIIIIIIIIIIIII
 * S: storage; P: permission; U: up/parent; I: index*/
#define LOOKUP_BIND( st, pm, up, id )  (((st)<<28)|((pm)<<26)|((up)<<16)|id)

#define LOOKUP_BIND_LC( id ) ((DAO_LOCAL_CONSTANT<<28)|id)
#define LOOKUP_BIND_GC( id ) ((DAO_GLOBAL_CONSTANT<<28)|id)
#define LOOKUP_BIND_GV( id ) ((DAO_GLOBAL_VARIABLE<<28)|id)

#define LOOKUP_ST( one )  ((one)>>28)
#define LOOKUP_PM( one )  (((one)>>26)&0x3)
#define LOOKUP_UP( one )  (((one)>>16)&0x3ff)
#define LOOKUP_ID( one )  ((unsigned short)((one)&0xffff))

DAO_DLL void* dao_malloc( size_t size );
DAO_DLL void* dao_calloc( size_t nmemb, size_t size );
DAO_DLL void* dao_realloc( void *ptr, size_t size );
DAO_DLL void  dao_free( void *p );

typedef struct DaoConfig  DaoConfig;
struct DaoConfig
{
	short cpu;  /* number of CPU */
	short jit;  /* enable JIT compiling, TODO fix */
	short safe; /* enable safe running mode */
	short typedcode; /* enable typed VM codes */
	short incompile; /* enable incremental compiling */
	short iscgi;     /* is CGI script */
	short tabspace;  /* number of spaces counted for a tab */
	short chindent;  /* check indentation */
	short mbs; /* MBS only */
	short wcs; /* WCS only */
};

extern DaoConfig daoConfig;


#endif
