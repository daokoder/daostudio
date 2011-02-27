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

#ifndef _DAO_DEBUGGER_H_
#define _DAO_DEBUGGER_H_

#include<QList>

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
}

#include<QLocalServer>
#include<QString>
#include<QMap>

class DaoDebugger : public QObject
{
    Q_OBJECT

        QList<int> align1;
    QList<int> align2;
    QMap<int,int> regmap;
    QVector<QVector<int> > codesim;

    QMap<QString,QMap<int,int> > breakPoints;

    QLocalServer server;

    public:
    DaoDebugger();

    void SetBreakPoints( DaoRoutine *routine );
    void ResetExecution( DaoContext *context, int line, int offset );
    void Similarity( QList<DaoVmCode> & x, QList<DaoVmCode> & y );
    void Matching( QList<DaoVmCode> & x, QList<DaoVmCode> & y );
    void Matching( int i, int j );
    bool EditContinue( DaoContext*, int, QList<int>&, QStringList&, QStringList& );

    protected slots:

        void slotSetBreakPoint();
};

#endif
