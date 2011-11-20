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

#include<QtGui>
#include<cmath>
#include<QDir>

#include<daoConsole.h>
#include<daoDebugger.h>
#include<daoInterpreter.h>


#if 0
DaoValueItem::DaoValueItem( DaoValue *p, QListWidget *w ) : QListWidgetItem(w)
{
	type = p->type;
	value = p;
	frame = NULL;
	parent = NULL;
	dataWidget = NULL;
}
DaoDataWidget::DaoDataWidget()
{
	setupUi(this);
	dataSplitter->setStretchFactor( 0, 2 );
	dataSplitter->setStretchFactor( 1, 3 );
	dataSplitter->setStretchFactor( 2, 3 );
	wgtInfoPanel->hide();

	hlDataInfo = new DaoCodeSHL( wgtDataInfo->document() );
	hlDataValue = new DaoCodeSHL( wgtDataValue->document() );
	hlDataInfo->SetState(DAO_HLSTATE_NORMAL);
	hlDataValue->SetState(DAO_HLSTATE_NORMAL);

	currentValue = NULL;
	currentFrame = NULL;

	tokens = DArray_New( D_TOKEN );
	daoString = DString_New(1);
	daoLong = DLong_New();

	connect( wgtDataTable, SIGNAL(cellDoubleClicked(int,int)), 
			this, SLOT(slotDataTableClicked(int, int) ) );
	connect( wgtInfoTable, SIGNAL(cellDoubleClicked(int,int)), 
			this, SLOT(slotInfoTableClicked(int, int) ) );
	connect( wgtExtraTable, SIGNAL(cellDoubleClicked(int,int)), 
			this, SLOT(slotExtraTableClicked(int, int) ) );
	connect( wgtDataTable, SIGNAL(cellChanged(int,int)), 
			this, SLOT(slotElementChanged(int, int) ) );

	connect( btnUpdateValue, SIGNAL(clicked()), this, SLOT(slotUpdateValue()));
}
DaoDataWidget::~DaoDataWidget()
{
	DArray_Delete( tokens );
	DString_Delete( daoString );
	DLong_Delete( daoLong );
	delete hlDataInfo;
	delete hlDataValue;
}
void DaoDataWidget::slotDataTableClicked(int row, int col)
{
	DaoValueItem *it = NULL;
	DaoStackFrame *frame = currentFrame;
	DaoValue *value = NULL;

	currentFrame = NULL;
	if( currentValue ){
		switch( currentValue->type ){
		case DAO_PROCESS :
			if( frame ){
				value = currentValue->xProcess.activeValues[row];
			}else{
				currentFrame = currentValue->xProcess.topFrame;
				while( (row--) >0 ) currentFrame = currentFrame->prev;
				ViewStackFrame( currentFrame, (DaoProcess*) currentValue );
				it = new DaoValueItem( currentValue );
				it->frame = currentFrame;
			}
			break;
		case DAO_ARRAY : break;
		case DAO_LIST : value = currentValue->xList.items.items.pValue[row]; break;
		case DAO_MAP : value = itemValues[ 2*row + (col>=2) ]; break;
		case DAO_TUPLE : value = currentValue->xTuple.items[row]; break;
		case DAO_CLASS : value = currentValue->xClass.cstData->items.pValue[row]; break;
		case DAO_OBJECT : value = currentValue->xObject.objValues[row]; break;
		case DAO_ROUTINE : value = currentValue->xRoutine.routConsts->items.pValue[row]; break;
		case DAO_NAMESPACE : value = currentValue->xNamespace.cstData->items.pValue[row]; break;
		default : break;
		}
	}
	if( value ){
		ViewValue( value );
		it = new DaoValueItem( value );
	}
	if( it ){
		it->setText( itemName );
		wgtDataList->insertItem( 0, it );
	}
}
void DaoDataWidget::slotInfoTableClicked(int row, int)
{
	DaoValue *oldValue = currentValue;
	if( currentValue == NULL ) return;
	switch( currentValue->type ){
	case DAO_CLASS :
		ViewClass( currentValue->xClass.superClass->items.pClass[row] );
		break;
	case DAO_OBJECT :
		if( row == 0 )
			ViewClass( currentValue->xObject.defClass );
		else
			ViewValue( currentValue->xObject.parents[row-1] );
		break;
	case DAO_ROUTINE :
		switch( row ){
		case 0 : ViewNamespace( currentValue->xRoutine.nameSpace ); break;
		case 1 : ViewClass( ( DaoClass*) currentValue->xRoutine.routHost->aux ); break;
		default : break;
		}
		break;
	case DAO_PROCESS :
		if( currentFrame ){
			if( currentFrame->routine ){
				switch( row ){
				case 0 : ViewNamespace( currentFrame->routine->nameSpace ); break;
				case 1 : ViewRoutine( currentFrame->routine ); break;
				case 2 : ViewObject( currentFrame->object ); break;
				default : break;
				}
			}else{
				switch( row ){
				case 0 : ViewNamespace( currentFrame->function->nameSpace ); break;
				case 1 : ViewFunction( currentFrame->function ); break;
				case 2 : ViewObject( currentFrame->object ); break;
				default : break;
				}
			}
		}
		break;
	case DAO_NAMESPACE :
		ViewNamespace( currentValue->xNamespace.nsLoaded->items.pNS[row] );
		break;
	default : break;
	}
	// View methods may have changed "currentValue", but did not add item in the value stack:
	if( currentValue && currentValue != oldValue ){
		DaoValueItem *it = new DaoValueItem( currentValue );
		it->setText( itemName );
		wgtDataList->insertItem( 0, it );
	}
}
void DaoDataWidget::slotExtraTableClicked(int row, int col)
{
	DaoValueItem *it = NULL;
	DaoValue *value = NULL;
	if( currentValue ){
		switch( currentValue->type ){
		case DAO_PROCESS : if( currentFrame ) ResetExecutionPoint( row, col ); break;
		case DAO_CLASS : value = currentValue->xClass.glbData->items.pValue[row]; break;
		case DAO_NAMESPACE : value = currentValue->xNamespace.varData->items.pValue[row]; break;
		default : break;
		}
	}
	if( value ){
		ViewValue( value );
		it = new DaoValueItem( value );
	}
	if( it ){
		it->setText( itemName );
		wgtDataList->insertItem( 0, it );
	}
}
void DaoDataWidget::ResetExecutionPoint(int row, int col)
{
}
void DaoDataWidget::EnableNoneTable()
{
	wgtDataTable->clear();
	wgtExtraTable->clear();
	wgtValuePanel->show();
	labDataTable->setText( "" );
	wgtDataTable->hide();
	wgtInfoPanel->hide();
	wgtExtraPanel->hide();
}
void DaoDataWidget::EnableOneTable( DaoValue *p )
{
	currentValue = p;
	wgtDataValue->clear();
	wgtDataTable->clear();
	wgtExtraTable->clear();
	wgtDataTable->show();
	wgtDataPanel->show();
	wgtInfoPanel->hide();
	wgtExtraPanel->hide();
	QList<int> sizes;
	int w = width()/3;
	sizes<<w<<w+w;
	dataSplitter->setSizes( sizes );
}
void DaoDataWidget::EnableTwoTable( DaoValue *p )
{
	currentValue = p;
	wgtDataValue->clear();
	wgtDataTable->clear();
	wgtExtraTable->clear();
	wgtDataTable->show();
	wgtDataPanel->show();
	wgtInfoPanel->hide();
	wgtExtraPanel->show();
	wgtDataTable->setEditTriggers( QAbstractItemView::NoEditTriggers );
	wgtInfoTable->setEditTriggers( QAbstractItemView::NoEditTriggers );
	wgtExtraTable->setEditTriggers( QAbstractItemView::NoEditTriggers );
	QList<int> sizes;
	int w = width()/8;
	sizes<<w+w<<3*w<<3*w;
	dataSplitter->setSizes( sizes );
}
void DaoDataWidget::EnableThreeTable( DaoValue *p )
{
	currentValue = p;
	wgtDataValue->clear();
	wgtDataTable->clear();
	wgtInfoTable->clear();
	wgtExtraTable->clear();
	wgtValuePanel->hide();
	wgtDataTable->show();
	wgtDataPanel->show();
	wgtInfoPanel->show();
	wgtExtraPanel->show();
	wgtDataTable->setEditTriggers( QAbstractItemView::NoEditTriggers );
	wgtInfoTable->setEditTriggers( QAbstractItemView::NoEditTriggers );
	wgtExtraTable->setEditTriggers( QAbstractItemView::NoEditTriggers );
	QList<int> sizes;
	int w = width()/8;
	sizes<<w+w<<3*w<<3*w;
	dataSplitter->setSizes( sizes );
}
void DaoDataWidget::ViewValue( DaoValue *value )
{
	currentValue = value;

	EnableNoneTable();
	wgtDataTable->clear();
	wgtExtraTable->clear();
	//wgtDataTable->setEnabled( false );
	//wgtExtraTable->setEnabled( false );
	labDataTable->setText( "" );
	labExtraTable->setText( "" );
	DaoType *type;
	QString info = "# address:\n" + StringAddress( value ) + "\n# type:\n";
	type = DaoNamespace_GetType( nameSpace, value );
	itemName = type ? type->name->mbs : "Value";
	if( itemName == "?" ) itemName = "Value";
	itemName += "[" + StringAddress( value ) + "]";
	itemName[0] = itemName[0].toUpper();
	info += type ? type->name->mbs : "?";
	if( value->type == DAO_STRING )
		info += "\n# length:\n" + QString::number( DString_Size( value->xString.data ) );
	wgtDataInfo->setPlainText( info );

	switch( value->type ){
	case DAO_NONE :
	case DAO_INTEGER :
	case DAO_FLOAT :
	case DAO_DOUBLE :
	case DAO_COMPLEX :
	case DAO_STRING :
	case DAO_LONG :
		EnableNoneTable();
		DaoValue_GetString( value, daoString );
		wgtDataValue->setPlainText( DString_GetMBS( daoString ) );
		break;
	case DAO_ARRAY : ViewArray( (DaoArray*) value ); break;
	case DAO_LIST  : ViewList( (DaoList*) value ); break;
	case DAO_MAP   : ViewMap( (DaoMap*) value ); break;
	case DAO_TUPLE : ViewTuple( (DaoTuple*) value ); break;
	case DAO_CLASS : ViewClass( (DaoClass*) value ); break;
	case DAO_OBJECT  : ViewObject( (DaoObject*) value ); break;
	case DAO_ROUTINE : ViewRoutine( (DaoRoutine*) value ); break;
	case DAO_FUNCTION : ViewFunction( (DaoFunction*) value ); break;
	case DAO_NAMESPACE : ViewNamespace( (DaoNamespace*) value ); break;
	case DAO_PROCESS : ViewProcess( (DaoProcess*) value, currentFrame ); break;
	default: break;
	}
}
void DaoDataWidget::ViewArray( DaoArray *array )
{
}
void DaoDataWidget::ViewList( DaoList *list )
{
}
void DaoDataWidget::ViewMap( DaoMap *map )
{
}
void DaoDataWidget::ViewTuple( DaoTuple *tuple )
{
}
void DaoDataWidget::ViewClass( DaoClass *klass )
{
	EnableThreeTable( (DaoValue*) klass );
	itemName = "Class[" + QString( klass->className->mbs ) + "]";
	size_t i;
	QString info, data;
	QStringList rowlabs;
	DaoClass *sup;

	data = "# address:\n" + StringAddress( klass );
	data += "\n# type\n" + QString( klass->clsType->name->mbs );
	data += "\n# constants\n" + QString::number( klass->cstData->size );
	data += "\n# globals\n" + QString::number( klass->glbData->size );
	data += "\n# variables\n" + QString::number( klass->objDataName->size );
	if( klass->superClass->size ) data += "\n# parent(s)\n";

	wgtInfoTable->setRowCount( klass->superClass->size );
	wgtInfoTable->setColumnCount(1);
	wgtInfoTable->setColumnWidth(0, 150);
	wgtInfoTable->setHorizontalHeaderLabels( QStringList( tr("Data") ) );
	for(i=0; i<klass->superClass->size; i++){
		rowlabs<<tr("Parent Class")+QString::number(i+1);
		sup = klass->superClass->items.pClass[i];
		info = "Class[" + StringAddress( sup ) + "]";
		data += sup->className->mbs + QString("\n");
		wgtInfoTable->setItem( i, 0, new QTableWidgetItem( info ) );
	}
	wgtDataInfo->setPlainText( data );
	wgtInfoTable->setVerticalHeaderLabels( rowlabs );
	rowlabs.clear();

	labDataTable->setText( tr("Constants") );
	labExtraTable->setText( tr("Global Variables") );
	FillTable( wgtDataTable, klass->cstData->items.pValue,
			klass->cstData->size, NULL, klass->lookupTable, DAO_CLASS_CONSTANT );
	FillTable( wgtExtraTable, klass->glbData->items.pValue, klass->glbData->size, 
			klass->glbDataType, klass->lookupTable, DAO_CLASS_VARIABLE );
	for(i=0; i<klass->cstData->size; i++){
		DaoValue *val = klass->cstData->items.pValue[i];
		//XXX if( val.t != DAO_ROUTINE || val.v.routine->tidHost != DAO_CLASS ) continue;
		//XXX if( val.v.routine->routHost->aux.v.klass == klass )
		//XXX	wgtDataTable->item(i,0)->setBackground( QColor(200,250,200) );
	}
}
void DaoDataWidget::ViewObject( DaoObject *object )
{
	DaoClass *klass = object->defClass;
	EnableOneTable( (DaoValue*) object );
	wgtValuePanel->hide();
	wgtInfoPanel->show();
	itemName = "Object[" + QString( object->defClass->className->mbs ) + "]";
	size_t i;
	QString info, data;
	QStringList rowlabs;

	data = "# address:\n" + StringAddress( object );
	data += "\n# type\n" + QString( klass->className->mbs );
	data += "\n# variables\n" + QString::number( klass->objDataName->size );
	if( klass->superClass->size )
		data += "\n# parent(s)\n" + QString::number( klass->superClass->size );
	wgtDataInfo->setPlainText( data );

	wgtInfoTable->setRowCount( klass->superClass->size + 1 );
	wgtInfoTable->setColumnCount(1);
	wgtInfoTable->setColumnWidth(0, 150);
	wgtInfoTable->setHorizontalHeaderLabels( QStringList( tr("Data") ) );
	rowlabs<<tr("Class");
	info = "Class[" + StringAddress( klass ) + "]";
	wgtInfoTable->setItem( 0, 0, new QTableWidgetItem( info ) );
	for(i=0; i<klass->superClass->size; i++){
		rowlabs<<tr("Parent Object")+QString::number(i+1);
		info = "Object[" + StringAddress( object->parents[i] ) + "]";
		wgtInfoTable->setItem( i+1, 0, new QTableWidgetItem( info ) );
	}
	wgtInfoTable->setVerticalHeaderLabels( rowlabs );
	rowlabs.clear();

	labDataTable->setText( tr("Instance Variables") );
	FillTable( wgtDataTable, object->objValues, klass->objDataName->size,
			klass->objDataType, klass->lookupTable, DAO_OBJECT_VARIABLE );
}
void DaoDataWidget::ViewFunction( DaoFunction *function )
{
	itemName = "CFuntion[" + QString( function->routName->mbs ) + "]";
}
QString DaoDataWidget::RoutineInfo( DaoRoutine *routine, void *address )
{
	DaoNamespace *ns = routine->nameSpace;
	QString info;
	info += "# address:\n" + StringAddress( address );
	info += "\n# function:\n";
	info += routine->routName->mbs;
	info += "\n# type:\n";
	info += routine->routType->name->mbs;
	info += "\n\n# instructions:\n";
	info += QString::number(routine->vmCodes->size);
	info += "\n# variables:\n";
	info += QString::number(routine->regCount);
	if( ns->file->mbs ){
		info += "\n\n# path:\n\"" + QString( ns->path->mbs );
		info += "\"\n# file:\n\"" + QString( ns->file->mbs ) + "\"";
	}
	return info;
}
void DaoDataWidget::ViewVmCodes( QTableWidget *table, DaoRoutine *routine )
{
	int i, n = routine->vmCodes->size;
	QStringList rowlabs;
	rowlabs<<tr("Opcode")<<"A"<<"B"<<"C"<<tr("Line")<<tr("Notes");
	table->setRowCount( n );
	table->setColumnCount( 6 );
	table->setHorizontalHeaderLabels( rowlabs );
	rowlabs.clear();
	for(i=1; i<5; i++) table->setColumnWidth( i, 60 );
	table->setColumnWidth( 5, 200 );
	for(i=0; i<n; i++){
		DaoVmCode vmc0 = routine->vmCodes->codes[i];
		DaoVmCodeX *vmc = routine->annotCodes->items.pVmc[i];
		rowlabs<<QString::number(i);
		table->setItem( i, 0, new QTableWidgetItem( getOpcodeName( vmc0.code )) );
		table->setItem( i, 1, new QTableWidgetItem( QString::number( vmc->a )) );
		table->setItem( i, 2, new QTableWidgetItem( QString::number( vmc->b )) );
		table->setItem( i, 3, new QTableWidgetItem( QString::number( vmc->c )) );
		table->setItem( i, 4, new QTableWidgetItem( QString::number( vmc->line )) );
		DaoTokens_AnnotateCode( routine->source, *vmc, daoString, 32 );
		table->setItem( i, 5, new QTableWidgetItem( daoString->mbs ) );
		table->item( i, 0 )->setToolTip( 
				tr("Double click on an Opcode cell to change\nthe current execution point") );
		table->item( i, 0 )->setBackground( QColor(200,250,200) );
		table->item( i, 4 )->setBackground( QColor(230,250,230) );
	}
	table->setVerticalHeaderLabels( rowlabs );
}
void DaoDataWidget::ViewRoutine( DaoRoutine *routine )
{
	while( routine->revised ) routine = routine->revised;
	DMap *map = DMap_New(D_STRING,0);
	DaoToken **tokens = routine->defLocals->items.pToken;
	int i, n = routine->defLocals->size;
	for(i=0; i<n; i++) if( tokens[i]->type ==0 )
		DMap_Insert( map, tokens[i]->string, (void*)(size_t)tokens[i]->index );

	EnableThreeTable( (DaoValue*) routine );
	itemName = "Routine[" + QString( routine->routName->mbs ) + "]";
	RoutineInfo( routine, routine );

	QStringList rowlabs;
	wgtInfoTable->setRowCount( 1 + (routine->routHost && routine->routHost->tid == DAO_OBJECT) );
	wgtInfoTable->setColumnCount(1);
	wgtInfoTable->setColumnWidth(0, 150);
	rowlabs<<tr("Namespace")<<tr("Class");
	wgtInfoTable->setHorizontalHeaderLabels( QStringList( tr("Data") ) );
	wgtInfoTable->setVerticalHeaderLabels( rowlabs );
	rowlabs.clear();

	QString info = "Namespace[" + StringAddress( routine->nameSpace ) + "]";
	wgtInfoTable->setItem( 0, 0, new QTableWidgetItem( info ) );
	if( routine->routHost && routine->routHost->tid == DAO_OBJECT ){
		info = "Class[" + StringAddress( routine->routHost->aux ) + "]";
		wgtInfoTable->setItem( 1, 0, new QTableWidgetItem( info ) );
	}

	labDataTable->setText( tr("Constants") );
	FillTable( wgtDataTable, routine->routConsts->items.pValue,
			routine->routConsts->size, NULL, map, 0 );
	ViewVmCodes( wgtExtraTable, routine );
	DMap_Delete( map );
}
void DaoDataWidget::ViewStackFrame( DaoStackFrame *frame, DaoProcess *process )
{
	if( frame->routine == NULL ) return; // TODO: function

	DaoRoutine *routine = frame->routine;
	DMap *map = DMap_New(D_STRING,0);
	DaoToken **tokens = routine->defLocals->items.pToken;
	int i, n = routine->defLocals->size;
	for(i=0; i<n; i++) if( tokens[i]->type )
		DMap_Insert( map, tokens[i]->string, (void*)(size_t)tokens[i]->index );

	EnableThreeTable( (DaoValue*) process );
	wgtValuePanel->hide();
	labDataTable->setText( tr("Local Variables") );
	labInfoTable->setText( tr("Associated Data") );
	labExtraTable->setText( tr("VM Instructions") );
	nameSpace = frame->routine->nameSpace;

	QStringList rowlabs;
	int j;
	n = routine->vmCodes->size;
	vmcEntry = vmcNewEntry = frame->entry;

	itemName = "StackFrame[" + QString( routine->routName->mbs ) + "]";

	RoutineInfo( routine, frame );

	wgtInfoTable->setRowCount( 2 + (frame->object != NULL) );
	wgtInfoTable->setColumnCount(1);
	wgtInfoTable->setColumnWidth(0, 150);
	rowlabs<<tr("Namespace")<<tr("Routine");
	if( frame->object ) rowlabs<<tr("Object");
	wgtInfoTable->setHorizontalHeaderLabels( QStringList( tr("Data") ) );
	wgtInfoTable->setVerticalHeaderLabels( rowlabs );
	rowlabs.clear();

	QString info = "Namespace[" + StringAddress( nameSpace ) + "]";
	wgtInfoTable->setItem( 0, 0, new QTableWidgetItem( info ) );
	info = "Routine[" + StringAddress( routine ) + "]";
	wgtInfoTable->setItem( 1, 0, new QTableWidgetItem( info ) );
	if( frame->object ){
		info = "Object[" + StringAddress( frame->object ) + "]";
		wgtInfoTable->setItem( 2, 0, new QTableWidgetItem( info ) );
	}
	if( routine->vmCodes->size ==0 ) return;

	FillTable2( wgtDataTable, process->stackValues + frame->stackBase, routine->regCount, 
			routine->regType, map );
	ViewVmCodes( wgtExtraTable, frame->routine );
	for(j=1; j<4; j++)
		wgtExtraTable->item( vmcEntry, j )->setBackground( QColor(200,200,250) );
	QIcon icon( QPixmap( ":/images/start.png" ) );
	wgtExtraTable->item( vmcEntry, 0 )->setIcon( icon );
	DMap_Delete( map );
}
void DaoDataWidget::FillTable( QTableWidget *table, 
		DaoValue **data, int size, DArray *type, DMap *names, int filter )
{
	DNode *node;
	DaoType *itp = NULL;
	QTableWidgetItem *it;
	QStringList headers, rowlabs;
	QMap<int,QString> idnames;
	int i;
	table->setRowCount( size );
	table->setColumnCount( 3 );
	headers<<tr("Name")<<tr("Type")<<tr("Value");
	table->setHorizontalHeaderLabels( headers );
	table->setColumnWidth( 0, 150 );
	table->setColumnWidth( 1, 150 );
	table->setColumnWidth( 2, 300 );
	if( names ){
		for(node=DMap_First(names); node!=NULL; node=DMap_Next(names,node)){
			if( filter && LOOKUP_ST( node->value.pInt ) != filter ) continue;
			if( LOOKUP_UP( node->value.pInt ) ) continue;
			// LOOKUP_ID: get the last 16 bits
			idnames[ LOOKUP_ID( node->value.pInt ) ] = node->key.pString->mbs;
		}
	}
	for(i=0; i<size; i++){
		DaoValue *val = data[i];
		rowlabs<<QString::number(i);
		if( val == NULL ){
			it = new QTableWidgetItem( "none" );
			table->setItem( i, 0, it );
		}else if( idnames.find(i) != idnames.end() ){
			it = new QTableWidgetItem( idnames[i] );
			table->setItem( i, 0, it );
		}else if( val->type == DAO_CLASS ){
			it = new QTableWidgetItem( val->xClass.className->mbs );
			table->setItem( i, 0, it );
		}else if( val->type == DAO_ROUTINE || val->type == DAO_FUNCTION ){
			it = new QTableWidgetItem( val->xRoutine.routName->mbs );
			table->setItem( i, 0, it );
		}
		itp = type ? type->items.pType[i] : NULL;
		if( itp == NULL ) itp = DaoNamespace_GetType( nameSpace, val );
		if( itp ){
			it = new QTableWidgetItem( itp->name->mbs );
			table->setItem( i, 1, it );
		}
		if( val ) DaoValue_GetString( val, daoString );
		if( DString_Size( daoString ) > 50 ) DString_Erase( daoString, 50, (size_t)-1 );
		it = new QTableWidgetItem( val ? DString_GetMBS( daoString ) : "null" );
		table->setItem( i, 2, it );
	}
	table->setVerticalHeaderLabels( rowlabs );
}
void DaoDataWidget::FillTable2( QTableWidget *table, 
		DaoValue **data, int size, DArray *type, DMap *names )
{
	DNode *node;
	DaoType *itp = NULL;
	QTableWidgetItem *it;
	QStringList headers, rowlabs;
	QMap<int,QString> idnames;
	int i;
	table->setRowCount( size );
	table->setColumnCount( 3 );
	headers<<tr("Name")<<tr("Type")<<tr("Value");
	table->setHorizontalHeaderLabels( headers );
	table->setColumnWidth( 0, 150 );
	table->setColumnWidth( 1, 150 );
	table->setColumnWidth( 2, 300 );
	if( names ){
		for(node=DMap_First(names); node!=NULL; node=DMap_Next(names,node))
			idnames[ node->value.pInt ] = node->key.pString->mbs;
	}
	for(i=0; i<size; i++){
		DaoValue *val = data[i];
		rowlabs<<QString::number(i);
		if( val == NULL ){
			it = new QTableWidgetItem( "none" );
			table->setItem( i, 0, it );
		}else if( idnames.find(i) != idnames.end() ){
			it = new QTableWidgetItem( idnames[i] );
			table->setItem( i, 0, it );
		}else if( val->type == DAO_CLASS ){
			it = new QTableWidgetItem( val->xClass.className->mbs );
			table->setItem( i, 0, it );
		}else if( val->type == DAO_ROUTINE || val->type == DAO_FUNCTION ){
			it = new QTableWidgetItem( val->xRoutine.routName->mbs );
			table->setItem( i, 0, it );
		}
		itp = type ? type->items.pType[i] : NULL;
		if( itp == NULL ) itp = DaoNamespace_GetType( nameSpace, val );
		if( itp ){
			it = new QTableWidgetItem( itp->name->mbs );
			table->setItem( i, 1, it );
		}
		if( val ) DaoValue_GetString( val, daoString );
		if( DString_Size( daoString ) > 50 ) DString_Erase( daoString, 50, (size_t)-1 );
		it = new QTableWidgetItem( val ? DString_GetMBS( daoString ) : "none" );
		table->setItem( i, 2, it );
	}
	table->setVerticalHeaderLabels( rowlabs );
}
void DaoDataWidget::ViewNamespace( DaoNamespace *nspace )
{
	EnableThreeTable( (DaoValue*) nspace );
	labDataTable->setText( tr("Constants") );
	labExtraTable->setText( tr("Variables") );
	nameSpace = nspace;
	itemName = "Namespace[" + StringAddress( nspace ) + "]";

	size_t i;
	QTableWidgetItem *it;
	QStringList headers;
	QString info;
	info += "# address:\n" + StringAddress( nspace );
	info += "\n# constants:\n";
	info += QString::number( nspace->cstData->size );
	info += "\n# variables:\n";
	info += QString::number( nspace->varData->size );
	if( nspace->file->mbs ){
		info += "\n\n# path:\n\"" + QString( nspace->path->mbs );
		info += "\"\n# file:\n\"" + QString( nspace->file->mbs ) + "\"";
	}
	wgtDataInfo->setPlainText( info );
	wgtDataValue->clear();

	wgtInfoTable->setRowCount( nspace->nsLoaded->size );
	wgtInfoTable->setColumnCount( 2 );
	headers<<tr("Loaded Modules")<<tr("Module Files");
	wgtInfoTable->setHorizontalHeaderLabels( headers );
	for(i=0; i<nspace->nsLoaded->size; i++){
		DaoNamespace *ns = nspace->nsLoaded->items.pNS[i];
		it = new QTableWidgetItem( "Namespace[" + StringAddress(ns) + "]" );
		wgtInfoTable->setItem( i, 0, it );
		it = new QTableWidgetItem( ns->name->mbs );
		wgtInfoTable->setItem( i, 1, it );
	}

	FillTable( wgtDataTable, nspace->cstData->items.pValue, nspace->cstData->size,
			NULL, nspace->lookupTable, DAO_GLOBAL_CONSTANT );
	FillTable( wgtExtraTable, nspace->varData->items.pValue, nspace->varData->size,
			nspace->varType, nspace->lookupTable, DAO_GLOBAL_VARIABLE );
	for(i=0; i<nspace->cstData->size; i++){
		DaoValue *val = nspace->cstData->items.pValue[i];
		it = wgtDataTable->item(i,0);
		if( it == NULL ) continue;
		if( val->type == DAO_CLASS && val->xClass.classRoutine->nameSpace == nspace )
			it->setBackground( QColor(200,250,200) );
		else if( val->type == DAO_ROUTINE && val->xRoutine.nameSpace == nspace )
			it->setBackground( QColor(200,250,200) );
	}
}
void DaoDataWidget::ViewProcess( DaoProcess *process, DaoStackFrame *frame )
{
	currentFrame = frame;
	if( frame ){
		ViewStackFrame( frame, process );
		return;
	}
	EnableOneTable( (DaoValue*) process );
	nameSpace = process->activeNamespace;

	int index = 0;
	frame = process->topFrame;
	while(frame && frame != process->firstFrame) {
		frame = frame->prev;
		index++;
	}

	QString info;
	info = "# address:\n" + StringAddress( process );
	info += "\n# type:\nVM Process\n# frames:\n";
	info += QString::number( process->topFrame ? index : 10 );
	wgtDataInfo->setPlainText( info );
	wgtDataValue->clear();

	labDataTable->setText( tr("Function Call Stack") );
	wgtDataTable->setRowCount( index );
	wgtDataTable->setColumnCount( 2 );
	wgtDataTable->setColumnWidth( 0, 200 );
	wgtDataTable->setColumnWidth( 1, 300 );

	QStringList headers;
	headers<<tr("Function Name")<<tr("Function Type");
	wgtDataTable->setHorizontalHeaderLabels( headers );
	wgtDataTable->setEditTriggers( QAbstractItemView::NoEditTriggers );

	if( process->topFrame == NULL ) return;

	QTableWidgetItem *it = NULL;
	int i;
	frame = process->topFrame;
	for(i=0; frame && frame!=process->firstFrame; i++, frame=frame->prev){
		if( frame->routine ){
			it = new QTableWidgetItem( frame->routine->routName->mbs );
			wgtDataTable->setItem( i, 0, it );
			it = new QTableWidgetItem( frame->routine->routType->name->mbs );
			wgtDataTable->setItem( i, 1, it );
		}else{
			it = new QTableWidgetItem( frame->function->routName->mbs );
			wgtDataTable->setItem( i, 0, it );
			it = new QTableWidgetItem( frame->function->routType->name->mbs );
			wgtDataTable->setItem( i, 1, it );
		}
	}
}
extern "C"{
//DaoValue* DaoParseNumber( DaoToken *tok, DLong *bigint, DaoComplex *buffer );
}
void DaoDataWidget::slotUpdateValue()
{
	if( currentValue ==NULL ) return;
	DaoToken *tok = NULL;
	DaoValue *number, *value = currentValue;
	QByteArray text = wgtDataValue->toPlainText().toUtf8();
	bool ok = true;
	if( value->type && (value->type <= DAO_DOUBLE || value->type == DAO_LONG) ){
		DaoToken_Tokenize( tokens, text.data(), 0, 1, 0 );
		if( tokens->size ) tok = tokens->items.pToken[0];
		if( tokens->size !=1 || tok->type <DTOK_DIGITS_HEX || tok->type >DTOK_NUMBER_SCI ){
			QMessageBox::warning( this, tr("DaoStudio"),
					tr("Invalid value for the data type!"), QMessageBox::Cancel );
			return;
		}
		//XXX number = DaoParseNumber( tok, daoLong );
	}
	switch( value->type ){
	case DAO_INTEGER : value->xInteger.value = DaoValue_GetInteger( number ); break;
	case DAO_FLOAT   : value->xFloat.value = DaoValue_GetFloat( number ); break;
	case DAO_DOUBLE  : value->xDouble.value = DaoValue_GetDouble( number ); break;
	case DAO_COMPLEX :
					  break;
	case DAO_STRING :
					  DString_SetDataMBS( value->xString.data, text.data(), text.size() );
					  break;
	case DAO_LONG :
					  break;
	default : break;
	}
	if( ok ){
		ViewValue( currentValue );
	}else{
		QMessageBox::warning( this, tr("DaoStudio"),
				tr("Invalid value for the data type!"), QMessageBox::Cancel );
	}
}
void DaoDataWidget::slotElementChanged(int row, int col)
{
	if( currentValue == NULL || currentValue->type != DAO_ARRAY ) return;
	int id = row * wgtDataTable->columnCount() + col;
	DaoValue *number;
	DaoToken *tok = NULL;
	DaoArray *array = (DaoArray*) currentValue;
	QByteArray text = wgtDataTable->item(row,col)->text().toLocal8Bit();
	DaoToken_Tokenize( tokens, text.data(), 0, 1, 0 );
	if( array->etype >= DAO_INTEGER && array->etype <= DAO_DOUBLE ){
		if( tokens->size ) tok = tokens->items.pToken[0];
		if( tokens->size !=1 || tok->type <DTOK_DIGITS_HEX || tok->type >DTOK_NUMBER_SCI ){
			QMessageBox::warning( this, tr("DaoStudio"),
					tr("Invalid value for the data type!"), QMessageBox::Cancel );
			return;
		}
		//XXX number = DaoParseNumber( tok, daoLong );
	}
	switch( array->etype ){
	case DAO_INTEGER :
		array->data.i[id] = DaoValue_GetInteger( number );
		break;
	case DAO_FLOAT :
		array->data.f[id] = DaoValue_GetFloat( number );
		break;
	case DAO_DOUBLE :
		array->data.d[id] = DaoValue_GetDouble( number );
		break;
	case DAO_COMPLEX : // TODO
		break;
	}
}
#endif

static int DaoEditContinueData( QByteArray &data, QList<int> & lineMap, QStringList & newCodes, QStringList & routCodes )
{
	QList<QByteArray> lines = data.split( '\n' );
	if( lines.size() <= 2 ) return lines[0].toInt();

	//printf( "lines: %i\n", lines.size() );

	int entry = lines[0].toInt();
	int size1 = lines[1].toInt();
	int size2 = lines[2].toInt();
	int size3 = lines[3].toInt();
	int i;

	lineMap.clear();
	newCodes.clear();
	routCodes.clear();
	for(i=0; i<size1; i++) lineMap.append( lines[i+4].toInt() );
	for(i=0; i<size2; i++) newCodes.append( lines[i+4+size1] );
	for(i=0; i<size3; i++) routCodes.append( lines[i+4+size1+size2] );
	return entry;
}
static void DaoResetExecution( DaoProcess *process, int line, int offset=0 )
{
	DaoRoutine *routine = process->activeRoutine;
	DaoVmCodeX **annotCodes = routine->annotCodes->items.pVmc;
	int i = 0, n = routine->annotCodes->size;
	while( i < n && annotCodes[i]->line < line ) ++i;
	while( i < n && offset >0 && annotCodes[i]->line == line){
		++i;
		--offset;
	}
	process->status = DAO_VMPROC_STACKED;
	process->topFrame->entry = i;
	//DaoVmCodeX_Print( *annotCodes[i], NULL );
	//printf( "entry: %s\n", annotCodes[i]->annot->mbs->data );
}
static void DaoStdioRead( DaoEventHandler *self, DString *buf, int count )
{
	self->interpreter->mutex.lock();
	fflush( stdout );
	self->socket2.flush();
	self->socket.connectToServer( DaoStudioSettings::socket_stdin );
	self->socket.write( QByteArray::number( count ) );
	self->socket.flush();
	QObject::connect( & self->socket, SIGNAL(disconnected()),
			self->interpreter, SLOT(slotExitWaiting()) );
	QByteArray data;
	self->interpreter->waiting = true;
	while( self->interpreter->waiting ){
		QCoreApplication::processEvents( QEventLoop::AllEvents, 1000 );
		fflush( stdout );
		if( self->process->stopit ) break;
	}
	self->socket.waitForReadyRead();
	data += self->socket.readAll();
	//QString s = QString::fromUtf8( data.data(), data.size() );
	DString_SetDataMBS( buf, data.data(), data.size() );
	self->interpreter->mutex.unlock();
}
static void DaoStdioWrite( DaoEventHandler *self, DString *buf )
{
#if 0
	FILE *fout = fopen( "output.txt", "a+" );
	fprintf( fout, "%s\n", DString_GetMBS( buf ) );
	fclose( fout );
#endif
	self->interpreter->mutex.lock();
	self->socket2.write( DString_GetMBS( buf ), DString_Size( buf ) );
	self->socket2.flush();
	self->interpreter->mutex.unlock();
}
static void DaoConsDebug( DaoEventHandler *self, DaoProcess *process )
{
	self->interpreter->mutex.lock();

	DaoVmCode *codes = process->activeRoutine->vmCodes->codes;
	DaoVmCodeX **annots = process->activeRoutine->annotCodes->items.pVmc;
	int oldline = annots[ process->activeCode - codes + 1]->line;
	QFileInfo fi( process->activeRoutine->nameSpace->name->mbs );
	QString name = fi.absoluteFilePath();
	QByteArray start = QByteArray::number( process->activeRoutine->bodyStart+1 );
	QByteArray end = QByteArray::number( process->activeRoutine->bodyEnd-1 );
	QByteArray next = QByteArray::number( oldline );
	QByteArray send;
	send.append( name );
	send += '\0' + start + '\0' + end + '\0' + next;

	QString info = QObject::tr("Entering debug mode, please click the debug button to resume");
	info += " ...";
	printf( "%s\n", info.toUtf8().data() );
	fflush( stdout );
	self->socket.connectToServer( DaoStudioSettings::socket_debug );
	self->socket.write( send );
	self->socket.flush();
	QObject::connect( & self->socket, SIGNAL(disconnected()),
			self->interpreter, SLOT(slotExitWaiting()) );
	QByteArray data;
	self->interpreter->waiting = true;
	self->interpreter->debugProcess = process;
	self->interpreter->InitDataBrowser();
	while( self->interpreter->waiting ){
		QCoreApplication::processEvents( QEventLoop::AllEvents, 1000 );
		fflush( stdout );
	}
	//while( self->socket.state() != QLocalSocket::UnconnectedState ){
	self->socket.waitForReadyRead();
	data += self->socket.readAll();
	//self->socket.waitForReadyRead();
	//data += self->socket.readAll();
	//}
	//printf( "all:\n%s\n", QString(data).toUtf8().data() );

	DaoDebugger & debugger = self->debugger;
	QList<int> lineMap;
	QStringList newCodes, routCodes;
	DaoRoutine *old = process->activeRoutine;
	int entry = DaoEditContinueData( data, lineMap, newCodes, routCodes );
	//printf( "all:\n%s\n", QString(data).toUtf8().data() );
	//printf( "new:\n%s\n", newCodes.join("\n").toUtf8().data() );
	//printf( "old:\n%s\n", routCodes.join("\n").toUtf8().data() );
	if( debugger.EditContinue( process, entry, lineMap, newCodes, routCodes ) ){
		if( old == process->activeRoutine && entry != oldline )
			DaoResetExecution( process, entry );
	}
	//printf( "%i %s\n", data.size(), data.data() );
	//QMessageBox::about( self->interpreter, "test", "test" );

	self->interpreter->mutex.unlock();
}
static void DaoSetBreaks( DaoEventHandler *self, DaoRoutine *routine )
{
	self->debugger.SetBreakPoints( routine );
}
static void DaoProcessMonitor( DaoEventHandler *self, DaoProcess *process )
{
	if( self->timer.time > self->time ){
		//printf( "time: %i  %i\n", self->timer.time, self->time );
		//fflush( stdout );
		self->time = self->timer.time;
		if( process != self->process ) return;
		QApplication::processEvents( QEventLoop::AllEvents, TIME_EVENT );
		fflush( stdout );
	}
}


DaoInterpreter::DaoInterpreter( const char *cmd ) : QObject()
{
	int i;

	locale = DaoStudioSettings::locale;
	program = DaoStudioSettings::program;
	programPath = DaoStudioSettings::program_path;

	QCommonStyle style;
	QIcon book( QPixmap( ":/images/book.png" ) );
	QIcon dao( QPixmap( ":/images/dao.png" ) );
	QIcon daomonitor( QPixmap( ":/images/daomonitor.png" ) );
	QApplication::setWindowIcon( daomonitor );

	index = 0;
	vmState = DAOCON_READY;
	debugProcess = NULL;
	itemValues = NULL;
	itemCount = 0;
	daoString = DString_New(1);
	daoLong = DLong_New();
	tokens = DArray_New( D_TOKEN );

	dataSocket = NULL;
	scriptSocket = NULL;

	handler.time = 0;
	handler.interpreter = this;
	handler.stdRead = DaoStdioRead;
	handler.stdWrite = DaoStdioWrite;
	handler.stdFlush = NULL;
	handler.debug = DaoConsDebug;
	handler.breaks = DaoSetBreaks;
	handler.Called = NULL;
	handler.Returned = NULL;
	handler.InvokeHost = DaoProcessMonitor;
	handler.timer.start();

	vmSpace = DaoInit();
	vmSpace->options |= DAO_EXEC_IDE | DAO_EXEC_INTERUN;
	vmSpace->mainNamespace->options |= DAO_EXEC_IDE | DAO_EXEC_INTERUN;
	nameSpace = vmSpace->mainNamespace;
	handler.process = DaoVmSpace_MainProcess( vmSpace );
	DaoVmSpace_SetUserHandler( vmSpace, (DaoUserHandler*) & handler );
	DString_SetMBS( vmSpace->fileName, "interactive codes" );
	DString_SetMBS( vmSpace->mainNamespace->name, "interactive codes" );

	DaoNamespace *ns = vmSpace->mainNamespace;
	codeType = DaoParser_ParseTypeName( CODE_ITEM_TYPE, ns, NULL );
	valueType = DaoParser_ParseTypeName( VALUE_ITEM_TYPE, ns, NULL );
	extraType = DaoParser_ParseTypeName( EXTRA_ITEM_TYPE, ns, NULL );
	messageType = DaoParser_ParseTypeName( VALUE_INFO_TYPE, ns, NULL );
	codeTuple = DaoTuple_Create( codeType, 1 );
	valueTuple = DaoTuple_Create( valueType, 1 );
	extraTuple = DaoTuple_Create( extraType, 1 );
	messageTuple = DaoTuple_Create( messageType, 1 );
	numArray = DaoArray_New( DAO_INTEGER );
	extraList = DaoList_New();
	constList = DaoList_New();
	varList = DaoList_New();
	codeList = DaoList_New();
	valueStack = DaoList_New();
	extraStack = DArray_New(0);

	DaoValue_MarkConst( (DaoValue*)codeTuple );
	DaoValue_MarkConst( (DaoValue*)valueTuple );
	DaoValue_MarkConst( (DaoValue*)extraTuple );
	DaoTuple_SetItem( messageTuple, (DaoValue*)numArray, INDEX_NUMBERS );
	DaoTuple_SetItem( messageTuple, (DaoValue*)extraList, INDEX_EXTRAS );
	DaoTuple_SetItem( messageTuple, (DaoValue*)constList, INDEX_CONSTS );
	DaoTuple_SetItem( messageTuple, (DaoValue*)varList, INDEX_VARS );
	DaoTuple_SetItem( messageTuple, (DaoValue*)codeList, INDEX_CODES );

	SetPathWorking( "." );
	DaoVmSpace_AddPath( vmSpace, programPath.toLocal8Bit().data() );

	if( QFile::exists( DaoStudioSettings::socket_data ) )
		QFile::remove( DaoStudioSettings::socket_data );
	dataServer.listen( DaoStudioSettings::socket_data );
	connect( &dataServer, SIGNAL(newConnection()), this, SLOT(slotServeData()));

	if( QFile::exists( DaoStudioSettings::socket_script ) )
		QFile::remove( DaoStudioSettings::socket_script );
	scriptServer.listen( DaoStudioSettings::socket_script );
	connect( &scriptServer, SIGNAL(newConnection()), this, SLOT(slotStartExecution()));

	time = 0;
	//connect( &timer, SIGNAL(timeout()), this, SLOT(slotFlushStdout()));
	//timer.start( 100 );

	QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
	env.insert( "daostudio", "yes" );

	shell = new QProcess();
	shell->setProcessEnvironment(env);
	connect( shell, SIGNAL(readyReadStandardOutput()),
			this, SLOT(slotReadStdOut()));
	//connect( shell, SIGNAL(readyReadStandardError()),
	//	this, SLOT(slotReadStdOut()));
	connect( shell, SIGNAL(finished(int, QProcess::ExitStatus)),
			this, SLOT(slotShellFinished(int, QProcess::ExitStatus)));
}

DaoInterpreter::~DaoInterpreter()
{
	shell->close();
}

void DaoInterpreter::SetPathWorking( const QString & path )
{
	QDir dir( path );
	pathWorking = dir.absolutePath() + "/";
	DaoVmSpace_SetPath( vmSpace, pathWorking.toLocal8Bit().data() );
}
void DaoInterpreter::slotReadStdOut()
{
	shell->setReadChannel( QProcess::StandardOutput );
	while( not shell->atEnd() ){
		QByteArray output = shell->readLine();
#if 0
		char *data = output.data();
		int i, n = output.size();
		for(i=0; i<n; i++) printf( "%c", data[i] );
		fflush( stdout );
#endif
		handler.socket2.write( output.data(), output.size() );
		handler.socket2.flush();
	}
	shell->setReadChannel( QProcess::StandardError );
	while( not shell->atEnd() ){
		QByteArray output = shell->readLine();
#if 0
		char *data = output.data();
		int i, n = output.size();
		for(i=0; i<n; i++) printf( "%c", data[i] );
		fflush( stdout );
#endif
		handler.socket2.write( output.data(), output.size() );
		handler.socket2.flush();
	}
}
void DaoInterpreter::slotShellFinished(int, QProcess::ExitStatus)
{
	handler.process->stopit = 1;
}
void DaoInterpreter::slotServeData()
{
	dataSocket = dataServer.nextPendingConnection();
	dataSocket->waitForReadyRead(1000);
	QByteArray data = dataSocket->readAll();
	if( data.size() ==0 ){
		dataSocket->disconnectFromServer();
		return;
	}
	DaoValue *value = NULL;
	DString_SetDataMBS( daoString, data.data(), data.size() );
	if( DaoValue_Deserialize( & value, daoString, vmSpace->mainNamespace, NULL ) == 0 ) return;

	DaoTuple *request = (DaoTuple*) value;
	value = valueStack->items.items.pValue[0];
	switch( value->type ){
	case DAO_PROCESS :
		if( extraStack->items.pValue[0] ){
			ViewStackData( (DaoProcess*)value, (DaoStackFrame*)extraStack->items.pValue[0], request );
		}else{
			ViewProcessStack( (DaoProcess*)value, request );
		}
		break;
	case DAO_NAMESPACE : ViewNamespaceData( (DaoNamespace*)value, request ); break;
	}
}
void DaoInterpreter::slotStartExecution()
{
	if( vmState == DAOCON_RUN ){
		//printf( "is running\n" );
		fflush( stdout );
		return;
	}

	scriptSocket = scriptServer.nextPendingConnection();
	scriptSocket->waitForReadyRead(1000);
	QByteArray script = scriptSocket->readAll();
	if( script.size() ==0 ){
		scriptSocket->disconnectFromServer();
		return;
	}
	char info = script[0];
	script.remove(0,1);
	if( info == DAO_SET_PATH ){
		DaoVmSpace_SetPath( vmSpace, script.data() );
		QDir::setCurrent( QString::fromUtf8( script.data(), script.size() ) );
		scriptSocket->disconnectFromServer();
		return;
	}
	handler.socket2.connectToServer( DaoStudioSettings::socket_stdout );
	if( handler.socket2.waitForConnected( 1000 ) ==0 ){
		printf( "cannot connect to console stdout\n" );
		fflush( stdout );
		scriptSocket->disconnectFromServer();
		return;
	}

	if( info )
		vmSpace->options |= DAO_EXEC_DEBUG;
	else
		vmSpace->options &= ~DAO_EXEC_DEBUG;

	vmState = DAOCON_RUN;
	DaoProcess *vmp = DaoVmSpace_MainProcess( vmSpace );
	DaoNamespace *ns = DaoVmSpace_MainNamespace( vmSpace );
	ns->options |= DAO_EXEC_IDE | DAO_NS_AUTO_GLOBAL;

	connect( & handler.socket2, SIGNAL( disconnected() ), this, SLOT( slotStopExecution() ) );

	vmp->stopit = 0;
	handler.process = vmp;
	QTime time;
	time.start();
	int res = 0;
	if( script[0] == '\\' ){
		shell->start( script.mid(1).data(), QIODevice::ReadWrite | QIODevice::Unbuffered );
		/* BUG
		   while( not shell->waitForFinished( 100 ) ){
		   QApplication::processEvents( QEventLoop::AllEvents, 100 );
		   }
		   program run in QProcess will crash!
		 */
		while(1){// not shell->waitForFinished( 100 ) )
			//usleep( 100 );
			//waitForReadyRead
			//shell->waitForReadyRead( 100 );
			//DaoProcessMonitor( & handler );
			//QApplication::processEvents( QEventLoop::WaitForMoreEvents, 100 );
			Sleeper::Sleep( 10000 );
			QApplication::processEvents( QEventLoop::AllEvents, 20 );
			if( vmp->stopit ){
				shell->kill();
				break;
			}
		}
		slotReadStdOut();
		res = shell->exitStatus();
		//printf( "stop: %i, status: %i, code: %i, error: %i\n", 
		//	vmSpace->stopit, res, shell->exitCode(), shell->error() );
	}else{
		DString *mbs = DString_New(1);
		DString_AppendDataMBS( mbs, script.data(), script.size() );
		res = (int) DaoProcess_Eval( vmp, ns, mbs, 1 );
		DaoCallServer_Join();
		DString_Delete( mbs );
	}
	fflush( stdout );
	handler.socket2.flush();
	handler.socket2.disconnectFromServer();

	QLocalSocket socket;
	socket.connectToServer( DaoStudioSettings::socket_logger );
	socket.waitForConnected( 1000 );
	int ms = time.elapsed();
	int sec = ms / 1000;
	int min = sec / 60;
	int hr = min / 60;
	char buf[100];
	ms -= sec * 1000;
	sec -= min * 60;
	min -= hr * 60;
	sprintf( buf, "%s: %02i:%02i:%02i.%03i", 
			tr("execution time").toUtf8().data(), hr, min, sec, ms );
	QString status = QString::number( res ) + '\1';
	status += QString::fromUtf8( buf, strlen( buf ) );
	socket.write( status.toUtf8() );
	socket.flush();
	socket.disconnectFromServer();
	//XXX ReduceValueItems( wgtDataList->item( wgtDataList->count()-1 ) );
	EraseDebuggingProcess();
	ViewNamespace( vmSpace->mainNamespace );
	messageTuple->items[INDEX_INIT]->xInteger.value = 1;
	SendDataInformation();
	//ViewValue( dataWidget, (DaoValueItem*) wgtDataList->item( 0 ) );
	//connect( &scriptServer, SIGNAL(newConnection()), this, SLOT(slotStartExecution()));
	vmState = DAOCON_READY;

#if 0
	FILE *fout = fopen( "debug.txt", "a+" );
	fprintf( fout, "%s\n", buf );
	fclose( fout );
#endif
}
void DaoInterpreter::slotStopExecution()
{
	//QMessageBox::about( this, "","" );
	DaoProcess *vmp = DaoVmSpace_MainProcess( vmSpace );
	vmp->stopit = 1;
}
void DaoInterpreter::slotExitWaiting()
{
	waiting = false;
}
void DaoInterpreter::slotFlushStdout()
{
	time += 1;
	fflush( stdout );
}
void DaoInterpreter::SendDataInformation()
{
	monitorSocket.connectToServer( DaoStudioSettings::socket_monitor );
	if( monitorSocket.waitForConnected( 1000 ) ==0 ){
		printf( "cannot connect to the virtual machine monitor\n" );
		fflush( stdout );
		return;
	}
	if( DaoValue_Serialize( (DaoValue*) messageTuple, daoString, vmSpace->mainNamespace, NULL )){
		//printf( "%s\n", daoString->mbs ); fflush(stdout);
		monitorSocket.write( daoString->mbs );
		monitorSocket.flush();
		monitorSocket.disconnectFromServer();
	}
}
void DaoInterpreter::InitDataBrowser()
{
	ViewNamespace( vmSpace->mainNamespace );
	messageTuple->items[INDEX_INIT]->xInteger.value = 1;
	SendDataInformation();
	ViewProcess( debugProcess );
	messageTuple->items[INDEX_INIT]->xInteger.value = 0;
	SendDataInformation();
}
#if 0
void DaoInterpreter::ViewValue( DaoDataWidget *dataView, DaoValueItem *it )
{
	switch( it->type ){
	case DAO_ARRAY  : dataView->ViewArray( it->array ); break;
	case DAO_LIST   : dataView->ViewList(  it->list  ); break;
	case DAO_MAP	: dataView->ViewMap(   it->map   ); break;
	case DAO_TUPLE  : dataView->ViewTuple( it->tuple ); break;
	case DAO_CLASS  : dataView->ViewClass( it->klass ); break;
	case DAO_OBJECT : dataView->ViewObject( it->object ); break;
	case DAO_ROUTINE : dataView->ViewRoutine( it->routine ); break;
	case DAO_NAMESPACE : dataView->ViewNamespace( it->nspace ); break;
	case DAO_PROCESS : dataView->ViewProcess( it->process, it->frame ); break;
	default : dataView->ViewValue( it->value ); break;
	}
}
void DaoInterpreter::ReduceValueItems( QListWidgetItem *item )
{
	QList<DaoValueItem*> items;
	DaoValueItem *it;
	int i, id = wgtDataList->row( item );
	for(i=0; i<id; i++){
		it = (DaoValueItem*) wgtDataList->item(i);
		if( it->dataWidget == NULL ) items.append( it );
	}
	for(i=0; i<items.size(); i++){
		it = items[i];
		id = wgtDataList->row( it );
		it = (DaoValueItem*) wgtDataList->takeItem(0);
		delete it;
	}
}
#endif
void DaoInterpreter::EraseDebuggingProcess()
{
	DaoList_Clear( valueStack );
	DArray_Clear( extraStack );
#if 0
	DaoValueItem *it;
	int i;
	for(i=0; i<wgtDataList->count(); i++){
		it = (DaoValueItem*) wgtDataList->item(i);
		if( it->type == DAO_PROCESS && it->text() == "Process[Debugging]" ){
			it = (DaoValueItem*) wgtDataList->takeItem(i);
			delete it;
			break;
		}
	}
#endif
}
#if 0
void DaoInterpreter::slotValueActivated( QListWidgetItem *item )
{
	ReduceValueItems( item );
	ViewValue( dataWidget, (DaoValueItem*) item );
}
#endif

void DaoInterpreter::InitMessage( DaoValue *value )
{
	DaoList_PushFront( valueStack, value );
	DArray_PushFront( extraStack, NULL );
	messageTuple->items[INDEX_ADDR]->xInteger.value = (dint) value;
	messageTuple->items[INDEX_TYPE]->xInteger.value = value->type;
	DaoArray_ResizeVector( numArray, 0 );
	DaoTuple_SetItem( messageTuple, (DaoValue*)numArray, INDEX_NUMBERS );
	DaoList_Clear( extraList );
	DaoList_Clear( constList );
	DaoList_Clear( varList );
	DaoList_Clear( codeList );
	DString_Clear( messageTuple->items[INDEX_VALUE]->xString.data );
}
void DaoInterpreter::ViewValue( DaoValue *value )
{
	int ok = true;
	if( value == NULL ) return;

	DaoType *type = DaoNamespace_GetType( vmSpace->mainNamespace, value );
	QString info = "# address:\n" + StringAddress( value ) + "\n# type:\n";
	QString itemName = type ? type->name->mbs : "Value";
	if( itemName == "?" ) itemName = "Value";
	itemName += "[" + StringAddress( value ) + "]";
	itemName[0] = itemName[0].toUpper();
	info += type ? type->name->mbs : "?";
	if( value->type == DAO_STRING )
		info += "\n# length:\n" + QString::number( DString_Size( value->xString.data ) );

	switch( value->type ){
	case DAO_NONE :
	case DAO_INTEGER :
	case DAO_FLOAT :
	case DAO_DOUBLE :
	case DAO_COMPLEX :
	case DAO_LONG :
	case DAO_STRING :
		InitMessage( value );
		DaoValue_GetString( value, daoString );
		DString_SetMBS( messageTuple->items[INDEX_NAME]->xString.data, itemName.toUtf8().data() );
		DString_SetMBS( messageTuple->items[INDEX_INFO]->xString.data, info.toUtf8().data() );
		DString_SetMBS( messageTuple->items[INDEX_VALUE]->xString.data, DString_GetMBS( daoString ) );
		break;
	case DAO_ARRAY : ViewArray( (DaoArray*)value ); break;
	case DAO_TUPLE : ViewTuple( (DaoTuple*)value ); break;
	case DAO_LIST : ViewList( (DaoList*)value ); break;
	case DAO_MAP : ViewMap( (DaoMap*)value ); break;
	case DAO_NAMESPACE : ViewNamespace( (DaoNamespace*)value ); break;
	case DAO_PROCESS : ViewProcess( (DaoProcess*)value ); break;
	default : ok = false; break;
	}
	if( ok ){
		messageTuple->items[INDEX_INIT]->xInteger.value = 0;
		SendDataInformation();
	}
}
void DaoInterpreter::ViewArray( DaoArray *array )
{
	DaoType *type = DaoNamespace_GetType( nameSpace, (DaoValue*)array );
	QString itemName = "Array[" + StringAddress(  array ) + "]";
	QString info = "# address:\n" + StringAddress( array ) + "\n# type:\n";
	size_t i;

	info += type ? type->name->mbs : "?";
	info += "\n# elements:\n" + QString::number( array->size );
	info += "\n# shape:\n[ ";
	for(i=0; i<array->ndim; i++){
		if( i ) info += ", ";
		info += QString::number( array->dims[i] );
	}
	info += " ]";

	InitMessage( (DaoValue*)array );
	DString_SetMBS( messageTuple->items[INDEX_NAME]->xString.data, itemName.toUtf8().data() );
	DString_SetMBS( messageTuple->items[INDEX_INFO]->xString.data, info.toUtf8().data() );

	DaoTuple_SetItem( messageTuple, (DaoValue*)array, INDEX_NUMBERS );
}
void DaoInterpreter::ViewList( DaoList *list )
{
	DaoType *itp, *type = DaoNamespace_GetType( nameSpace, (DaoValue*)list );
	QString itemName = "List[" + StringAddress(  list ) + "]";
	QString info = "# address:\n" + StringAddress( list ) + "\n# type:\n";
	size_t i, n = list->items.size;
	bool sametype = false;

	info += type ? type->name->mbs : "?";
	info += "\n# items:\n" + QString::number( n );

	InitMessage( (DaoValue*)list );
	DString_SetMBS( messageTuple->items[INDEX_NAME]->xString.data, itemName.toUtf8().data() );
	DString_SetMBS( messageTuple->items[INDEX_INFO]->xString.data, info.toUtf8().data() );

	if( type && type->nested->size ){
		itp = type->nested->items.pType[0];
		if( itp->tid >= DAO_INTEGER && itp->tid <= DAO_LONG ){
			type = itp;
			sametype = true;
		}
	}
	valueTuple->items[2]->xString.data->size = 0;
	for(i=0; i<n; i++){
		DaoValue *val = list->items.items.pValue[i];
		itp = sametype ? type : DaoNamespace_GetType( nameSpace, val );
		valueTuple->items[0]->xString.data->size = 0;
		valueTuple->items[1]->xString.data->size = 0;
		if( itp ) DString_SetMBS( valueTuple->items[0]->xString.data, itp->name->mbs );
		DaoValue_GetString( val, daoString );
		DString_SetMBS( valueTuple->items[1]->xString.data, daoString->mbs );
		DaoList_PushBack( varList, (DaoValue*)valueTuple );
	}
}
void DaoInterpreter::ViewMap( DaoMap *map )
{
	DaoType *itp, *type = DaoNamespace_GetType( nameSpace, (DaoValue*)map );
	QString itemName = "Map[" + StringAddress(  map ) + "]";
	QString info = "# address:\n" + StringAddress( map ) + "\n# type:\n";
	DNode *node;
	size_t i, n = map->items->size;

	info += type ? type->name->mbs : "?";
	info += "\n# items:\n" + QString::number( n );

	InitMessage( (DaoValue*)map );
	DString_SetMBS( messageTuple->items[INDEX_NAME]->xString.data, itemName.toUtf8().data() );
	DString_SetMBS( messageTuple->items[INDEX_INFO]->xString.data, info.toUtf8().data() );

	valueTuple->items[2]->xString.data->size = 0;
	for(node=DMap_First(map->items); node!=NULL; node=DMap_Next(map->items,node) ){
		itp = DaoNamespace_GetType( nameSpace, node->key.pValue );
		if( itp ) DString_SetMBS( valueTuple->items[0]->xString.data, itp->name->mbs );
		DaoValue_GetString( node->key.pValue, daoString );
		DString_SetMBS( valueTuple->items[1]->xString.data, daoString->mbs );
		DaoList_PushBack( constList, (DaoValue*)valueTuple );
		itp = DaoNamespace_GetType( nameSpace, node->value.pValue );
		if( itp ) DString_SetMBS( valueTuple->items[0]->xString.data, itp->name->mbs );
		DaoValue_GetString( node->value.pValue, daoString );
		DString_SetMBS( valueTuple->items[1]->xString.data, daoString->mbs );
		DaoList_PushBack( varList, (DaoValue*)valueTuple );
	}
}
void DaoInterpreter::ViewTuple( DaoTuple *tuple )
{
	DaoType *type = DaoNamespace_GetType( nameSpace, (DaoValue*)tuple );
	QString itemName = "Array[" + StringAddress(  tuple ) + "]";
	QString info = "# address:\n" + StringAddress( tuple ) + "\n# type:\n";
	QMap<int,QString> idnames;
	DNode *node;
	int i;

	info += type ? type->name->mbs : "?";
	info += "\n# items:\n" + QString::number( tuple->size );

	InitMessage( (DaoValue*)tuple );
	DString_SetMBS( messageTuple->items[INDEX_NAME]->xString.data, itemName.toUtf8().data() );
	DString_SetMBS( messageTuple->items[INDEX_INFO]->xString.data, info.toUtf8().data() );

	MakeList( varList, tuple->items, tuple->size, NULL, type->mapNames, 0 );
}
void DaoInterpreter::ViewNamespace( DaoNamespace *nspace )
{
	size_t i;
	QString info, itemName = "Namespace[" + StringAddress( nspace ) + "]";
	info += "# address:\n" + StringAddress( nspace );
	info += "\n# constants:\n";
	info += QString::number( nspace->cstData->size );
	info += "\n# variables:\n";
	info += QString::number( nspace->varData->size );
	if( nspace->file->mbs ){
		info += "\n\n# path:\n\"" + QString( nspace->path->mbs );
		info += "\"\n# file:\n\"" + QString( nspace->file->mbs ) + "\"";
	}
	InitMessage( (DaoValue*)nspace );
	DString_SetMBS( messageTuple->items[INDEX_NAME]->xString.data, itemName.toUtf8().data() );
	DString_SetMBS( messageTuple->items[INDEX_INFO]->xString.data, info.toUtf8().data() );

	extraTuple->items[2]->xString.data->size = 0;
	for(i=0; i<nspace->nsLoaded->size; i++){
		DaoNamespace *ns = nspace->nsLoaded->items.pNS[i];
		QString itname = "Namespace[" + StringAddress(ns) + "]";
		DString_SetMBS( extraTuple->items[0]->xString.data, itname.toUtf8().data() );
		DString_Assign( extraTuple->items[1]->xString.data, ns->name );
		DaoList_PushBack( extraList, (DaoValue*)extraTuple );
	}

	MakeList( constList, nspace->cstData->items.pValue, nspace->cstData->size,
			NULL, nspace->lookupTable, DAO_GLOBAL_CONSTANT );
	MakeList( varList, nspace->varData->items.pValue, nspace->varData->size,
			nspace->varType, nspace->lookupTable, DAO_GLOBAL_VARIABLE );

#if 0
	for(i=0; i<nspace->cstData->size; i++){
		DaoValue *val = nspace->cstData->items.pValue[i];
		it = wgtDataTable->item(i,0);
		if( it == NULL ) continue;
		if( val->type == DAO_CLASS && val->xClass.classRoutine->nameSpace == nspace )
			it->setBackground( QColor(200,250,200) );
		else if( val->type == DAO_ROUTINE && val->xRoutine.nameSpace == nspace )
			it->setBackground( QColor(200,250,200) );
	}
#endif
}
void DaoInterpreter::ViewProcess( DaoProcess *process )
{
#if 0
	currentFrame = frame;
	if( frame ){
		ViewStackFrame( frame, process );
		return;
	}
	EnableOneTable( (DaoValue*) process );
	nameSpace = process->activeNamespace;
#endif

	int i, index = 0;
	DaoStackFrame *frame = process->topFrame;
	while(frame && frame != process->firstFrame) {
		frame = frame->prev;
		index++;
	}

	QString info, itemName = "Process[" + StringAddress( process ) + "]";
	info = "# address:\n" + StringAddress( process );
	info += "\n# type:\nVM Process\n# frames:\n";
	info += QString::number( process->topFrame ? index : 10 );

	InitMessage( (DaoValue*)process );
	DString_SetMBS( messageTuple->items[INDEX_NAME]->xString.data, itemName.toUtf8().data() );
	DString_SetMBS( messageTuple->items[INDEX_INFO]->xString.data, info.toUtf8().data() );

	if( process->topFrame == NULL ) return;

	valueTuple->items[0]->xString.data->size = 0;
	valueTuple->items[1]->xString.data->size = 0;
	valueTuple->items[2]->xString.data->size = 0;

	QTableWidgetItem *it = NULL;
	frame = process->topFrame;
	for(i=0; frame && frame!=process->firstFrame; i++, frame=frame->prev){
		if( frame->routine ){
			DString_SetMBS( valueTuple->items[0]->xString.data, frame->routine->routName->mbs );
			DString_SetMBS( valueTuple->items[1]->xString.data, frame->routine->routType->name->mbs );
		}else{
			DString_SetMBS( valueTuple->items[0]->xString.data, frame->function->routName->mbs );
			DString_SetMBS( valueTuple->items[1]->xString.data, frame->function->routType->name->mbs );
		}
		DaoList_PushBack( constList, (DaoValue*)valueTuple );
	}
}
void DaoInterpreter::MakeList( DaoList *list, DaoValue **data, int size, DArray *type, DMap *names, int filter )
{
	DNode *node;
	DaoType *itp = NULL;
	QMap<int,QString> idnames;
	if( names ){
		for(node=DMap_First(names); node!=NULL; node=DMap_Next(names,node)){
			if( filter && LOOKUP_ST( node->value.pInt ) != filter ) continue;
			if( LOOKUP_UP( node->value.pInt ) ) continue;
			// LOOKUP_ID: get the last 16 bits
			idnames[ LOOKUP_ID( node->value.pInt ) ] = node->key.pString->mbs;
		}
	}
	DaoList_Clear( list );
	for(int i=0; i<size; i++){
		DaoValue *val = data[i];
		valueTuple->items[0]->xString.data->size = 0;
		valueTuple->items[1]->xString.data->size = 0;
		valueTuple->items[2]->xString.data->size = 0;
		if( val == NULL ){
			DString_SetMBS( valueTuple->items[0]->xString.data, "none" );
		}else if( idnames.find(i) != idnames.end() ){
			DString_SetMBS( valueTuple->items[0]->xString.data, idnames[i].toUtf8().data() );
		}else if( val->type == DAO_CLASS ){
			DString_SetMBS( valueTuple->items[0]->xString.data, val->xClass.className->mbs );
		}else if( val->type == DAO_ROUTINE || val->type == DAO_FUNCTION ){
			DString_SetMBS( valueTuple->items[0]->xString.data, val->xRoutine.routName->mbs );
		}
		itp = type ? type->items.pType[i] : NULL;
		if( itp == NULL ) itp = DaoNamespace_GetType( vmSpace->mainNamespace, val );
		if( itp ) DString_SetMBS( valueTuple->items[1]->xString.data, itp->name->mbs );
		DString *daoString = valueTuple->items[2]->xString.data;
		if( val ) DaoValue_GetString( val, daoString );
		if( DString_Size( daoString ) > 50 ) DString_Erase( daoString, 50, (size_t)-1 );
		DaoList_PushBack( list, (DaoValue*)valueTuple );
	}
}
void DaoInterpreter::ViewNamespaceData( DaoNamespace *nspace, DaoTuple *request )
{
	int table = request->items[1]->xInteger.value;
	int row = request->items[2]->xInteger.value;
	int col = request->items[3]->xInteger.value;
	if( table == INFO_TABLE ){
	}else if( table == DATA_TABLE ){
		ViewValue( nspace->cstData->items.pValue[row] );
	}else if( table == CODE_TABLE ){
		ViewValue( nspace->varData->items.pValue[row] );
	}
}
void DaoInterpreter::ViewProcessStack( DaoProcess *process, DaoTuple *request )
{
	int table = request->items[1]->xInteger.value;
	int row = request->items[2]->xInteger.value;
	int col = request->items[3]->xInteger.value;
	if( table == DATA_TABLE ){
		DaoStackFrame *frame = process->topFrame;
		while( (row--) >0 ) frame = frame->prev;
		ViewStackFrame( frame, process );
		messageTuple->items[INDEX_INIT]->xInteger.value = 0;
		SendDataInformation();
	}
}
void DaoInterpreter::ViewStackData( DaoProcess *proc, DaoStackFrame *frame, DaoTuple *request )
{
	int table = request->items[1]->xInteger.value;
	int row = request->items[2]->xInteger.value;
	int col = request->items[3]->xInteger.value;
	if( table == INFO_TABLE ){
	}else if( table == DATA_TABLE ){
		ViewValue( proc->stackValues[ frame->stackBase + row ] );
	}else if( table == CODE_TABLE ){
		if( col ) return;
		if( row >= frame->entry ) return;
		while( frame != proc->topFrame ) DaoProcess_PopFrame( proc );
		proc->topFrame->entry = row;
		proc->status = DAO_VMPROC_STACKED;
	}
}
QString DaoInterpreter::RoutineInfo( DaoRoutine *routine, void *address )
{
	DaoNamespace *ns = routine->nameSpace;
	QString info;
	info += "# address:\n" + StringAddress( address );
	info += "\n# function:\n";
	info += routine->routName->mbs;
	info += "\n# type:\n";
	info += routine->routType->name->mbs;
	info += "\n\n# instructions:\n";
	info += QString::number(routine->vmCodes->size);
	info += "\n# variables:\n";
	info += QString::number(routine->regCount);
	if( ns->file->mbs ){
		info += "\n\n# path:\n\"" + QString( ns->path->mbs );
		info += "\"\n# file:\n\"" + QString( ns->file->mbs ) + "\"";
	}
	return info;
}
void DaoInterpreter::ViewStackFrame( DaoStackFrame *frame, DaoProcess *process )
{
	if( frame->routine == NULL ) return; // TODO: function

	DaoRoutine *routine = frame->routine;
	DMap *map = DMap_New(D_STRING,0);
	DaoToken **tokens = routine->defLocals->items.pToken;
	int i, n = routine->defLocals->size;
	for(i=0; i<n; i++) if( tokens[i]->type )
		DMap_Insert( map, tokens[i]->string, (void*)(size_t)tokens[i]->index );

	QString info = RoutineInfo( routine, frame );
	QString itemName = "StackFrame[" + StringAddress( frame ) + "]";

	InitMessage( (DaoValue*)process );
	extraStack->items.pVoid[0] = frame;
	messageTuple->items[INDEX_ADDR]->xInteger.value = (dint) frame;
	messageTuple->items[INDEX_TYPE]->xInteger.value = DAO_FRAME_ROUT;
	messageTuple->items[INDEX_ENTRY]->xInteger.value = frame->entry;
	DString_SetMBS( messageTuple->items[INDEX_NAME]->xString.data, itemName.toUtf8().data() );
	DString_SetMBS( messageTuple->items[INDEX_INFO]->xString.data, info.toUtf8().data() );

	extraTuple->items[1]->xString.data->size = 0;
	extraTuple->items[2]->xString.data->size = 0;

	QString itname = "Namespace[" + StringAddress( routine->nameSpace ) + "]";
	DString_SetMBS( extraTuple->items[0]->xString.data, itname.toUtf8().data() );
	DaoList_PushBack( extraList, (DaoValue*)extraTuple );
	itname = "Routine[" + StringAddress( routine ) + "]";
	DString_SetMBS( extraTuple->items[0]->xString.data, itname.toUtf8().data() );
	DaoList_PushBack( extraList, (DaoValue*)extraTuple );
	if( frame->object ){
		itname = "Object[" + StringAddress( frame->object ) + "]";
		DString_SetMBS( extraTuple->items[0]->xString.data, itname.toUtf8().data() );
		DaoList_PushBack( extraList, (DaoValue*)extraTuple );
	}

	if( routine->vmCodes->size ==0 ) return;

	MakeList( varList, process->stackValues + frame->stackBase, routine->regCount, 
			routine->regType, map, 0 );
	ViewVmCodes( codeList, frame->routine );
#if 0
	for(j=1; j<4; j++)
		wgtExtraTable->item( vmcEntry, j )->setBackground( QColor(200,200,250) );
	QIcon icon( QPixmap( ":/images/start.png" ) );
	wgtExtraTable->item( vmcEntry, 0 )->setIcon( icon );
#endif
	DMap_Delete( map );
}
void DaoInterpreter::ViewVmCodes( DaoList *list, DaoRoutine *routine )
{
	int i, n = routine->vmCodes->size;
	for(i=0; i<n; i++){
		DaoVmCode vmc0 = routine->vmCodes->codes[i];
		DaoVmCodeX *vmc = routine->annotCodes->items.pVmc[i];
		codeTuple->items[0]->xInteger.value = vmc0.code;
		codeTuple->items[1]->xInteger.value = vmc->a;
		codeTuple->items[2]->xInteger.value = vmc->b;
		codeTuple->items[3]->xInteger.value = vmc->c;
		codeTuple->items[4]->xInteger.value = vmc->line;
		DaoTokens_AnnotateCode( routine->source, *vmc, daoString, 32 );
		DString_SetMBS( codeTuple->items[5]->xString.data, daoString->mbs );
		DaoList_PushBack( codeList, (DaoValue*)codeTuple );
	}
}
