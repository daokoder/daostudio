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

#ifndef DAO_PARSER_H
#define DAO_PARSER_H

#include"daoType.h"
#include"daoLexer.h"


struct DaoParser
{
	DaoVmSpace   *vmSpace;
	DaoNamespace *nameSpace;

	DString *fileName;

	DaoParser *defParser;
	int parStart;
	int parEnd;

	int curToken;

	DArray  *tokens;
	DArray  *partoks;
	DMap    *comments; /* <int,DString*> */

	/* DArray<DaoVmCodeX*>: need to be store as pointers, because in code generation,
	 * it may be necessary to modify previously generated codes, for this,
	 * it is much easier to use pointers. */
	DArray  *vmCodes;

	DaoInode *vmcBase;
	DaoInode *vmcFirst;
	DaoInode *vmcLast;
	int vmcCount;

	/* Stack of maps: mapping local variable names to virtual register ids at each level: */
	DArray  *localVarMap; /* < DMap<DString*,int> > */
	DArray  *localCstMap; /* < DMap<DString*,int> > */
	DArray  *localDecMap; /* < DMap<DString*,int> > */
	DArray  *switchMaps;
	DArray  *enumTypes; /* <DaoType*> */

	/* the line number where a register is first used;
	 * with respect to the first line in the routine body;
	 * -1 is used for register for parameters */
	DArray *regLines; /* <int> : obsolete */

	short levelBase;
	short lexLevel;

	DMap  *allConsts; /* <DString*,int>: implicit and explict constants; */

	DArray *routCompilable; /* list of defined routines with bodies */

	int    regCount;
	int    lastValue;
	DMap  *initTypes; /* type holders @T from parameters and the up routine */

	int noneValue;
	int integerZero;
	int integerOne;
	int imaginaryOne;

	int cmpOptions;
	int exeOptions;

	DaoRoutine *routine;
	DString    *routName;

	/* if 1, variables not nested in any scope are declared as global */
	char topAsGlobal;
	char isClassBody;
	char isInterBody;
	char isDynamicClass;
	char permission;
	char isFunctional;

	DaoInterface *hostInter;
	DaoClass     *hostClass;
	DaoType      *hostCdata;
	DaoType      *hostType;
	DaoParser    *outParser;

	DaoType      *cblockType;
	DaoType      *returnType;

	int curLine;
	int lineCount;
	short indent;
	short defined;
	short error;
	short parsed;
	DArray *scopeOpenings; /* <DaoInode*> */
	DArray *scopeClosings; /* <DaoInode*> */
	DArray *errors;
	DArray *warnings;
	DArray *bindtos;
	DArray *uplocs;
	DArray *outers;
	DArray *decoFuncs;
	DArray *decoParams;

	/* members for convenience */
	DaoEnum   *denum;
	DLong     *bigint;
	DString   *mbs;
	DString   *mbs2;
	DString   *str;
	DMap      *lvm; /* <DString*,int>, for localVarMap; */
	DArray    *toks;

	DArray  *strings;
	DArray  *arrays;
};

DAO_DLL DaoParser* DaoParser_New();
DAO_DLL void DaoParser_Delete( DaoParser *self );

DAO_DLL int DaoParser_LexCode( DaoParser *self, const char *source, int replace );
DAO_DLL int DaoParser_ParsePrototype( DaoParser *self, DaoParser *module, int key, int start );
DAO_DLL int DaoParser_ParseScript( DaoParser *self );
DAO_DLL int DaoParser_ParseRoutine( DaoParser *self );

DAO_DLL DaoType* DaoParser_ParseTypeName( const char *type, DaoNamespace *ns, DaoClass *cls );

#endif
