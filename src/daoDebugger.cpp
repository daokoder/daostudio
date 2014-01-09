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

#include<QtCore>
#include<QtGui>
#include<QtNetwork>

#include"daoDebugger.h"
#include<daoStudioMain.h>

DaoxDebugger::DaoxDebugger()
{
	if( QFile::exists( DaoStudioSettings::socket_breakpoints ) )
		QFile::remove( DaoStudioSettings::socket_breakpoints );
	server.listen( DaoStudioSettings::socket_breakpoints );
	connect( & server, SIGNAL(newConnection()), this, SLOT(slotSetBreakPoint()));
}
void DaoxDebugger::slotSetBreakPoint()
{
	QLocalSocket *socket = server.nextPendingConnection();
	socket->waitForReadyRead();
	QList<QByteArray> data = socket->readAll().split( '\0' );
	QString name = data[0];
	int set = data[1].toInt();
	int line = data[2].toInt();
	if( set ){
		if( not breakPoints.contains( name ) ) breakPoints[ name ] = QMap<int,int>();
		breakPoints[ name ][ line ] = 1;
	}else{
		if( breakPoints.contains( name ) ) breakPoints[name].remove( line );
		if( breakPoints[name].size() ==0 ) breakPoints.remove( name );
	}
}

void DaoxDebugger::SetBreakPoints( DaoRoutine *routine )
{
	QFileInfo fi( routine->nameSpace->name->mbs );
	QString name = fi.absoluteFilePath();
	DaoVmCode  *codes = routine->body->vmCodes->data.codes;
	DaoVmCodeX **annots = routine->body->annotCodes->items.pVmc;
	int i, j, n = routine->body->vmCodes->size;

	for(i=0; i<n; i++) if( codes[i].code == DVM_DEBUG ) codes[i].code = DVM_NOP;
	if( not breakPoints.contains( name ) ) return;

	QList<int> breakPointLines = breakPoints[name].keys();
	j = 0;
	//printf( "break points: %i\n", breakPointLines.size() );
	for(i=0; i<breakPointLines.size(); i++){
		int k = breakPointLines[i];
		while( j < n && annots[j]->line < k ) j ++;
		while( j < n && annots[j]->line == k && codes[j].code != DVM_NOP ) j ++;
		if( j < n && annots[j]->line == k && codes[j].code == DVM_NOP ){
			//printf( "1 break point:  %3i  %3i\n", i, k );
			codes[j].code = DVM_DEBUG;
		}
	}
}
void DaoxDebugger::ResetExecution( DaoProcess *process, int line, int offset )
{
	DaoRoutine *routine = process->activeRoutine;
	DaoVmCodeX **annotCodes = routine->body->annotCodes->items.pVmc;
	int i = 0, n = routine->body->annotCodes->size;
	while( i < n && annotCodes[i]->line < line ) ++i;
	while( i < n && offset >0 && annotCodes[i]->line == line){
		++i;
		--offset;
	}
	process->status = DAO_PROCESS_STACKED;
	process->topFrame->entry = i;
}
QString NormalizeCodes( const QString & source, DaoLexer *lexer );
static int DaoVmCode_Similarity( const QMap<int,int> & regmap, DaoVmCode x, DaoVmCode y )
{
	QMap<int,int>::const_iterator it, end = regmap.end();
	int i, errors = 0, sim = 10 * (1 + (x.code == y.code) );
	int xcode = DaoVmCode_GetOpcodeBase( x.code );
	int ycode = DaoVmCode_GetOpcodeBase( y.code );
	DaoCnode xnode, ynode;

	memset( & xnode, 0, sizeof(DaoCnode) );
	memset( & ynode, 0, sizeof(DaoCnode) );
	DaoCnode_InitOperands( & xnode, & x );
	DaoCnode_InitOperands( & ynode, & y );
	if( xcode != ycode || xnode.type != ynode.type ) return -100; // high penalty;
	switch( xnode.type ){
	case DAO_OP_NONE : 
		break;
	case DAO_OP_SINGLE :
		if( (it = regmap.find( xnode.first )) != end && it.value() != ynode.first ) errors += 1;
		break;
	case DAO_OP_PAIR :
		if( (it = regmap.find( xnode.first )) != end && it.value() != ynode.first ) errors += 1;
		if( (it = regmap.find( xnode.second )) != end && it.value() != ynode.second ) errors += 1;
		break;
	case DAO_OP_TRIPLE :
		if( (it = regmap.find( xnode.first )) != end && it.value() != ynode.first ) errors += 1;
		if( (it = regmap.find( xnode.second )) != end && it.value() != ynode.second ) errors += 1;
		if( (it = regmap.find( xnode.third )) != end && it.value() != ynode.third ) errors += 1;
		break;
	case DAO_OP_RANGE :
	case DAO_OP_RANGE2 :
		for(i=xnode.first; i<=xnode.second; i++){
			it = regmap.find( i );
			if( it != end && it.value() != (ynode.first + i - xnode.first) ) errors += 1;
		}
		if( xnode.type == DAO_OP_RANGE2 ){
			if( (it = regmap.find( xnode.third )) != end && it.value() != ynode.third ) errors += 1;
		}
		break;
	}
	return sim - errors;
}
void DaoxDebugger::Similarity( QList<DaoVmCode> & x, QList<DaoVmCode> & y )
{
	int m = x.size() + 1;
	int n = y.size() + 1;
	int i, j;
	if( m ==0 || n ==0 ) return;
	codesim.resize( m );
	for(i=0; i<m; i++){
		codesim[i].resize( n );
		codesim[i][0] = -1000 * i;
	}
	for(i=0; i<n; i++) codesim[0][i] = -1000 * i;
	for(i=1; i<m; i++){
		for(j=1; j<n; j++){
			int s01 = codesim[i-1][j];
			int s10 = codesim[i][j-1];
			int s00 = codesim[i-1][j-1] + DaoVmCode_Similarity( regmap, x[i-1], y[j-1] );
			int max = s01 > s10 ? s01 : s10;
			if( s00 > max ) max = s00;
			codesim[i][j] = max;
			//printf( "%i ", max );
		}
		//printf( "\n" );
	}
}
void DaoxDebugger::Matching( QList<DaoVmCode> & x, QList<DaoVmCode> & y )
{
	align1.clear();
	align2.clear();
	Similarity( x, y );
	Matching( x.size(), y.size() );
	int i, j, k, m, n = align1.size();
#if 0
	//printf( "x: %i; y: %i\n", x.size(), y.size() );
	for(i=0; i<x.size(); i++) DaoVmCode_Print( x[i], NULL );
	for(i=0; i<y.size(); i++) DaoVmCode_Print( y[i], NULL );
#endif
	for(i=0; i<n; i++){
		DaoCnode xnode, ynode;
		int a1 = align1[i];
		int a2 = align2[i];
#if 0
		//printf( "%3i:  %3i  %3i\n", i, a1, a2 );
		if( a1 >=0 ) DaoVmCode_Print( x[a1], NULL );
		if( a2 >=0 ) DaoVmCode_Print( y[a2], NULL );
#endif
		if( a1 < 0 || a2 < 0 ) continue;

		memset( & xnode, 0, sizeof(DaoCnode) );
		memset( & ynode, 0, sizeof(DaoCnode) );
		DaoCnode_InitOperands( & xnode, & x[a1] );
		DaoCnode_InitOperands( & ynode, & y[a2] );

		switch( xnode.type ){
		case DAO_OP_NONE : 
			break;
		case DAO_OP_SINGLE :
			if( regmap.find( xnode.first ) == regmap.end() ) regmap[ xnode.first ] = ynode.first;
			break;
		case DAO_OP_PAIR :
			if( regmap.find( xnode.first ) == regmap.end() ) regmap[ xnode.first ] = ynode.first;
			if( regmap.find( xnode.second ) == regmap.end() ) regmap[ xnode.second ] = ynode.second;
			break;
		case DAO_OP_TRIPLE :
			if( regmap.find( xnode.first ) == regmap.end() ) regmap[ xnode.first ] = ynode.first;
			if( regmap.find( xnode.second ) == regmap.end() ) regmap[ xnode.second ] = ynode.second;
			if( regmap.find( xnode.third ) == regmap.end() ) regmap[ xnode.third ] = ynode.third;
			break;
		case DAO_OP_RANGE :
		case DAO_OP_RANGE2 :
			for(i=xnode.first; i<=xnode.second; i++){
				if( regmap.find( i ) == regmap.end() ) regmap[i] = ynode.first + i - xnode.first;
			}
			if( xnode.type == DAO_OP_RANGE2 ){
				if( regmap.find( xnode.third ) == regmap.end() ) regmap[ xnode.third ] = ynode.third;
			}
			break;
		}
	}
}
void DaoxDebugger::Matching( int i, int j )
{
	if( i >0 && codesim[i][j] == codesim[i-1][j] ){
		Matching( i-1, j );
		align1.append( i-1 );
		align2.append( -1 );
	}else if( j >0 && codesim[i][j] == codesim[i][j-1] ){
		Matching( i, j-1 );
		align1.append( -1 );
		align2.append( j-1 );
	}else if( i >0 && j >0 ){
		Matching( i-1, j-1 );
		align1.append( i-1 );
		align2.append( j-1 );
	}
}
static void DaoNS_UpdateLineInfo( DaoNamespace *ns, int start, int diff )
{
	size_t i, j, n;
	if( diff ==0 ) return;
	for(i=0; i<ns->definedRoutines->size; i++){
		DaoRoutine *rout = ns->definedRoutines->items.pRoutine[i];
		if( rout->nameSpace != ns ) continue;
		while( rout->body->revised ) rout = rout->body->revised;
		if( rout->body->codeStart <= start ) continue;
		rout->body->codeStart += diff;
		rout->body->codeEnd += diff;
		n = rout->body->annotCodes->size;
		DaoVmCodeX **annots = rout->body->annotCodes->items.pVmc;
		for(j=0; j<n; j++) annots[j]->line += diff;
		DaoToken **tokens = rout->body->defLocals->items.pToken;
		n = rout->body->defLocals->size;
		for(j=0; j<n; j++) tokens[j]->line += diff;
	}
}
bool DaoxDebugger::EditContinue ( DaoProcess *process, int newEntryLine, QList<int> & lineMap, QStringList & newCodes, QStringList & routCodes )
{
	DaoRoutine *oldrout = process->activeRoutine;
	int i, j, k, dest = 0;
	//printf( "=======%s\n", routCodes.join("\n").toLocal8Bit().data() );
	//printf( "=======%s\n", newCodes.join("\n").toLocal8Bit().data() );
	if( routCodes.size() == newCodes.size() ){
		DaoLexer *lexer = DaoLexer_New();
		bool eq = true;
		for(i=0; i<routCodes.size(); i++){
			QString s1 = NormalizeCodes( routCodes[i], lexer );
			QString s2 = NormalizeCodes( newCodes[i], lexer );
			if( s1 != s2 ){
				eq = false;
				break;
			}
		}
		DaoLexer_Delete( lexer );
		if( eq ) return true;
	}
	QString codes = newCodes.join( "\n" );
	DaoParser *parser = DaoParser_New();
	DaoRoutine *routine = DaoRoutine_New( oldrout->nameSpace, oldrout->routHost, 1 );
	routine->routType = oldrout->routType;
	routine->parCount = oldrout->parCount;
	routine->attribs = oldrout->attribs;
	routine->defLine = oldrout->defLine;
	parser->routine = routine;
	parser->nameSpace = routine->nameSpace = oldrout->nameSpace;
	parser->vmSpace = oldrout->nameSpace->vmSpace;
	DString_Assign( parser->fileName, oldrout->nameSpace->name );
	DString_Assign( parser->routName, oldrout->routName );
	routine->body->codeStart = oldrout->body->codeStart;
	routine->body->codeEnd = oldrout->body->codeStart + newCodes.size() + 1;
	parser->regCount = routine->parCount;
	parser->levelBase = oldrout->defLine != 0;
	bool res = DaoParser_LexCode( parser, codes.toLocal8Bit().data(), 1 );
	for(i=0; i<(int)parser->tokens->size; i++){
		parser->tokens->items.pToken[i]->line += routine->body->codeStart;
	}
	for(i=0; i<(int)oldrout->body->defLocals->size; i++){
		DaoToken *tok = oldrout->body->defLocals->items.pToken[i];
		if( tok->index >= oldrout->parCount || tok->type ==0 ) break;
		MAP_Insert( DArray_Top( parser->localDataMaps ), & tok->string, i );
		DArray_Append( routine->body->defLocals, tok );
	}
	res = res && DaoParser_ParseRoutine( parser );
	DaoParser_Delete( parser );
	if( res == false ){
		DaoRoutine_Delete( routine );
		return false;
	}
	if( (process->stackSize - process->stackTop) < (routine->body->regCount + DAO_MAX_PARAM) ){
		DaoProcess_PushFrame( process, routine->body->regCount );
		DaoProcess_PopFrame( process );
	}
	DaoType **regTypes = routine->body->regType->items.pType;
	DaoValue **newValues = process->activeValues;
	DaoValue **oldValues = (DaoValue**)calloc( oldrout->body->regCount, sizeof(DaoValue*) );

	memcpy( oldValues, newValues, oldrout->body->regCount * sizeof(DaoValue*) );
	memset( newValues, 0, oldrout->body->regCount * sizeof(DaoValue*) );
	DaoProcess_InitTopFrame( process, routine, process->activeObject );

#if 0
	DaoStream *stream = DaoStream_New();
	DaoRoutine_PrintCode( oldrout, stream );
	DaoRoutine_PrintCode( routine, stream );
#endif

	regmap.clear();
	for(i=0; i<oldrout->parCount; i++) regmap[i] = i;

	DaoVmCode   *oldVMC = oldrout->body->vmCodes->data.codes;
	DaoVmCode   *newVMC = routine->body->vmCodes->data.codes;
	DaoVmCodeX **oldAnnot = oldrout->body->annotCodes->items.pVmc;
	DaoVmCodeX **newAnnot = routine->body->annotCodes->items.pVmc;
	int M = oldrout->body->vmCodes->size;
	int N = routine->body->vmCodes->size;
	j = k = 0;
	for(i=0; i<lineMap.size(); i++){
		QList<DaoVmCode> oldLineCodes;
		QList<DaoVmCode> newLineCodes;
		if( lineMap[i] <0 ) continue;
		int old = lineMap[i] + oldrout->body->codeStart + 1;
		int niu = i + routine->body->codeStart + 1;
		//printf( "%3i  %3i: %3i  %3i;  %3i  %3i\n", j, k, i, niu, lineMap[i], old );
		while( j<M && oldAnnot[j]->line < old ) ++j;
		while( k<N && newAnnot[k]->line < niu ) ++k;
		while( j<M && oldAnnot[j]->line == old ){
			oldLineCodes.append( oldVMC[j] );
			++j;
		}
		while( k<N && newAnnot[k]->line == niu ){
			newLineCodes.append( newVMC[k] );
			++k;
		}
		Matching( oldLineCodes, newLineCodes );
	}
	QMap<int,int>::iterator it, end = regmap.end();
	for(it=regmap.begin(); it != end; ++it){
		j = it.key();
		i = it.value();
		DaoValue_Move( oldValues[j], & newValues[i], regTypes[i] );
	}

	int offset = 0;
	if( newEntryLine <0 ){
		DaoVmCodeX **annotCodes = oldrout->body->annotCodes->items.pVmc;
		int entry = (process->activeCode - process->topFrame->codes) + 1;
		int entryline = oldrout->body->annotCodes->items.pVmc[entry]->line;
		/* if the entry line is NOT modified, use it */
		entryline -= oldrout->body->codeStart + 1;
		for(i=0; i<lineMap.size(); i++) if( lineMap[i] == entryline ) break;
		int newEntryLine = i < lineMap.size() ? i : -1;
		if( newEntryLine >=0 ){
			entryline += oldrout->body->codeStart + 1;
			while( (--entry) >=0 && annotCodes[entry]->line == entryline ) offset ++;
		}
		/* if the entry line IS modified, set the entry line to the first modified line */
		if( newEntryLine <0 ){
			for(i=0; i<lineMap.size(); i++) if( lineMap[i] <0 ) break;
			newEntryLine = i;
		}
		/* if the entry line is manually set: */
		newEntryLine += routine->body->codeStart + 1;
	}

	GC_ShiftRC( routine, oldrout );
	GC_IncRC( routine );
	oldrout->body->revised = routine;
	process->activeRoutine = routine;
	process->activeTypes = regTypes;
	process->topFrame->codes = routine->body->vmCodes->data.codes;

	ResetExecution( process, newEntryLine, offset );

	i = newCodes.size() - routCodes.size();
	DaoNS_UpdateLineInfo( routine->nameSpace, routine->body->codeStart, i );
	return true;
}
