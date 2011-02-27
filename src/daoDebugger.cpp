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

#include<QtCore>
#include<QtGui>
#include<QtNetwork>

#include"daoDebugger.h"

DaoDebugger::DaoDebugger()
{
    system( "rm /tmp/daostudio.socket.breakpoints" );
    server.listen( "/tmp/daostudio.socket.breakpoints" );
    connect( & server, SIGNAL(newConnection()), this, SLOT(slotSetBreakPoint()));
}
void DaoDebugger::slotSetBreakPoint()
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

void DaoDebugger::SetBreakPoints( DaoRoutine *routine )
{
    QFileInfo fi( routine->nameSpace->name->mbs );
    QString name = fi.absoluteFilePath();
    DaoVmCode  *codes = routine->vmCodes->codes;
    DaoVmCodeX **annots = routine->annotCodes->items.pVmc;
    int i, j, n = routine->vmCodes->size;

    for(i=0; i<n; i++) if( codes[i].code == DVM_DEBUG ) codes[i].code = DVM_NOP;
    if( not breakPoints.contains( name ) ) return;

    QList<int> breakPointLines = breakPoints[name].keys();
    j = 0;
    //printf( "break points: %i\n", breakPointLines.size() );
    for(i=0; i<breakPointLines.size(); i++){
        int k = breakPointLines[i];
        //printf( "1 break point:  %3i  %3i\n", i, k );
        while( j < n && annots[j]->line < k ) j ++;
        while( j < n && annots[j]->line == k && codes[j].code != DVM_NOP ) j ++;
        if( codes[j].code == DVM_NOP ){
            codes[j].code = DVM_DEBUG;
            break;
        }
    }
}
void DaoDebugger::ResetExecution( DaoContext *context, int line, int offset )
{
    DaoRoutine *routine = context->routine;
    DaoVmCodeX **annotCodes = routine->annotCodes->items.pVmc;
    int i = 0, n = routine->annotCodes->size;
    while( i < n && annotCodes[i]->line < line ) ++i;
    while( i < n && offset >0 && annotCodes[i]->line == line){
        ++i;
        --offset;
    }
    context->process->status = DAO_VMPROC_STACKED;
    context->process->topFrame->entry = i;
}
static QString NormalizeCodes( const QString & source, DArray *tokens )
{
    QString norm;
    DaoToken_Tokenize( tokens, source.toLocal8Bit().data(), 0, 0, 0 );
    for(size_t i=0; i<tokens->size; i++){
        norm += tokens->items.pToken[i]->string->mbs;
        norm += '\n';
    }
    return norm;
}
/* 0: ignore;
 * 1: register id;
 * 2: register count;
 * 3: index; XXX not considered yet
 */
static short DaoVmCodeInfo[][5] =
{
    { 0, 0, 0, DVM_NOP,  DVM_NOP } , 
    { 0, 0, 1, DVM_DATA, DVM_DATA } , 
    { 3, 0, 1, DVM_GETCL, DVM_GETCL } , 
    { 3, 0, 1, DVM_GETCK, DVM_GETCK } , 
    { 3, 0, 1, DVM_GETCG, DVM_GETCG } , 
    { 3, 3, 1, DVM_GETVL, DVM_GETVL } , 
    { 3, 3, 1, DVM_GETVO, DVM_GETVO } , 
    { 3, 3, 1, DVM_GETVK, DVM_GETVK } , 
    { 3, 3, 1, DVM_GETVG, DVM_GETVG } , 
    { 1, 1, 1, DVM_GETI,  DVM_GETI } ,  
    { 0, 2, 1, DVM_GETMI, DVM_GETMI } , 
    { 1, 0, 1, DVM_GETF,  DVM_GETF } ,  
    { 1, 0, 1, DVM_GETMF, DVM_GETMF } ,  
    { 1, 3, 3, DVM_SETVL, DVM_SETVL } , 
    { 1, 3, 3, DVM_SETVO, DVM_SETVO } , 
    { 1, 3, 3, DVM_SETVK, DVM_SETVK } , 
    { 1, 3, 3, DVM_SETVG, DVM_SETVG } , 
    { 1, 1, 1, DVM_SETI,  DVM_SETI } , 
    { 1, 1, 1, DVM_SETMI, DVM_SETMI } ,  // TODO
    { 1, 0, 1, DVM_SETF,  DVM_SETF } , 
    { 1, 0, 1, DVM_SETMF, DVM_SETMF } , 
    { 1, 0, 1, DVM_LOAD, DVM_LOAD } ,
    { 1, 0, 1, DVM_CAST, DVM_CAST } , 
    { 1, 0, 1, DVM_MOVE, DVM_MOVE } , 
    { 1, 0, 1, DVM_NOT,  DVM_NOT } ,  
    { 1, 0, 1, DVM_UNMS, DVM_UNMS } , 
    { 1, 0, 1, DVM_BITREV, DVM_BITREV } , 
    { 1, 1, 1, DVM_ADD, DVM_ADD } ,  
    { 1, 1, 1, DVM_SUB, DVM_SUB } ,  
    { 1, 1, 1, DVM_MUL, DVM_MUL } ,  
    { 1, 1, 1, DVM_DIV, DVM_DIV } ,  
    { 1, 1, 1, DVM_MOD, DVM_MOD } ,  
    { 1, 1, 1, DVM_POW, DVM_POW } ,  
    { 1, 1, 1, DVM_AND, DVM_AND } ,  
    { 1, 1, 1, DVM_OR, DVM_OR } ,   
    { 1, 1, 1, DVM_LT, DVM_LT } ,   
    { 1, 1, 1, DVM_LE, DVM_LE } ,   
    { 1, 1, 1, DVM_EQ, DVM_EQ } ,   
    { 1, 1, 1, DVM_NE, DVM_NE } ,   
    { 1, 1, 1, DVM_IN, DVM_IN } ,   
    { 1, 1, 1, DVM_BITAND, DVM_BITAND } , 
    { 1, 1, 1, DVM_BITOR,  DVM_BITOR } ,  
    { 1, 1, 1, DVM_BITXOR, DVM_BITXOR } ,  
    { 1, 1, 1, DVM_BITLFT, DVM_BITLFT } , 
    { 1, 1, 1, DVM_BITRIT, DVM_BITRIT } , 
    { 1, 1, 1, DVM_CHECK,  DVM_CHECK } , 
    { 1, 1, 1, DVM_NAMEVA,  DVM_NAMEVA } , 
    { 1, 1, 1, DVM_PAIR,  DVM_PAIR } , 
    { 0, 2, 1, DVM_TUPLE, DVM_TUPLE } , 
    { 0, 2, 1, DVM_LIST,  DVM_LIST } , 
    { 0, 2, 1, DVM_MAP,   DVM_MAP } , 
    { 0, 2, 1, DVM_HASH,  DVM_HASH } , 
    { 0, 2, 1, DVM_ARRAY, DVM_ARRAY } , 
    { 0, 2, 1, DVM_MATRIX, DVM_MATRIX } , 
    { 0, 2, 1, DVM_CURRY,  DVM_CURRY } , 
    { 0, 2, 1, DVM_MCURRY, DVM_MCURRY } , 
    { 0, 2, 1, DVM_ROUTINE, DVM_ROUTINE } , 
    { 0, 2, 1, DVM_CLASS, DVM_CLASS } , 
    { 0, 0, 0, DVM_GOTO,   DVM_GOTO } , 
    { 1, 0, 0, DVM_SWITCH, DVM_SWITCH } , 
    { 1, 0, 0, DVM_CASE, DVM_CASE } , 
    { 1, 0, 1, DVM_ITER, DVM_ITER } , 
    { 1, 0, 0, DVM_TEST, DVM_TEST } , 
    { 0, 1, 1, DVM_MATH, DVM_MATH } , 
    { 0, 1, 1, DVM_FUNCT, DVM_FUNCT } , 
    { 0, 2, 1, DVM_CALL,  DVM_CALL } , 
    { 0, 2, 1, DVM_MCALL, DVM_MCALL } , 
    { 0, 2, 0, DVM_CRRE,  DVM_CRRE } , 

    { 0, 0, 0, DVM_JITC,   DVM_JITC } , 
    { 0, 2, 0, DVM_RETURN, DVM_RETURN } , 
    { 0, 2, 0, DVM_YIELD,  DVM_YIELD } , 
    { 0, 0, 0, DVM_DEBUG,  DVM_DEBUG } , 
    { 0, 0, 0, DVM_SECT,   DVM_SECT } ,   

    { 3, 3, 1, DVM_SETVL, DVM_SETVL_II } , 
    { 3, 3, 1, DVM_SETVL, DVM_SETVL_IF } , 
    { 3, 3, 1, DVM_SETVL, DVM_SETVL_ID } , 
    { 3, 3, 1, DVM_SETVL, DVM_SETVL_FI } , 
    { 3, 3, 1, DVM_SETVL, DVM_SETVL_FF } , 
    { 3, 3, 1, DVM_SETVL, DVM_SETVL_FD } , 
    { 3, 3, 1, DVM_SETVL, DVM_SETVL_DI } , 
    { 3, 3, 1, DVM_SETVL, DVM_SETVL_DF } , 
    { 3, 3, 1, DVM_SETVL, DVM_SETVL_DD } , 

    { 3, 3, 1, DVM_SETVO, DVM_SETVO_II } , 
    { 3, 3, 1, DVM_SETVO, DVM_SETVO_IF } , 
    { 3, 3, 1, DVM_SETVO, DVM_SETVO_ID } , 
    { 3, 3, 1, DVM_SETVO, DVM_SETVO_FI } , 
    { 3, 3, 1, DVM_SETVO, DVM_SETVO_FF } , 
    { 3, 3, 1, DVM_SETVO, DVM_SETVO_FD } , 
    { 3, 3, 1, DVM_SETVO, DVM_SETVO_DI } , 
    { 3, 3, 1, DVM_SETVO, DVM_SETVO_DF } , 
    { 3, 3, 1, DVM_SETVO, DVM_SETVO_DD } , 

    { 3, 3, 1, DVM_SETVK, DVM_SETVK_II } , 
    { 3, 3, 1, DVM_SETVK, DVM_SETVK_IF } , 
    { 3, 3, 1, DVM_SETVK, DVM_SETVK_ID } , 
    { 3, 3, 1, DVM_SETVK, DVM_SETVK_FI } , 
    { 3, 3, 1, DVM_SETVK, DVM_SETVK_FF } , 
    { 3, 3, 1, DVM_SETVK, DVM_SETVK_FD } , 
    { 3, 3, 1, DVM_SETVK, DVM_SETVK_DI } , 
    { 3, 3, 1, DVM_SETVK, DVM_SETVK_DF } , 
    { 3, 3, 1, DVM_SETVK, DVM_SETVK_DD } , 

    { 3, 3, 1, DVM_SETVG, DVM_SETVG_II } , 
    { 3, 3, 1, DVM_SETVG, DVM_SETVG_IF } , 
    { 3, 3, 1, DVM_SETVG, DVM_SETVG_ID } , 
    { 3, 3, 1, DVM_SETVG, DVM_SETVG_FI } , 
    { 3, 3, 1, DVM_SETVG, DVM_SETVG_FF } , 
    { 3, 3, 1, DVM_SETVG, DVM_SETVG_FD } , 
    { 3, 3, 1, DVM_SETVG, DVM_SETVG_DI } , 
    { 3, 3, 1, DVM_SETVG, DVM_SETVG_DF } , 
    { 3, 3, 1, DVM_SETVG, DVM_SETVG_DD } , 

    { 1, 0, 1, DVM_MOVE, DVM_MOVE_II } ,
    { 1, 0, 1, DVM_MOVE, DVM_MOVE_IF } ,
    { 1, 0, 1, DVM_MOVE, DVM_MOVE_ID } ,
    { 1, 0, 1, DVM_MOVE, DVM_MOVE_FI } ,
    { 1, 0, 1, DVM_MOVE, DVM_MOVE_FF } ,
    { 1, 0, 1, DVM_MOVE, DVM_MOVE_FD } ,
    { 1, 0, 1, DVM_MOVE, DVM_MOVE_DI } ,
    { 1, 0, 1, DVM_MOVE, DVM_MOVE_DF } ,
    { 1, 0, 1, DVM_MOVE, DVM_MOVE_DD } ,
    { 1, 0, 1, DVM_MOVE, DVM_MOVE_CC } , 
    { 1, 0, 1, DVM_MOVE, DVM_MOVE_SS } , 
    { 1, 0, 1, DVM_MOVE, DVM_MOVE_PP } , 
    { 1, 0, 1, DVM_NOT, DVM_NOT_I } ,
    { 1, 0, 1, DVM_NOT, DVM_NOT_F } ,
    { 1, 0, 1, DVM_NOT, DVM_NOT_D } ,
    { 1, 0, 1, DVM_UNMS, DVM_UNMS_I } ,
    { 1, 0, 1, DVM_UNMS, DVM_UNMS_F } ,
    { 1, 0, 1, DVM_UNMS, DVM_UNMS_D } ,
    { 1, 0, 1, DVM_BITREV, DVM_BITREV_I } ,
    { 1, 0, 1, DVM_BITREV, DVM_BITREV_F } ,
    { 1, 0, 1, DVM_BITREV, DVM_BITREV_D } ,
    { 1, 0, 1, DVM_UNMS,   DVM_UNMS_C } ,

    { 1, 1, 1, DVM_ADD, DVM_ADD_III } ,
    { 1, 1, 1, DVM_SUB, DVM_SUB_III } ,
    { 1, 1, 1, DVM_MUL, DVM_MUL_III } ,
    { 1, 1, 1, DVM_DIV, DVM_DIV_III } ,
    { 1, 1, 1, DVM_MOD, DVM_MOD_III } ,
    { 1, 1, 1, DVM_POW, DVM_POW_III } ,
    { 1, 1, 1, DVM_AND, DVM_AND_III } ,
    { 1, 1, 1, DVM_OR, DVM_OR_III } ,
    { 1, 1, 1, DVM_LT, DVM_LT_III } ,
    { 1, 1, 1, DVM_LE, DVM_LE_III } ,
    { 1, 1, 1, DVM_EQ, DVM_EQ_III } ,
    { 1, 1, 1, DVM_NE, DVM_NE_III } ,
    { 1, 1, 1, DVM_BITAND, DVM_BITAND_III } ,
    { 1, 1, 1, DVM_BITOR,  DVM_BITOR_III } ,
    { 1, 1, 1, DVM_BITXOR, DVM_BITXOR_III } ,
    { 1, 1, 1, DVM_BITLFT, DVM_BITLFT_III } ,
    { 1, 1, 1, DVM_BITRIT, DVM_BITRIT_III } ,

    { 1, 1, 1, DVM_ADD, DVM_ADD_FFF } ,
    { 1, 1, 1, DVM_SUB, DVM_SUB_FFF } ,
    { 1, 1, 1, DVM_MUL, DVM_MUL_FFF } ,
    { 1, 1, 1, DVM_DIV, DVM_DIV_FFF } ,
    { 1, 1, 1, DVM_MOD, DVM_MOD_FFF } ,
    { 1, 1, 1, DVM_POW, DVM_POW_FFF } ,
    { 1, 1, 1, DVM_AND, DVM_AND_FFF } ,
    { 1, 1, 1, DVM_OR, DVM_OR_FFF } ,
    { 1, 1, 1, DVM_LT, DVM_LT_FFF } ,
    { 1, 1, 1, DVM_LE, DVM_LE_FFF } ,
    { 1, 1, 1, DVM_EQ, DVM_EQ_FFF } ,
    { 1, 1, 1, DVM_NE, DVM_NE_FFF } ,
    { 1, 1, 1, DVM_BITAND, DVM_BITAND_FFF } ,
    { 1, 1, 1, DVM_BITOR,  DVM_BITOR_FFF } ,
    { 1, 1, 1, DVM_BITXOR, DVM_BITXOR_FFF } ,
    { 1, 1, 1, DVM_BITLFT, DVM_BITLFT_FFF } ,
    { 1, 1, 1, DVM_BITRIT, DVM_BITRIT_FFF } ,

    { 1, 1, 1, DVM_ADD, DVM_ADD_DDD } ,
    { 1, 1, 1, DVM_SUB, DVM_SUB_DDD } ,
    { 1, 1, 1, DVM_MUL, DVM_MUL_DDD } ,
    { 1, 1, 1, DVM_DIV, DVM_DIV_DDD } ,
    { 1, 1, 1, DVM_MOD, DVM_MOD_DDD } ,
    { 1, 1, 1, DVM_POW, DVM_POW_DDD } ,
    { 1, 1, 1, DVM_AND, DVM_AND_DDD } ,
    { 1, 1, 1, DVM_OR, DVM_OR_DDD } ,
    { 1, 1, 1, DVM_LT, DVM_LT_DDD } ,
    { 1, 1, 1, DVM_LE, DVM_LE_DDD } ,
    { 1, 1, 1, DVM_EQ, DVM_EQ_DDD } ,
    { 1, 1, 1, DVM_NE, DVM_NE_DDD } ,
    { 1, 1, 1, DVM_BITAND, DVM_BITAND_DDD } ,
    { 1, 1, 1, DVM_BITOR,  DVM_BITOR_DDD } ,
    { 1, 1, 1, DVM_BITXOR, DVM_BITXOR_DDD } ,
    { 1, 1, 1, DVM_BITLFT, DVM_BITLFT_DDD } ,
    { 1, 1, 1, DVM_BITRIT, DVM_BITRIT_DDD } ,

    { 1, 1, 1, DVM_ADD, DVM_ADD_FNN } ,
    { 1, 1, 1, DVM_SUB, DVM_SUB_FNN } ,
    { 1, 1, 1, DVM_MUL, DVM_MUL_FNN } ,
    { 1, 1, 1, DVM_DIV, DVM_DIV_FNN } ,
    { 1, 1, 1, DVM_MOD, DVM_MOD_FNN } ,
    { 1, 1, 1, DVM_POW, DVM_POW_FNN } ,
    { 1, 1, 1, DVM_AND, DVM_AND_FNN } ,
    { 1, 1, 1, DVM_OR, DVM_OR_FNN } ,
    { 1, 1, 1, DVM_LT, DVM_LT_FNN } ,
    { 1, 1, 1, DVM_LE, DVM_LE_FNN } ,
    { 1, 1, 1, DVM_EQ, DVM_EQ_FNN } ,
    { 1, 1, 1, DVM_NE, DVM_NE_FNN } ,
    { 1, 1, 1, DVM_BITLFT, DVM_BITLFT_FNN } ,
    { 1, 1, 1, DVM_BITRIT, DVM_BITRIT_FNN } ,

    { 1, 1, 1, DVM_ADD, DVM_ADD_DNN } ,
    { 1, 1, 1, DVM_SUB, DVM_SUB_DNN } ,
    { 1, 1, 1, DVM_MUL, DVM_MUL_DNN } ,
    { 1, 1, 1, DVM_DIV, DVM_DIV_DNN } ,
    { 1, 1, 1, DVM_MOD, DVM_MOD_DNN } ,
    { 1, 1, 1, DVM_POW, DVM_POW_DNN } ,
    { 1, 1, 1, DVM_AND, DVM_AND_DNN } ,
    { 1, 1, 1, DVM_OR, DVM_OR_DNN } ,
    { 1, 1, 1, DVM_LT, DVM_LT_DNN } ,
    { 1, 1, 1, DVM_LE, DVM_LE_DNN } ,
    { 1, 1, 1, DVM_EQ, DVM_EQ_DNN } ,
    { 1, 1, 1, DVM_NE, DVM_NE_DNN } ,
    { 1, 1, 1, DVM_BITLFT, DVM_BITLFT_DNN } ,
    { 1, 1, 1, DVM_BITRIT, DVM_BITRIT_DNN } ,

    { 1, 1, 1, DVM_ADD, DVM_ADD_SS } , 
    { 1, 1, 1, DVM_LT,  DVM_LT_SS } ,
    { 1, 1, 1, DVM_LE,  DVM_LE_SS } ,
    { 1, 1, 1, DVM_EQ,  DVM_EQ_SS } ,
    { 1, 1, 1, DVM_NE,  DVM_NE_SS } ,

    { 1, 1, 1, DVM_GETI, DVM_GETI_LI } , 
    { 1, 1, 1, DVM_SETI, DVM_SETI_LI } , 
    { 1, 1, 1, DVM_GETI, DVM_GETI_SI } , 
    { 1, 1, 1, DVM_SETI, DVM_SETI_SII } , 
    { 1, 1, 1, DVM_GETI, DVM_GETI_LII } , 
    { 1, 1, 1, DVM_GETI, DVM_GETI_LFI } , 
    { 1, 1, 1, DVM_GETI, DVM_GETI_LDI } , 
    { 1, 1, 1, DVM_GETI, DVM_GETI_LSI } , 
    { 1, 1, 1, DVM_SETI, DVM_SETI_LIII } , 
    { 1, 1, 1, DVM_SETI, DVM_SETI_LIIF } , 
    { 1, 1, 1, DVM_SETI, DVM_SETI_LIID } , 
    { 1, 1, 1, DVM_SETI, DVM_SETI_LFII } , 
    { 1, 1, 1, DVM_SETI, DVM_SETI_LFIF } , 
    { 1, 1, 1, DVM_SETI, DVM_SETI_LFID } , 
    { 1, 1, 1, DVM_SETI, DVM_SETI_LDII } , 
    { 1, 1, 1, DVM_SETI, DVM_SETI_LDIF } , 
    { 1, 1, 1, DVM_SETI, DVM_SETI_LDID } , 
    { 1, 1, 1, DVM_SETI, DVM_SETI_LSIS } , 
    { 1, 1, 1, DVM_GETI, DVM_GETI_AII } , 
    { 1, 1, 1, DVM_GETI, DVM_GETI_AFI } , 
    { 1, 1, 1, DVM_GETI, DVM_GETI_ADI } , 
    { 1, 1, 1, DVM_SETI, DVM_SETI_AIII } , 
    { 1, 1, 1, DVM_SETI, DVM_SETI_AIIF } , 
    { 1, 1, 1, DVM_SETI, DVM_SETI_AIID } , 
    { 1, 1, 1, DVM_SETI, DVM_SETI_AFII } , 
    { 1, 1, 1, DVM_SETI, DVM_SETI_AFIF } , 
    { 1, 1, 1, DVM_SETI, DVM_SETI_AFID } , 
    { 1, 1, 1, DVM_SETI, DVM_SETI_ADII } , 
    { 1, 1, 1, DVM_SETI, DVM_SETI_ADIF } , 
    { 1, 1, 1, DVM_SETI, DVM_SETI_ADID } , 

    { 1, 1, 1, DVM_GETI, DVM_GETI_TI } , 
    { 1, 1, 1, DVM_SETI, DVM_SETI_TI } , 

    { 1, 0, 1, DVM_GETF, DVM_GETF_T } ,
    { 1, 0, 1, DVM_GETF, DVM_GETF_TI } , 
    { 1, 0, 1, DVM_GETF, DVM_GETF_TF } , 
    { 1, 0, 1, DVM_GETF, DVM_GETF_TD } , 
    { 1, 0, 1, DVM_GETF, DVM_GETF_TS } , 
    { 1, 0, 1, DVM_SETF, DVM_SETF_T } ,
    { 1, 0, 1, DVM_SETF, DVM_SETF_TII } , 
    { 1, 0, 1, DVM_SETF, DVM_SETF_TIF } , 
    { 1, 0, 1, DVM_SETF, DVM_SETF_TID } , 
    { 1, 0, 1, DVM_SETF, DVM_SETF_TFI } , 
    { 1, 0, 1, DVM_SETF, DVM_SETF_TFF } , 
    { 1, 0, 1, DVM_SETF, DVM_SETF_TFD } , 
    { 1, 0, 1, DVM_SETF, DVM_SETF_TDI } , 
    { 1, 0, 1, DVM_SETF, DVM_SETF_TDF } , 
    { 1, 0, 1, DVM_SETF, DVM_SETF_TDD } , 
    { 1, 0, 1, DVM_SETF, DVM_SETF_TSS } , 

    { 1, 1, 1, DVM_ADD, DVM_ADD_CC } ,
    { 1, 1, 1, DVM_SUB, DVM_SUB_CC } ,
    { 1, 1, 1, DVM_MUL, DVM_MUL_CC } ,
    { 1, 1, 1, DVM_DIV, DVM_DIV_CC } ,
    { 1, 1, 1, DVM_GETI, DVM_GETI_ACI } , 
    { 1, 1, 1, DVM_SETI, DVM_SETI_ACI } , 

    { 1, 1, 1, DVM_GETI, DVM_GETI_AM } , 
    { 1, 1, 1, DVM_SETI, DVM_SETI_AM } , 

    { 1, 0, 1, DVM_GETF, DVM_GETF_M } , 

    { 1, 0, 1, DVM_GETF, DVM_GETF_KC } , 
    { 1, 0, 1, DVM_GETF, DVM_GETF_KG } , 
    { 1, 0, 1, DVM_GETF, DVM_GETF_OC } , 
    { 1, 0, 1, DVM_GETF, DVM_GETF_OG } , 
    { 1, 0, 1, DVM_GETF, DVM_GETF_OV } , 
    { 1, 0, 1, DVM_SETF, DVM_SETF_KG } ,
    { 1, 0, 1, DVM_SETF, DVM_SETF_OG } ,
    { 1, 0, 1, DVM_SETF, DVM_SETF_OV } ,

    { 1, 0, 1, DVM_GETF, DVM_GETF_KCI } , 
    { 1, 0, 1, DVM_GETF, DVM_GETF_KGI } ,
    { 1, 0, 1, DVM_GETF, DVM_GETF_OCI } , 
    { 1, 0, 1, DVM_GETF, DVM_GETF_OGI } ,
    { 1, 0, 1, DVM_GETF, DVM_GETF_OVI } ,
    { 1, 0, 1, DVM_GETF, DVM_GETF_KCF } , 
    { 1, 0, 1, DVM_GETF, DVM_GETF_KGF } ,
    { 1, 0, 1, DVM_GETF, DVM_GETF_OCF } , 
    { 1, 0, 1, DVM_GETF, DVM_GETF_OGF } ,
    { 1, 0, 1, DVM_GETF, DVM_GETF_OVF } ,
    { 1, 0, 1, DVM_GETF, DVM_GETF_KCD } , 
    { 1, 0, 1, DVM_GETF, DVM_GETF_KGD } ,
    { 1, 0, 1, DVM_GETF, DVM_GETF_OCD } , 
    { 1, 0, 1, DVM_GETF, DVM_GETF_OGD } ,
    { 1, 0, 1, DVM_GETF, DVM_GETF_OVD } ,

    { 1, 0, 1, DVM_SETF, DVM_SETF_KGII } ,
    { 1, 0, 1, DVM_SETF, DVM_SETF_OGII } ,
    { 1, 0, 1, DVM_SETF, DVM_SETF_OVII } ,
    { 1, 0, 1, DVM_SETF, DVM_SETF_KGIF } ,
    { 1, 0, 1, DVM_SETF, DVM_SETF_OGIF } ,
    { 1, 0, 1, DVM_SETF, DVM_SETF_OVIF } ,
    { 1, 0, 1, DVM_SETF, DVM_SETF_KGID } ,
    { 1, 0, 1, DVM_SETF, DVM_SETF_OGID } ,
    { 1, 0, 1, DVM_SETF, DVM_SETF_OVID } ,
    { 1, 0, 1, DVM_SETF, DVM_SETF_KGFI } ,
    { 1, 0, 1, DVM_SETF, DVM_SETF_OGFI } ,
    { 1, 0, 1, DVM_SETF, DVM_SETF_OVFI } ,
    { 1, 0, 1, DVM_SETF, DVM_SETF_KGFF } ,
    { 1, 0, 1, DVM_SETF, DVM_SETF_OGFF } ,
    { 1, 0, 1, DVM_SETF, DVM_SETF_OVFF } ,
    { 1, 0, 1, DVM_SETF, DVM_SETF_KGFD } ,
    { 1, 0, 1, DVM_SETF, DVM_SETF_OGFD } ,
    { 1, 0, 1, DVM_SETF, DVM_SETF_OVFD } ,
    { 1, 0, 1, DVM_SETF, DVM_SETF_KGDI } ,
    { 1, 0, 1, DVM_SETF, DVM_SETF_OGDI } ,
    { 1, 0, 1, DVM_SETF, DVM_SETF_OVDI } ,
    { 1, 0, 1, DVM_SETF, DVM_SETF_KGDF } ,
    { 1, 0, 1, DVM_SETF, DVM_SETF_OGDF } ,
    { 1, 0, 1, DVM_SETF, DVM_SETF_OVDF } ,
    { 1, 0, 1, DVM_SETF, DVM_SETF_KGDD } ,
    { 1, 0, 1, DVM_SETF, DVM_SETF_OGDD } ,
    { 1, 0, 1, DVM_SETF, DVM_SETF_OVDD } ,

    { 1, 0, 0, DVM_TEST, DVM_TEST_I } ,
    { 1, 0, 0, DVM_TEST, DVM_TEST_F } ,
    { 1, 0, 0, DVM_TEST, DVM_TEST_D } ,

    { 0, 2, 1, DVM_CALL, DVM_CALL_CF } , 
    { 0, 2, 1, DVM_CALL, DVM_CALL_CMF } , 

    { 0, 2, 1, DVM_CALL, DVM_CALL_TC } , 
    { 0, 2, 1, DVM_MCALL, DVM_MCALL_TC } , 

    { 1, 0, 0, DVM_GOTO, DVM_SAFE_GOTO }, 

    { 0, 0, 0, DVM_NULL, DVM_NULL }
};
static int DaoVmCode_Info( DaoVmCode vmc, QList<int> & regids )
{
    short *info = DaoVmCodeInfo[ vmc.code ];
    int code = info[3];
    if( info[0] == 1 ) regids.append( vmc.a );
    if( info[1] == 1 ){
        regids.append( vmc.b );
    }else if( info[1] == 2 ){
        int i, n = 0;
        switch( code ){
        case DVM_TUPLE : case DVM_LIST : case DVM_MAP : 
        case DVM_ARRAY : case DVM_RETURN : 
        case DVM_YIELD  : n = vmc.b; break;
        case DVM_MATRIX : n = (0xff & vmc.b) * ((0xff00 & vmc.b)>>8); break;
        case DVM_CURRY : case DVM_MCURRY : 
        case DVM_CALL : case DVM_MCALL : n = vmc.b + 1; break;
        case DVM_ROUTINE : case DVM_CRRE : break;
        }
        for(i=0; i<n; i++) regids.append( vmc.a + i );
    }
    if( info[2] == 1 ) regids.append( vmc.c );
    return code;
}
static int DaoVmCode_Similarity( const QMap<int,int> & regmap, DaoVmCode x, DaoVmCode y )
{
    QList<int> xreg, yreg;
    int xcode = DaoVmCode_Info( x, xreg );
    int ycode = DaoVmCode_Info( y, yreg );
    if( xcode != ycode ) return -100; // high penalty;
    if( xreg.size() != yreg.size() ) return -100;

    int i, n = xreg.size();
    int notmatched = 0;
    QMap<int,int>::const_iterator it, end = regmap.end();
    for(i=0; i<n; i++){
        it = regmap.find( xreg[i] );
        if( it != end && it.value() != yreg[i] ) ++notmatched;
    }
    int sim = 10 * (1 + (x.code == y.code) );
    return sim - notmatched;
}
void DaoDebugger::Similarity( QList<DaoVmCode> & x, QList<DaoVmCode> & y )
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
void DaoDebugger::Matching( QList<DaoVmCode> & x, QList<DaoVmCode> & y )
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
        int a1 = align1[i];
        int a2 = align2[i];
#if 0
        //printf( "%3i:  %3i  %3i\n", i, a1, a2 );
        if( a1 >=0 ) DaoVmCode_Print( x[a1], NULL );
        if( a2 >=0 ) DaoVmCode_Print( y[a2], NULL );
#endif
        if( a1 >=0 && a2 >=0 ){
            QList<int> xreg, yreg;
            DaoVmCode_Info( x[a1], xreg );
            DaoVmCode_Info( y[a2], yreg );
            m = xreg.size();
            for(j=0; j<m; j++){
                k = xreg[j];
                if( regmap.find( k ) == regmap.end() ) regmap[k] = yreg[j];
            }
        }
    }
}
void DaoDebugger::Matching( int i, int j )
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
static void DaoNS_UpdateLineInfo( DaoNameSpace *ns, int start, int diff )
{
    size_t i, j, n;
    if( diff ==0 ) return;
    for(i=0; i<ns->definedRoutines->size; i++){
        DaoRoutine *rout = ns->definedRoutines->items.pRout[i];
        if( rout->nameSpace != ns ) continue;
        while( rout->revised ) rout = rout->revised;
        if( rout->bodyStart <= start ) continue;
        rout->bodyStart += diff;
        rout->bodyEnd += diff;
        n = rout->annotCodes->size;
        DaoVmCodeX **annots = rout->annotCodes->items.pVmc;
        for(j=0; j<n; j++) annots[j]->line += diff;
        DaoToken **tokens = rout->defLocals->items.pToken;
        n = rout->defLocals->size;
        for(j=0; j<n; j++) tokens[j]->line += diff;
    }
}
    bool DaoDebugger::EditContinue
( DaoContext *context, int newEntryLine, QList<int> & lineMap, 
  QStringList & newCodes, QStringList & routCodes )
{
    DaoRoutine *oldrout = context->routine;
    int i, j, k, dest = 0;
    //printf( "=======%s\n", routCodes.join("\n").toLocal8Bit().data() );
    //printf( "=======%s\n", newCodes.join("\n").toLocal8Bit().data() );
    if( routCodes.size() == newCodes.size() ){
        DArray *tokens = DArray_New(D_TOKEN);
        bool eq = true;
        for(i=0; i<routCodes.size(); i++){
            QString s1 = NormalizeCodes( routCodes[i], tokens );
            QString s2 = NormalizeCodes( newCodes[i], tokens );
            if( s1 != s2 ){
                eq = false;
                break;
            }
        }
        DArray_Delete( tokens );
        if( eq ) return true;
    }
    QString codes = newCodes.join( "\n" );
    DaoParser *parser = DaoParser_New();
    DaoRoutine *routine = DaoRoutine_New();
    routine->routType = oldrout->routType;
    parser->routine = routine;
    parser->nameSpace = routine->nameSpace = oldrout->nameSpace;
    parser->vmSpace = oldrout->nameSpace->vmSpace;
    DString_Assign( parser->fileName, oldrout->nameSpace->name );
    DString_Assign( parser->routName, oldrout->routName );
    routine->defLine = oldrout->defLine;
    routine->bodyStart = oldrout->bodyStart;
    routine->bodyEnd = oldrout->bodyStart + newCodes.size() + 1;
    routine->attribs = oldrout->attribs;
    routine->parCount = oldrout->parCount;
    parser->locRegCount = routine->parCount;
    parser->levelBase = oldrout->defLine != 0;
    bool res = DaoParser_LexCode( parser, codes.toLocal8Bit().data(), 1 );
    for(i=0; i<(int)parser->tokens->size; i++){
        parser->tokens->items.pToken[i]->line += routine->bodyStart;
    }
    for(i=0; i<(int)oldrout->defLocals->size; i++){
        DaoToken *tok = oldrout->defLocals->items.pToken[i];
        if( tok->index >= oldrout->parCount || tok->type ==0 ) break;
        MAP_Insert( DArray_Top( parser->localVarMap ), tok->string, i );
        DArray_Append( routine->defLocals, tok );
    }
    res = res && DaoParser_ParseRoutine( parser );
    routine->parser = NULL;
    DaoParser_Delete( parser );
    if( res == false ){
        DaoRoutine_Delete( routine );
        return false;
    }
    DaoType **regTypes = routine->regType->items.pType;
    DVaTuple *regArray = DVaTuple_New( routine->locRegCount, daoNullValue );
    DValue **oldValues = context->regValues;
    DValue **newValues = (DValue**)calloc( routine->locRegCount, sizeof(DValue*) );

#if 0
    DaoStream *stream = DaoStream_New();
    DaoRoutine_PrintCode( oldrout, stream );
    DaoRoutine_PrintCode( routine, stream );
#endif

    for(i=0; i<routine->locRegCount; i++){
        newValues[i] = regArray->data + i;
        if( regTypes[i] && regTypes[i]->tid <= DAO_COMPLEX ){
            regArray->data[i].t = regTypes[i]->tid;
            if( regArray->data[i].t == DAO_COMPLEX ){
                regArray->data[i].v.c = (complex16*) dao_malloc( sizeof(complex16) );
            }
        }
    }
    regmap.clear();
    for(i=0; i<oldrout->parCount; i++) regmap[i] = i;

    DaoVmCode   *oldVMC = oldrout->vmCodes->codes;
    DaoVmCode   *newVMC = routine->vmCodes->codes;
    DaoVmCodeX **oldAnnot = oldrout->annotCodes->items.pVmc;
    DaoVmCodeX **newAnnot = routine->annotCodes->items.pVmc;
    int M = oldrout->vmCodes->size;
    int N = routine->vmCodes->size;
    j = k = 0;
    for(i=0; i<lineMap.size(); i++){
        QList<DaoVmCode> oldLineCodes;
        QList<DaoVmCode> newLineCodes;
        if( lineMap[i] <0 ) continue;
        int old = lineMap[i] + oldrout->bodyStart + 1;
        int niu = i + routine->bodyStart + 1;
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
        if( oldValues[j] != context->regArray->data + j ){
            newValues[i] = oldValues[j];
        }else{
            DValue_Move( *oldValues[j], newValues[i], regTypes[i] );
        }
    }

    int offset = 0;
    if( newEntryLine <0 ){
        DaoVmCodeX **annotCodes = oldrout->annotCodes->items.pVmc;
        int entry = (context->vmc - context->codes) + 1;
        int entryline = oldrout->annotCodes->items.pVmc[entry]->line;
        /* if the entry line is NOT modified, use it */
        entryline -= oldrout->bodyStart + 1;
        for(i=0; i<lineMap.size(); i++) if( lineMap[i] == entryline ) break;
        int newEntryLine = i < lineMap.size() ? i : -1;
        if( newEntryLine >=0 ){
            entryline += oldrout->bodyStart + 1;
            while( (--entry) >=0 && annotCodes[entry]->line == entryline ) offset ++;
        }
        /* if the entry line IS modified, set the entry line to the first modified line */
        if( newEntryLine <0 ){
            for(i=0; i<lineMap.size(); i++) if( lineMap[i] <0 ) break;
            newEntryLine = i;
        }
        /* if the entry line is manually set: */
        newEntryLine += routine->bodyStart + 1;
    }

    DVaTuple_Delete( context->regArray );
    free( context->regValues );
    GC_ShiftRC( routine, oldrout );
    GC_IncRC( routine );
    oldrout->revised = routine;
    context->routine = routine;
    context->codes = routine->vmCodes->codes;
    context->regArray = regArray;
    context->regValues = newValues;
    context->regTypes = regTypes;
    ResetExecution( context, newEntryLine, offset );

    i = newCodes.size() - routCodes.size();
    DaoNS_UpdateLineInfo( routine->nameSpace, routine->bodyStart, i );
    return true;
}
