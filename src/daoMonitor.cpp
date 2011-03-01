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
#include<daoMonitor.h>


DaoValueItem::DaoValueItem( DValue *p, QListWidget *w ) : QListWidgetItem(w)
{
    type = 0;
    value = p;
    parent = NULL;
    dataWidget = NULL;
}
DaoValueItem::DaoValueItem( DaoBase *p, QListWidget *w ) : QListWidgetItem(w)
{
    type = p->type;
    base = p;
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
    DaoBase *dbase = NULL;
    DaoVmFrame *frame;
    DValue *value = NULL;
    if( currentValue && currentValue->t >= DAO_ARRAY ){
        dbase = currentValue->v.p;
    }else if( currentObject ){
        dbase = currentObject;
    }
    DValueRef ref;
    ref.base = dbase;
    if( dbase ){
        switch( dbase->type ){
        case DAO_VMPROCESS :
            frame = ref.process->topFrame;
            while( (row--) >0 ) frame = frame->prev;
            ViewContext( frame->context );
            it = new DaoValueItem( (DaoBase*) frame->context );
            break;
        case DAO_ARRAY : break;
        case DAO_LIST : value = ref.list->items->data + row; break;
        case DAO_MAP : value = itemValues[ 2*row + (col>=2) ]; break;
        case DAO_TUPLE : value = ref.tuple->items->data + row; break;
        case DAO_CLASS : value = ref.klass->cstData->data + row; break;
        case DAO_OBJECT : value = ref.object->objValues + row; break;
        case DAO_ROUTINE : value = ref.routine->routConsts->data + row; break;
        case DAO_CONTEXT : value = ref.context->regValues[ row ]; break;
        case DAO_NAMESPACE : value = ref.nspace->cstData->data + row; break;
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
    DaoBase *oldObject = currentObject;
    DValue *oldValue = currentValue;
    DValueRef ref;
    ref.base = currentObject;
    if(  currentObject == NULL ) return;
    switch( currentObject->type ){
    case DAO_CLASS :
        ViewClass( ref.klass->superClass->items.pClass[row] );
        break;
    case DAO_OBJECT :
        if( row == 0 )
            ViewClass( ref.object->myClass );
        else
            ViewObject( ref.object->superObject->items.pObject[row-1] );
        break;
    case DAO_ROUTINE :
        switch( row ){
        case 0 : ViewNameSpace( ref.routine->nameSpace ); break;
        case 1 : ViewClass( ref.routine->routHost->aux.v.klass ); break;
        default : break;
        }
        break;
    case DAO_CONTEXT :
        switch( row ){
        case 0 : ViewNameSpace( ref.context->nameSpace ); break;
        case 1 : ViewRoutine( ref.context->routine ); break;
        case 2 : ViewObject( ref.context->object ); break;
        default : break;
        }
        break;
    case DAO_NAMESPACE :
        ViewNameSpace( (DaoNameSpace*) ref.nspace->nsLoaded->items.pBase[row] );
        break;
    default : break;
    }
    if( currentValue != oldValue || currentObject != oldObject ){
        if( currentValue ){
            DaoValueItem *it = new DaoValueItem( currentValue );
            it->setText( itemName );
            wgtDataList->insertItem( 0, it );
        }else{
            DaoValueItem *it = new DaoValueItem( currentObject );
            it->setText( itemName );
            wgtDataList->insertItem( 0, it );
        }
    }
}
void DaoDataWidget::slotExtraTableClicked(int row, int col)
{
    DaoValueItem *it = NULL;
    DaoBase *dbase = NULL;
    DValue *value = NULL;
    if( currentValue && currentValue->t >= DAO_ARRAY ){
        dbase = currentValue->v.p;
    }else if( currentObject ){
        dbase = currentObject;
    }
    DValueRef ref;
    ref.base = dbase;
    if( dbase ){
        switch( dbase->type ){
        case DAO_CONTEXT : ResetExecutionPoint( row, col ); break;
        case DAO_CLASS : value = ref.klass->glbData->data + row; break;
        case DAO_NAMESPACE : value = ref.nspace->varData->data + row; break;
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
    if( currentObject == NULL || currentObject->type != DAO_CONTEXT ) return;
    DaoContext *context = (DaoContext*)currentObject;
    DaoVmProcess *vmp = context->process;
    if( col ) return;
    if( row >= vmcEntry ){
        QMessageBox::warning( this, tr("DaoStudio - Change execution point"),
                tr("Only possible to go back to a previously executed point"), 
                QMessageBox::Cancel );
        return;
    }
    if( row == vmcNewEntry ) return;
    if( context != vmp->topFrame->context ){
        int ret = QMessageBox::warning( this, tr("DaoStudio - Change execution point"),
                tr("This frame is not in the top of the execution stack.\n"
                    "Setting this new execution point will remove those frames "
                    "that are above this one.\n\nProceed?"), 
                QMessageBox::Yes | QMessageBox::No );
        if( ret == QMessageBox::No ) return;
    }
    QIcon icon( QPixmap( ":/images/start.png" ) );
    wgtExtraTable->item( vmcNewEntry, 0 )->setIcon( QIcon() );
    wgtExtraTable->item( row, 0 )->setIcon( icon );
    vmcNewEntry = row;
    while( context != vmp->topFrame->context ) DaoVmProcess_PopContext( vmp );
    vmp->topFrame->entry = row;
    vmp->status = DAO_VMPROC_STACKED;
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
void DaoDataWidget::EnableOneTable( DaoBase *p )
{
    currentValue = NULL;
    currentObject = p;
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
void DaoDataWidget::EnableTwoTable( DaoBase *p )
{
    currentValue = NULL;
    currentObject = p;
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
void DaoDataWidget::EnableThreeTable( DaoBase *p )
{
    currentValue = NULL;
    currentObject = p;
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
void DaoDataWidget::ViewValue( DValue *value )
{
    currentValue = value;
    currentObject = NULL;

    EnableNoneTable();
    wgtDataTable->clear();
    wgtExtraTable->clear();
    //wgtDataTable->setEnabled( false );
    //wgtExtraTable->setEnabled( false );
    labDataTable->setText( "" );
    labExtraTable->setText( "" );
    DaoType *type;
    QString info = "# address:\n" + StringAddress( value ) + "\n# type:\n";
    type = DaoNameSpace_GetTypeV( nameSpace, *value );
    itemName = type ? type->name->mbs : "Value";
    if( itemName == "?" ) itemName = "Value";
    itemName += "[" + StringAddress( value ) + "]";
    itemName[0] = itemName[0].toUpper();
    info += type ? type->name->mbs : "?";
    if( value->t == DAO_STRING )
        info += "\n# length:\n" + QString::number( DString_Size( value->v.s ) );
    wgtDataInfo->setPlainText( info );

    switch( value->t ){
    case DAO_NIL :
    case DAO_INTEGER :
    case DAO_FLOAT :
    case DAO_DOUBLE :
    case DAO_COMPLEX :
    case DAO_STRING :
    case DAO_LONG :
        EnableNoneTable();
        DValue_GetString( *value, daoString );
        wgtDataValue->setPlainText( DString_GetMBS( daoString ) );
        break;
    case DAO_ARRAY : ViewArray( value->v.array ); break;
    case DAO_LIST  : ViewList(  value->v.list  ); break;
    case DAO_MAP   : ViewMap(   value->v.map   ); break;
    case DAO_TUPLE : ViewTuple( value->v.tuple ); break;
    case DAO_CLASS : ViewClass( value->v.klass ); break;
    case DAO_OBJECT  : ViewObject(  value->v.object  ); break;
    case DAO_ROUTINE : ViewRoutine( value->v.routine ); break;
    case DAO_FUNCTION : ViewFunction( (DaoFunction*)value->v.p ); break;
    case DAO_NAMESPACE : ViewNameSpace( value->v.ns ); break;
    case DAO_VMPROCESS : ViewProcess( value->v.vmp  ); break;
    default: break;
    }
}
void DaoDataWidget::ViewArray( DaoArray *array )
{
    EnableOneTable( (DaoBase*) array );
    itemName = "Array[" + StringAddress(  array ) + "]";

    DaoType *type = DaoNameSpace_GetType( nameSpace, (DaoBase*)array );
    QVector<int> tmp;
    QStringList headers;
    QString info = "# address:\n" + StringAddress( array ) + "\n# type:\n";
    size_t i, n, row, col;
    info += type ? type->name->mbs : "?";
    info += "\n# elements:\n" + QString::number( array->size );
    info += "\n# shape:\n[ ";
    for(i=0; i<array->dims->size; i++){
        if( i ) info += ", ";
        info += QString::number( array->dims->items.pInt[i] );
    }
    info += " ]";
    wgtDataInfo->setPlainText( info );
    n = array->dims->items.pSize[array->dims->size-1];
    wgtDataTable->setRowCount( array->size / n );
    wgtDataTable->setColumnCount( n );
    for(i=0; i<n; i++) wgtDataTable->setColumnWidth( i, 80 );
    wgtDataTable->setEditTriggers( QAbstractItemView::DoubleClicked );

    tmp.resize( array->dims->size );
    row = -1;
    col = 0;
    for(i=0; i<array->size; i++){
        size_t *dims = array->dims->items.pSize;
        int j, mod = i;
        for(j=array->dims->size-1; j >=0; j--){
            int res = ( mod % dims[j] );
            mod /= dims[j];
            tmp[j] = res;
        }
        if( tmp[array->dims->size-1] ==0 ){
            QString hd;
            for(j=0; j+1<(int)array->dims->size; j++) hd += QString::number(tmp[j]) + ",";
            headers << "row[" + hd + ":]";
            row ++;
            col = 0;
        }
        QString elem;
        switch( array->numType ){
        case DAO_INTEGER :
            elem = QString::number( array->data.i[i] );
            break;
        case DAO_FLOAT :
            elem = QString::number( array->data.f[i] );
            break;
        case DAO_DOUBLE :
            elem = QString::number( array->data.d[i] );
            break;
        case DAO_COMPLEX :
            elem = "(" + QString::number( array->data.c[i].real );
            elem += "," + QString::number( array->data.c[i].imag ) + ")";
            break;
        }
        wgtDataTable->setItem( row, col,  new QTableWidgetItem( elem ) );
        col ++;
    }
    wgtDataTable->setVerticalHeaderLabels( headers );
}
void DaoDataWidget::ViewList( DaoList *list )
{
    EnableOneTable( (DaoBase*) list );
    itemName = "List[" + StringAddress(  list ) + "]";

    DaoType *itp, *type = DaoNameSpace_GetType( nameSpace, (DaoBase*)list );
    QTableWidgetItem *it;
    QStringList headers, rowlabs;
    QString info = "# address:\n" + StringAddress( list ) + "\n# type:\n";
    int i, n = list->items->size;
    bool sametype = false;
    info += type ? type->name->mbs : "?";
    info += "\n# items:\n" + QString::number( n );
    wgtDataInfo->setPlainText( info );
    //itemValues.resize( n );
    wgtDataTable->setRowCount( n );
    wgtDataTable->setColumnCount( 2 );
    headers<<tr("Type")<<tr("Value");
    wgtDataTable->setHorizontalHeaderLabels( headers );
    wgtDataTable->setColumnWidth( 0, 200 );
    wgtDataTable->setColumnWidth( 1, 300 );

    if( type && type->nested->size ){
        itp = type->nested->items.pType[0];
        if( itp->tid >= DAO_INTEGER && itp->tid <= DAO_LONG ){
            type = itp;
            sametype = true;
        }
    }
    for(i=0; i<n; i++){
        DValue val = list->items->data[i];
        rowlabs<<QString::number(i);
        //itemValues[i] = list->items->data + i;
        itp = sametype ? type : DaoNameSpace_GetTypeV( nameSpace, val );
        if( itp ){
            it = new QTableWidgetItem( itp->name->mbs );
            wgtDataTable->setItem( i, 0, it );
        }
        DValue_GetString( val, daoString );
        it = new QTableWidgetItem( DString_GetMBS( daoString ) );
        wgtDataTable->setItem( i, 1, it );
    }
    wgtDataTable->setVerticalHeaderLabels( rowlabs );
}
void DaoDataWidget::ViewMap( DaoMap *map )
{
    EnableOneTable( (DaoBase*) map );
    itemName = "Map[" + StringAddress(  map ) + "]";

    DaoType *itp, *type = DaoNameSpace_GetType( nameSpace, (DaoBase*)map );
    QTableWidgetItem *it;
    QStringList headers;
    QString info = "# address:\n" + StringAddress( map ) + "\n# type:\n";
    DNode *node;
    int i, n = map->items->size;
    info += type ? type->name->mbs : "?";
    info += "\n# items:\n" + QString::number( n );
    wgtDataTable->setRowCount( n );
    wgtDataTable->setColumnCount( 4 );
    headers<<tr("Key Type")<<tr("Key Data")<<tr("Value Type")<<tr("Value Data");
    wgtDataTable->setHorizontalHeaderLabels( headers );
    wgtDataTable->setColumnWidth( 0, 100 );
    wgtDataTable->setColumnWidth( 1, 200 );
    wgtDataTable->setColumnWidth( 2, 100 );
    wgtDataTable->setColumnWidth( 3, 200 );
    i = 0;
    for(node=DMap_First(map->items); node!=NULL; node=DMap_Next(map->items,node), i++ ){
        itemValues.append( node->key.pValue );
        itemValues.append( node->value.pValue );
        if( (itp = DaoNameSpace_GetTypeV( nameSpace, node->key.pValue[0] )) ){
            it = new QTableWidgetItem( itp->name->mbs );
            wgtDataTable->setItem( i, 0, it );
        }
        DValue_GetString( node->key.pValue[0], daoString );
        it = new QTableWidgetItem( DString_GetMBS( daoString ) );
        wgtDataTable->setItem( i, 1, it );
        if( (itp = DaoNameSpace_GetTypeV( nameSpace, node->value.pValue[0] )) ){
            it = new QTableWidgetItem( itp->name->mbs );
            wgtDataTable->setItem( i, 2, it );
        }
        DValue_GetString( node->value.pValue[0], daoString );
        it = new QTableWidgetItem( DString_GetMBS( daoString ) );
        wgtDataTable->setItem( i, 3, it );
    }
}
void DaoDataWidget::ViewTuple( DaoTuple *tuple )
{
    EnableOneTable( (DaoBase*) tuple );
    labDataTable->setText( tr("Items") );
    labExtraTable->setText( "" );
    itemName = "Tuple[" + StringAddress(  tuple ) + "]";

    size_t i;
    DNode *node;
    DaoType *type = DaoNameSpace_GetType( nameSpace, (DaoBase*)tuple );
    QMap<int,QString> idnames;
    QTableWidgetItem *it;
    QStringList headers, rowlabs;
    QString info = "# address:\n" + StringAddress( tuple ) + "\n# type:\n";
    info += type ? type->name->mbs : "?";
    info += "\n# items:\n" + QString::number( tuple->items->size );
    wgtDataInfo->setPlainText( info );

    wgtDataTable->setRowCount( tuple->items->size );
    wgtDataTable->setColumnCount( 3 );
    headers<<tr("Field")<<tr("Type")<<tr("Value");
    wgtDataTable->setHorizontalHeaderLabels( headers );
    wgtDataTable->setColumnWidth( 0, 150 );
    wgtDataTable->setColumnWidth( 1, 150 );
    wgtDataTable->setColumnWidth( 2, 300 );
    node = type->mapNames ? DMap_First( type->mapNames ) : NULL;
    for( ; node != NULL; node=DMap_Next(type->mapNames, node) ){
        idnames[ node->value.pInt ] = node->key.pString->mbs;
    }
    for(i=0; i<tuple->items->size; i++){
        DValue val = tuple->items->data[i];
        rowlabs<<QString::number(i);
        if( idnames.find(i) != idnames.end() ){
            it = new QTableWidgetItem( idnames[i] );
            wgtDataTable->setItem( i, 0, it );
        }
        type = DaoNameSpace_GetTypeV( nameSpace, val );
        if( type ){
            it = new QTableWidgetItem( type->name->mbs );
            wgtDataTable->setItem( i, 1, it );
        }
        DValue_GetString( val, daoString );
        it = new QTableWidgetItem( DString_GetMBS( daoString ) );
        wgtDataTable->setItem( i, 2, it );
    }
    wgtDataTable->setVerticalHeaderLabels( rowlabs );
}
void DaoDataWidget::ViewClass( DaoClass *klass )
{
    EnableThreeTable( (DaoBase*) klass );
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
    FillTable( wgtDataTable, & klass->cstData->data,
            klass->cstData->size, NULL, klass->lookupTable, DAO_CLASS_CONSTANT );
    FillTable( wgtExtraTable, & klass->glbData->data, klass->glbData->size, 
			klass->glbDataType, klass->lookupTable, DAO_CLASS_VARIABLE );
    for(i=0; i<klass->cstData->size; i++){
        DValue val = klass->cstData->data[i];
        if( val.t != DAO_ROUTINE || val.v.routine->tidHost != DAO_CLASS ) continue;
        if( val.v.routine->routHost->aux.v.klass == klass )
            wgtDataTable->item(i,0)->setBackground( QColor(200,250,200) );
    }
}
void DaoDataWidget::ViewObject( DaoObject *object )
{
    DaoClass *klass = object->myClass;
    EnableOneTable( (DaoBase*) object );
    wgtValuePanel->hide();
    wgtInfoPanel->show();
    itemName = "Object[" + QString( object->myClass->className->mbs ) + "]";
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
        info = "Object[" + StringAddress( object->superObject->items.pBase[i] ) + "]";
        wgtInfoTable->setItem( i+1, 0, new QTableWidgetItem( info ) );
    }
    wgtInfoTable->setVerticalHeaderLabels( rowlabs );
    rowlabs.clear();

    labDataTable->setText( tr("Instance Variables") );
    FillTable( wgtDataTable, & object->objValues, klass->objDataName->size,
			klass->objDataType, klass->lookupTable, DAO_OBJECT_VARIABLE );
}
void DaoDataWidget::ViewFunction( DaoFunction *function )
{
    itemName = "CFuntion[" + QString( function->routName->mbs ) + "]";
}
void DaoDataWidget::RoutineInfo( DaoRoutine *routine, void *address )
{
    DaoNameSpace *ns = routine->nameSpace;
    QString info;
    info += "# address:\n" + StringAddress( address );
    info += "\n# function:\n";
    info += routine->routName->mbs;
    info += "\n# type:\n";
    info += routine->routType->name->mbs;
    info += "\n\n# instructions:\n";
    info += QString::number(routine->vmCodes->size);
    info += "\n# variables:\n";
    info += QString::number(routine->locRegCount);
    if( ns->file->mbs ){
        info += "\n\n# path:\n\"" + QString( ns->path->mbs );
        info += "\"\n# file:\n\"" + QString( ns->file->mbs ) + "\"";
    }
    wgtDataInfo->setPlainText( info );
    wgtDataValue->clear();
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
    for(i=1; i<5; i++)  table->setColumnWidth( i, 60 );
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

    EnableThreeTable( (DaoBase*) routine );
    itemName = "Routine[" + QString( routine->routName->mbs ) + "]";
    RoutineInfo( routine, routine );

    QStringList rowlabs;
    wgtInfoTable->setRowCount( 1 + (routine->tidHost == DAO_CLASS) );
    wgtInfoTable->setColumnCount(1);
    wgtInfoTable->setColumnWidth(0, 150);
    rowlabs<<tr("NameSpace")<<tr("Class");
    wgtInfoTable->setHorizontalHeaderLabels( QStringList( tr("Data") ) );
    wgtInfoTable->setVerticalHeaderLabels( rowlabs );
    rowlabs.clear();

    QString info = "NameSpace[" + StringAddress(  routine->nameSpace ) + "]";
    wgtInfoTable->setItem( 0, 0, new QTableWidgetItem( info ) );
    if( routine->tidHost == DAO_CLASS ){
        info = "Class[" + StringAddress(  routine->routHost->aux.v.klass ) + "]";
        wgtInfoTable->setItem( 1, 0, new QTableWidgetItem( info ) );
    }

    labDataTable->setText( tr("Constants") );
    FillTable( wgtDataTable, & routine->routConsts->data,
            routine->routConsts->size, NULL, map, 0 );
    ViewVmCodes( wgtExtraTable, routine );
    DMap_Delete( map );
}
void DaoDataWidget::ViewContext( DaoContext *context )
{
    DaoRoutine *routine = context->routine;
    DMap *map = DMap_New(D_STRING,0);
    DaoToken **tokens = routine->defLocals->items.pToken;
    int i, n = routine->defLocals->size;
    for(i=0; i<n; i++) if( tokens[i]->type )
        DMap_Insert( map, tokens[i]->string, (void*)(size_t)tokens[i]->index );

    EnableThreeTable( (DaoBase*) context );
    wgtValuePanel->hide();
    labDataTable->setText( tr("Local Variables") );
    labInfoTable->setText( tr("Associated Data") );
    labExtraTable->setText( tr("VM Instructions") );
    nameSpace = context->nameSpace;

    QStringList rowlabs;
    int j;
    n = routine->vmCodes->size;
    vmcEntry = vmcNewEntry = (int)( context->vmc - routine->vmCodes->codes );

    itemName = "Context[" + QString( routine->routName->mbs ) + "]";

    RoutineInfo( routine, context );

    wgtInfoTable->setRowCount( 2 + (context->object != NULL) );
    wgtInfoTable->setColumnCount(1);
    wgtInfoTable->setColumnWidth(0, 150);
    rowlabs<<tr("NameSpace")<<tr("Routine");
    if( context->object ) rowlabs<<tr("Object");
    wgtInfoTable->setHorizontalHeaderLabels( QStringList( tr("Data") ) );
    wgtInfoTable->setVerticalHeaderLabels( rowlabs );
    rowlabs.clear();

    QString info = "NameSpace[" + StringAddress( nameSpace ) + "]";
    wgtInfoTable->setItem( 0, 0, new QTableWidgetItem( info ) );
    info = "Routine[" + StringAddress(  routine ) + "]";
    wgtInfoTable->setItem( 1, 0, new QTableWidgetItem( info ) );
    if( context->object ){
        info = "Object[" + StringAddress(  context->object ) + "]";
        wgtInfoTable->setItem( 2, 0, new QTableWidgetItem( info ) );
    }
    if( routine->vmCodes->size ==0 ) return;

    FillTable2( wgtDataTable, context->regValues, routine->locRegCount, 
            routine->regType, map );
    ViewVmCodes( wgtExtraTable, context->routine );
    for(j=1; j<4; j++)
        wgtExtraTable->item( vmcEntry, j )->setBackground( QColor(200,200,250) );
    QIcon icon( QPixmap( ":/images/start.png" ) );
    wgtExtraTable->item( vmcEntry, 0 )->setIcon( icon );
    DMap_Delete( map );
}
void DaoDataWidget::FillTable( QTableWidget *table, 
        DValue **data, int size, DArray *type, DMap *names, int filter )
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
            // LOOKUP_ID: get the last 16 bits
            idnames[ LOOKUP_ID( node->value.pInt ) ] = node->key.pString->mbs;
        }
    }
    for(i=0; i<size; i++){
        DValue val = (*data)[i];
        rowlabs<<QString::number(i);
        if( idnames.find(i) != idnames.end() ){
            it = new QTableWidgetItem( idnames[i] );
            table->setItem( i, 0, it );
        }else if( val.t == DAO_CLASS ){
            it = new QTableWidgetItem( val.v.klass->className->mbs );
            table->setItem( i, 0, it );
        }else if( val.t == DAO_ROUTINE || val.t == DAO_FUNCTION ){
            it = new QTableWidgetItem( val.v.routine->routName->mbs );
            table->setItem( i, 0, it );
        }
        itp = type ? type->items.pType[i] : NULL;
        if( itp == NULL ) itp = DaoNameSpace_GetTypeV( nameSpace, val );
        if( itp ){
            it = new QTableWidgetItem( itp->name->mbs );
            table->setItem( i, 1, it );
        }
        DValue_GetString( val, daoString );
        if( DString_Size( daoString ) > 50 ) DString_Erase( daoString, 50, (size_t)-1 );
        it = new QTableWidgetItem( DString_GetMBS( daoString ) );
        table->setItem( i, 2, it );
    }
    table->setVerticalHeaderLabels( rowlabs );
}
void DaoDataWidget::FillTable2( QTableWidget *table, 
        DValue **data, int size, DArray *type, DMap *names )
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
        DValue val = *data[i];
        rowlabs<<QString::number(i);
        if( idnames.find(i) != idnames.end() ){
            it = new QTableWidgetItem( idnames[i] );
            table->setItem( i, 0, it );
        }else if( val.t == DAO_CLASS ){
            it = new QTableWidgetItem( val.v.klass->className->mbs );
            table->setItem( i, 0, it );
        }else if( val.t == DAO_ROUTINE || val.t == DAO_FUNCTION ){
            it = new QTableWidgetItem( val.v.routine->routName->mbs );
            table->setItem( i, 0, it );
        }
        itp = type ? type->items.pType[i] : NULL;
        if( itp == NULL ) itp = DaoNameSpace_GetTypeV( nameSpace, val );
        if( itp ){
            it = new QTableWidgetItem( itp->name->mbs );
            table->setItem( i, 1, it );
        }
        DValue_GetString( val, daoString );
        if( DString_Size( daoString ) > 50 ) DString_Erase( daoString, 50, (size_t)-1 );
        it = new QTableWidgetItem( DString_GetMBS( daoString ) );
        table->setItem( i, 2, it );
    }
    table->setVerticalHeaderLabels( rowlabs );
}
void DaoDataWidget::ViewNameSpace( DaoNameSpace *nspace )
{
    EnableThreeTable( (DaoBase*) nspace );
    labDataTable->setText( tr("Constants") );
    labExtraTable->setText( tr("Variables") );
    nameSpace = nspace;
    itemName = "NameSpace[" + StringAddress(  nspace ) + "]";

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
        DaoNameSpace *ns = (DaoNameSpace*) nspace->nsLoaded->items.pBase[i];
        it = new QTableWidgetItem( "NameSpace[" + StringAddress(ns) + "]" );
        wgtInfoTable->setItem( i, 0, it );
        it = new QTableWidgetItem( ns->name->mbs );
        wgtInfoTable->setItem( i, 1, it );
    }

    FillTable( wgtDataTable, & nspace->cstData->data, nspace->cstData->size,
            NULL, nspace->lookupTable, DAO_GLOBAL_CONSTANT );
    FillTable( wgtExtraTable, & nspace->varData->data, nspace->varData->size,
            nspace->varType, nspace->lookupTable, DAO_GLOBAL_VARIABLE );
    for(i=0; i<nspace->cstData->size; i++){
        DValue val = nspace->cstData->data[i];
        it = wgtDataTable->item(i,0);
        if( it == NULL ) continue;
        if( val.t == DAO_CLASS && val.v.klass->classRoutine->nameSpace == nspace )
            it->setBackground( QColor(200,250,200) );
        else if( val.t == DAO_ROUTINE && val.v.routine->nameSpace == nspace )
            it->setBackground( QColor(200,250,200) );
    }
}
void DaoDataWidget::ViewProcess( DaoVmProcess *process )
{
    EnableOneTable( (DaoBase*) process );
    nameSpace = process->vmSpace->mainNamespace;
    if( process->topFrame && process->topFrame->context )
        nameSpace = process->topFrame->context->nameSpace;

    int index = 0;
    DaoVmFrame *frame = process->topFrame;
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
        DaoContext *ctx = frame->context;
        it = new QTableWidgetItem( ctx->routine->routName->mbs );
        wgtDataTable->setItem( i, 0, it );
        it = new QTableWidgetItem( ctx->routine->routType->name->mbs );
        wgtDataTable->setItem( i, 1, it );
    }
}
extern "C"{
DValue DaoParseNumber( DaoToken *tok, DLong *bigint );
}
void DaoDataWidget::slotUpdateValue()
{
    if( currentValue ==NULL ) return;
    DaoToken *tok = NULL;
    DValue number, *value = currentValue;
    QByteArray text = wgtDataValue->toPlainText().toUtf8();
    bool ok = true;
    if( value->t && (value->t <= DAO_DOUBLE || value->t == DAO_LONG) ){
        DaoToken_Tokenize( tokens, text.data(), 0, 1, 0 );
        if( tokens->size ) tok = tokens->items.pToken[0];
        if( tokens->size !=1 || tok->type <DTOK_DIGITS_HEX || tok->type >DTOK_NUMBER_SCI ){
            QMessageBox::warning( this, tr("DaoStudio"),
                    tr("Invalid value for the data type!"), QMessageBox::Cancel );
            return;
        }
        number = DaoParseNumber( tok, daoLong );
    }
    switch( value->t ){
    case DAO_INTEGER : value->v.i = DValue_GetInteger( number ); break;
    case DAO_FLOAT : value->v.f = DValue_GetFloat( number ); break;
    case DAO_DOUBLE : value->v.d = DValue_GetDouble( number ); break;
    case DAO_COMPLEX :
                      break;
    case DAO_STRING :
                      DString_SetDataMBS( value->v.s, text.data(), text.size() );
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
    if( currentObject == NULL || currentObject->type != DAO_ARRAY ) return;
    int id = row * wgtDataTable->columnCount() + col;
    DValue number;
    DaoToken *tok = NULL;
    DaoArray *array = (DaoArray*) currentObject;
    QByteArray text = wgtDataTable->item(row,col)->text().toLocal8Bit();
    DaoToken_Tokenize( tokens, text.data(), 0, 1, 0 );
    if( array->numType >= DAO_INTEGER && array->numType <= DAO_DOUBLE ){
        if( tokens->size ) tok = tokens->items.pToken[0];
        if( tokens->size !=1 || tok->type <DTOK_DIGITS_HEX || tok->type >DTOK_NUMBER_SCI ){
            QMessageBox::warning( this, tr("DaoStudio"),
                    tr("Invalid value for the data type!"), QMessageBox::Cancel );
            return;
        }
        number = DaoParseNumber( tok, daoLong );
    }
    switch( array->numType ){
    case DAO_INTEGER :
        array->data.i[id] = DValue_GetInteger( number );
        break;
    case DAO_FLOAT :
        array->data.f[id] = DValue_GetFloat( number );
        break;
    case DAO_DOUBLE :
        array->data.d[id] = DValue_GetDouble( number );
        break;
    case DAO_COMPLEX : // TODO
        break;
    }
}
    static int DaoEditContinueData
( QByteArray &data, QList<int> & lineMap, 
  QStringList & newCodes, QStringList & routCodes )
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
static void DaoResetExecution( DaoContext *context, int line, int offset=0 )
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
    //DaoVmCodeX_Print( *annotCodes[i], NULL );
    //printf( "entry: %s\n", annotCodes[i]->annot->mbs->data );
}
static void DaoStdioRead( DaoEventHandler *self, DString *buf, int count )
{
    fflush( stdout );
    self->socket.connectToServer( DaoStudioSettings::socket_stdin );
    self->socket.write( QByteArray::number( count ) );
    self->socket.flush();
    QObject::connect( & self->socket, SIGNAL(disconnected()),
            self->monitor, SLOT(slotExitWaiting()) );
    QByteArray data;
    self->monitor->waiting = true;
    while( self->monitor->waiting ){
        QCoreApplication::processEvents( QEventLoop::AllEvents, 1000 );
        fflush( stdout );
        if( self->process->stopit ) break;
    }
    self->socket.waitForReadyRead();
    data += self->socket.readAll();
    //QString s = QString::fromUtf8( data.data(), data.size() );
    DString_SetDataMBS( buf, data.data(), data.size() );
}
static void DaoConsDebug( DaoEventHandler *self, DaoContext *ctx )
{
    DaoVmCode  *codes = ctx->routine->vmCodes->codes;
    DaoVmCodeX **annots = ctx->routine->annotCodes->items.pVmc;
    int oldline = annots[ ctx->vmc - codes + 1]->line;
    QFileInfo fi( ctx->routine->nameSpace->name->mbs );
    QString name = fi.absoluteFilePath();
    QByteArray start = QByteArray::number( ctx->routine->bodyStart+1 );
    QByteArray end = QByteArray::number( ctx->routine->bodyEnd-1 );
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
            self->monitor, SLOT(slotExitWaiting()) );
    QByteArray data;
    self->monitor->waiting = true;
    self->monitor->viewContext = ctx;
    self->monitor->InitDataBrowser();
    while( self->monitor->waiting ){
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
    DaoRoutine *old = ctx->routine;
    int entry = DaoEditContinueData( data, lineMap, newCodes, routCodes );
    //printf( "all:\n%s\n", QString(data).toUtf8().data() );
    //printf( "new:\n%s\n", newCodes.join("\n").toUtf8().data() );
    //printf( "old:\n%s\n", routCodes.join("\n").toUtf8().data() );
    if( debugger.EditContinue( ctx, entry, lineMap, newCodes, routCodes ) == false ) return;
    if( old == ctx->routine && entry != oldline ) DaoResetExecution( ctx, entry );
    //printf( "%i %s\n", data.size(), data.data() );
    //QMessageBox::about( self->monitor, "test", "test" );
}
static void DaoSetBreaks( DaoEventHandler *self, DaoRoutine *routine )
{
    self->debugger.SetBreakPoints( routine );
}
static void DaoProcessMonitor( DaoEventHandler *self, DaoContext *ctx )
{
    if( self->timer.time > self->time ){
        //printf( "time: %i  %i\n", self->timer.time, self->time );
        //fflush( stdout );
        self->time = self->timer.time;
        if( ctx->process != self->process ) return;
        QApplication::processEvents( QEventLoop::AllEvents, TIME_EVENT );
        fflush( stdout );
    }
}


DaoMonitor::DaoMonitor( const char *cmd ) : QMainWindow()
{
    int i;
    if( cmd ) program = cmd;
    QFileInfo finfo( program ); 
    programPath = finfo.absolutePath();
    locale = QLocale::system().name();

	DaoStudioSettings::SetProgramPath( programPath );

    QCommonStyle style;
    QIcon book( QPixmap( ":/images/book.png" ) );
    QIcon dao( QPixmap( ":/images/dao.png" ) );

    index = 0;
    vmState = DAOCON_READY;
    debugProcess = NULL;
    viewContext = NULL;
    itemValues = NULL;
    itemCount = 0;
    daoString = DString_New(1);
    daoLong = DLong_New();
    tokens = DArray_New( D_TOKEN );

    setupUi(this);

    scriptSocket = NULL;

    handler.time = 0;
    handler.monitor = this;
    handler.stdRead = DaoStdioRead;
    handler.stdWrite = NULL;
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
    handler.process = DaoVmSpace_MainVmProcess( vmSpace );
    DaoVmSpace_SetUserHandler( vmSpace, (DaoUserHandler*) & handler );
	DString_SetMBS( vmSpace->fileName, "interactive codes" );
	DString_SetMBS( vmSpace->mainNamespace->name, "interactive codes" );

    dataWidget = new DaoDataWidget;
    dataWidget->SetDataList( wgtDataList );
    splitter->addWidget( dataWidget );

    DaoValueItem *vit = new DaoValueItem( (DaoBase*)vmSpace->mainNamespace, wgtDataList );
    vit->dataWidget = dataWidget;
    vit->setText( "NameSpace[Console]" );

    connect( wgtDataList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), 
            this, SLOT(slotValueActivated(QListWidgetItem*)) );

    SetPathWorking( "." );
    DaoVmSpace_AddPath( vmSpace, programPath.toLocal8Bit().data() );

	if( QFile::exists( DaoStudioSettings::socket_script ) )
		QFile::remove( DaoStudioSettings::socket_script );
    server.listen( DaoStudioSettings::socket_script );
    connect( &server, SIGNAL(newConnection()), this, SLOT(slotStartExecution()));

    time = 0;
    //connect( &timer, SIGNAL(timeout()), this, SLOT(slotFlushStdout()));
    //timer.start( 100 );

    shell = new QProcess();
    connect( shell, SIGNAL(readyReadStandardOutput()),
            this, SLOT(slotReadStdOut()));
    //connect( shell, SIGNAL(readyReadStandardError()),
    //    this, SLOT(slotReadStdOut()));
    connect( shell, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(slotShellFinished(int, QProcess::ExitStatus)));
}

DaoMonitor::~DaoMonitor()
{
    shell->close();
}

void DaoMonitor::SetPathWorking( const QString & path )
{
    QDir dir( path );
    pathWorking = dir.absolutePath() + "/";
    DaoVmSpace_SetPath( vmSpace, pathWorking.toLocal8Bit().data() );
}
void DaoMonitor::closeEvent ( QCloseEvent *e )
{
    int ret = QMessageBox::warning( this, tr("DaoMonitor - quit"),
            tr("Are you sure to quit?\n" ), QMessageBox::Yes | QMessageBox::No );
    if( ret == QMessageBox::Yes ){
		handler.timer.terminate();
		return;
	}
    e->ignore();
}
void DaoMonitor::slotReadStdOut()
{
    shell->setReadChannel( QProcess::StandardOutput );
    while( not shell->atEnd() ){
        QByteArray output = shell->readLine();
        char *data = output.data();
        int i, n = output.size();
        for(i=0; i<n; i++) printf( "%c", data[i] );
        fflush( stdout );
    }
    shell->setReadChannel( QProcess::StandardError );
    while( not shell->atEnd() ){
        QByteArray output = shell->readLine();
        char *data = output.data();
        int i, n = output.size();
        for(i=0; i<n; i++) printf( "%c", data[i] );
        fflush( stdout );
    }
}
void DaoMonitor::slotShellFinished(int, QProcess::ExitStatus)
{
    handler.process->stopit = 1;
}
class Sleeper : public QThread
{
    public:
        static void Sleep( int us ){ QThread::usleep( us ); }
};
void DaoMonitor::slotStartExecution()
{
    if( vmState == DAOCON_RUN ){
        printf( "is running\n" );
        fflush( stdout );
        return;
    }
    scriptSocket = server.nextPendingConnection();
    scriptSocket->waitForReadyRead();
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

    if( info )
        vmSpace->options |= DAO_EXEC_DEBUG;
    else
        vmSpace->options &= ~DAO_EXEC_DEBUG;

    vmState = DAOCON_RUN;
    DaoVmProcess *vmp = DaoVmSpace_MainVmProcess( vmSpace );
    DaoNameSpace *ns = DaoVmSpace_MainNameSpace( vmSpace );
	ns->options |= DAO_EXEC_IDE | DAO_NS_AUTO_GLOBAL;
    connect( scriptSocket, SIGNAL( disconnected() ), this, SLOT( slotStopExecution() ) );
    //disconnect( &server, SIGNAL(newConnection()), this, SLOT(slotStartExecution()));
    //QMessageBox::about( this, mbs->mbs, QString::number( mbs->size ) );
    connect( scriptSocket, SIGNAL( readyRead() ), this, SLOT( slotStopExecution() ) );
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
        //    vmSpace->stopit, res, shell->exitCode(), shell->error() );
    }else{
        DString *mbs = DString_New(1);
        DString_AppendDataMBS( mbs, script.data(), script.size() );
        res = (int) DaoVmProcess_Eval( vmp, ns, mbs, 1 );
        DString_Delete( mbs );
    }
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
    QString status = QString::number( res ) + '\0';
    status += QString::fromUtf8( buf, strlen( buf ) );
    scriptSocket->write( status.toUtf8() );
    fflush( stdout );
    ReduceValueItems( wgtDataList->item( wgtDataList->count()-1 ) );
    EraseDebuggingProcess();
    ViewValue( dataWidget, (DaoValueItem*) wgtDataList->item( 0 ) );
    scriptSocket->disconnectFromServer();
    //connect( &server, SIGNAL(newConnection()), this, SLOT(slotStartExecution()));
    vmState = DAOCON_READY;
}
void DaoMonitor::slotStopExecution()
{
    //QMessageBox::about( this, "","" );
    DaoVmProcess *vmp = DaoVmSpace_MainVmProcess( vmSpace );
    vmp->stopit = 1;
}
void DaoMonitor::slotExitWaiting()
{
    waiting = false;
}
void DaoMonitor::slotFlushStdout()
{
    time += 1;
    fflush( stdout );
}
void DaoMonitor::InitDataBrowser()
{
    ReduceValueItems( wgtDataList->item( wgtDataList->count()-1 ) );
    EraseDebuggingProcess();
    toolBox->setCurrentWidget( pageDataList );
    debugProcess = viewContext->process;

    DaoValueItem *vit = new DaoValueItem( (DaoBase*)debugProcess );
    vit->dataWidget = dataWidget;
    vit->setText( "Process[Debugging]" );
    wgtDataList->insertItem( 0, vit );
    ViewValue( dataWidget, (DaoValueItem*) wgtDataList->item( 0 ) );
}
void DaoMonitor::ViewValue( DaoDataWidget *dataView, DaoValueItem *it )
{
    switch( it->type ){
    case DAO_ARRAY  : dataView->ViewArray( it->array ); break;
    case DAO_LIST   : dataView->ViewList(  it->list  ); break;
    case DAO_MAP    : dataView->ViewMap(   it->map   ); break;
    case DAO_TUPLE  : dataView->ViewTuple( it->tuple ); break;
    case DAO_CLASS  : dataView->ViewClass( it->klass ); break;
    case DAO_OBJECT : dataView->ViewObject( it->object ); break;
    case DAO_ROUTINE : dataView->ViewRoutine( it->routine ); break;
    case DAO_CONTEXT : dataView->ViewContext( it->context ); break;
    case DAO_NAMESPACE : dataView->ViewNameSpace( it->nspace ); break;
    case DAO_VMPROCESS : dataView->ViewProcess( it->process ); break;
    default : dataView->ViewValue( it->value ); break;
    }
}
void DaoMonitor::ReduceValueItems( QListWidgetItem *item )
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
void DaoMonitor::EraseDebuggingProcess()
{
    DaoValueItem *it;
    int i;
    for(i=0; i<wgtDataList->count(); i++){
        it = (DaoValueItem*) wgtDataList->item(i);
        if( it->type == DAO_VMPROCESS && it->text() == "Process[Debugging]" ){
            it = (DaoValueItem*) wgtDataList->takeItem(i);
            delete it;
            break;
        }
    }
}
void DaoMonitor::slotValueActivated( QListWidgetItem *item )
{
    ReduceValueItems( item );
    ViewValue( dataWidget, (DaoValueItem*) item );
}

