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

#include<QtGui>
#include<cmath>
#include<QDir>

#include<daoConsole.h>
#include<daoDebugger.h>
#include<daoMonitor.h>
#include<daoStudio.h>


DaoMonitor::DaoMonitor( QWidget *parent ) : QWidget( parent )
{
	setupUi(this);
	dataSplitter->setStretchFactor( 0, 1 );
	dataSplitter->setStretchFactor( 1, 2 );
	//dataSplitter->setStretchFactor( 2, 3 );
	wgtInfoTableGroup->hide();

	hlDataInfo = new DaoCodeSHL( wgtDataInfo->document() );
	hlDataValue = new DaoCodeSHL( wgtDataValue->document() );
	hlDataInfo->SetState(DAO_HLSTATE_NORMAL);
	hlDataValue->SetState(DAO_HLSTATE_NORMAL);

	vmspace = NULL;
	currentValue = NULL;
	currentFrame = NULL;
	currentAddress = 0;
	currentType = 0;
	currentEntry = 0;
	currentTop = 0;

	requestTuple = NULL;

	lexer = DaoLexer_New();
	daoString = DString_New(1);
	daoLong = DLong_New();

	connect( wgtDataTable, SIGNAL(cellDoubleClicked(int,int)), 
			this, SLOT(slotDataTableClicked(int, int) ) );
	connect( wgtInfoTable, SIGNAL(cellDoubleClicked(int,int)), 
			this, SLOT(slotInfoTableClicked(int, int) ) );
	connect( wgtExtraTable, SIGNAL(cellDoubleClicked(int,int)), 
			this, SLOT(slotExtraTableClicked(int, int) ) );

	connect( wgtDataList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), 
			this, SLOT(slotValueActivated(QListWidgetItem*)) );

	if( QFile::exists( DaoStudioSettings::socket_monitor ) )
		QFile::remove( DaoStudioSettings::socket_monitor );
	monitorServer.listen( DaoStudioSettings::socket_monitor );
	connect( & monitorServer, SIGNAL(newConnection()), this, SLOT(slotAcceptConnection()));
}
DaoMonitor::~DaoMonitor()
{
	DaoLexer_Delete( lexer );
	DString_Delete( daoString );
	DLong_Delete( daoLong );
	delete hlDataInfo;
	delete hlDataValue;
}
void DaoMonitor::keyPressEvent ( QKeyEvent * event )
{
	Qt::KeyboardModifiers mdf = event->modifiers();
	int key = event->key();
	if( mdf & Qt::AltModifier ){
		if( key == Qt::Key_Up || key == Qt::Key_Down ){
			studio->slotMaxEditor();
			return;
		}else if( key == Qt::Key_Left ){
			return;
		}else if( key == Qt::Key_Right ){
			return;
		}
	}
	QWidget::keyPressEvent( event );
}
void DaoMonitor::focusInEvent ( QFocusEvent * event )
{
	emit signalFocusIn();
}
void DaoMonitor::slotAcceptConnection()
{
	monitorSocket = monitorServer.nextPendingConnection();
	if( vmspace == NULL ) return;

	connect( monitorSocket, SIGNAL(readyRead()), this, SLOT(slotReadData()));
	connect( monitorSocket, SIGNAL(disconnected()), this, SLOT(slotUpdateMonitor()));

	DString_Reset( daoString, 0 );
}
void DaoMonitor::slotReadData()
{
	QByteArray info = monitorSocket->readAll();
	DString_AppendDataMBS( daoString, info.data(), info.size() );
}
void DaoMonitor::Reset()
{
	wgtDataList->clear();
	wgtDataInfo->clear();
	wgtDataValue->clear();
	EnableNoneTable();
}
void DaoMonitor::slotUpdateMonitor()
{
	DaoValue *value = NULL;
	DaoNamespace *nspace = vmspace->mainNamespace;
	DaoProcess *process = vmspace->mainProcess;
	if( DaoValue_Deserialize( & value, daoString, nspace, process ) == 0 ) return;

	if( requestTuple == NULL ){
		DaoType *type = DaoParser_ParseTypeName( DATA_REQUEST_TYPE, nspace, NULL );
		requestTuple = DaoTuple_Create( type, 0, 1 );
	}

	DaoTuple *tuple = (DaoTuple*) value;
	if( tuple->items[INDEX_INIT]->xInteger.value == 1 ) wgtDataList->clear();// ClearDataStack();
#if 0
	if( tuple->items[INDEX_ADDR]->xInteger.value == currentAddress ){
		DaoTuple_Delete( tuple );
		return;
	}
#endif
	int subtype = tuple->items[INDEX_SUBTYPE]->xInteger.value;
	currentType = tuple->items[INDEX_TYPE]->xInteger.value;
	currentEntry = tuple->items[INDEX_ENTRY]->xInteger.value;
	currentTop = tuple->items[INDEX_TOP]->xInteger.value;
	currentAddress = tuple->items[INDEX_ADDR]->xInteger.value;

	QListWidgetItem *it = new QListWidgetItem( tuple->items[INDEX_NAME]->xString.data->mbs );
	wgtDataList->insertItem( 0, it );
	wgtDataList->setCurrentItem( it );

	wgtDataInfo->setPlainText( tuple->items[INDEX_INFO]->xString.data->mbs );
	wgtDataValue->setPlainText( tuple->items[INDEX_VALUE]->xString.data->mbs );

	switch( currentType ){
	case DAO_ARRAY : ViewArray( tuple ); break;
	case DAO_TUPLE : ViewTuple( tuple ); break;
	case DAO_LIST : ViewList( tuple ); break;
	case DAO_MAP : ViewMap( tuple ); break;
	case DAO_ROUTINE :
				   switch( subtype ){
				   case DAO_ROUTINE: ViewRoutine( tuple ); break;
				   case DAO_CFUNCTION : ViewFunction( tuple ); break;
				   case DAO_ROUTINES : ViewRoutines( tuple ); break;
				   }
				   break;
	case DAO_CLASS   : ViewClass( tuple ); break;
	case DAO_OBJECT   : ViewObject( tuple ); break;
	case DAO_NAMESPACE : ViewNamespace( tuple ); break;
	case DAO_PROCESS : ViewProcess( tuple ); break;
	case DAO_FRAME_ROUT : ViewStackFrame( tuple ); break;
	case DAO_FRAME_FUNC : ViewStackFrame( tuple ); break;
	default : EnableNoneTable(); break;
	}
	DaoTuple_Delete( tuple );
}
void DaoMonitor::slotValueActivated( QListWidgetItem *item )
{
	if( wgtDataList->row( item ) == 0 ) return;

	DString_SetMBS( requestTuple->items[0]->xString.data, item->text().toUtf8().data() );
	requestTuple->items[1]->xInteger.value = DATA_STACK;
	requestTuple->items[2]->xInteger.value = wgtDataList->row( item );
	requestTuple->items[3]->xInteger.value = 0;

	int keep = wgtDataList->count() - wgtDataList->row( item ) - 1;
	while( wgtDataList->count() > keep ) delete wgtDataList->takeItem(0);
	SendDataRequest();
}
void DaoMonitor::ClearDataStack()
{
	for(int i=0; i<wgtDataList->count(); i++){
		QListWidgetItem *it = wgtDataList->item(i);
		if( it->text() == "Process[Debugging]" ){
			it = (QListWidgetItem*) wgtDataList->takeItem(i);
			delete it;
			break;
		}
	}
}
void DaoMonitor::ViewArray( DaoTuple *tuple )
{
	EnableOneTable( NULL );

	QVector<int> tmp;
	QStringList headers;
	DaoArray *array = (DaoArray*)tuple->items[INDEX_NUMBERS];
	size_t i, n, row, col;

	n = array->dims[array->ndim-1];
	wgtDataTable->setRowCount( array->size / n );
	wgtDataTable->setColumnCount( n );
	for(i=0; i<n; i++) wgtDataTable->setColumnWidth( i, 80 );
	//wgtDataTable->setEditTriggers( QAbstractItemView::DoubleClicked );

	tmp.resize( array->ndim );
	row = -1;
	col = 0;
	for(i=0; i<array->size; i++){
		daoint *dims = array->dims;
		int j, mod = i;
		for(j=array->ndim-1; j >=0; j--){
			int res = ( mod % dims[j] );
			mod /= dims[j];
			tmp[j] = res;
		}
		if( tmp[array->ndim-1] ==0 ){
			QString hd;
			for(j=0; j+1<(int)array->ndim; j++) hd += QString::number(tmp[j]) + ",";
			headers << "row[" + hd + ":]";
			row ++;
			col = 0;
		}
		QString elem;
		switch( array->etype ){
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
		wgtDataTable->setItem( row, col, new QTableWidgetItem( elem ) );
		col ++;
	}
	wgtDataTable->setVerticalHeaderLabels( headers );
}
void DaoMonitor::ViewTuple( DaoTuple *tuple )
{
	EnableOneTable( (DaoValue*) tuple );
	wgtDataTableGroup->setTitle( tr("Items") );
	wgtExtraTableGroup->setTitle( "" );

	QTableWidgetItem *it;
	QStringList headers, rowlabs;
	DArray *items = & tuple->items[INDEX_VARS]->xList.items;
	size_t i, j, n = items->size;

	wgtDataTable->setRowCount( tuple->size );
	wgtDataTable->setColumnCount( 3 );
	headers<<tr("Field")<<tr("Type")<<tr("Value");
	wgtDataTable->setHorizontalHeaderLabels( headers );
	wgtDataTable->setColumnWidth( 0, 150 );
	wgtDataTable->setColumnWidth( 1, 150 );
	wgtDataTable->setColumnWidth( 2, 300 );

	for(i=0; i<n; i++){
		DaoTuple *itup = items->items.pTuple[i];
		rowlabs<<QString::number(i);
		for(j=0; j<3; j++){
			wgtDataTable->setItem( i, j, new QTableWidgetItem( itup->items[j]->xString.data->mbs ) );
		}
	}
	wgtDataTable->setVerticalHeaderLabels( rowlabs );
}
void DaoMonitor::ViewList( DaoTuple *tuple )
{
	EnableOneTable( NULL );

	QTableWidgetItem *it;
	QStringList headers, rowlabs;
	DArray *items = & tuple->items[INDEX_VARS]->xList.items;
	size_t i, j, n = items->size;

	wgtDataTable->setRowCount( n );
	wgtDataTable->setColumnCount( 2 );
	headers<<tr("Type")<<tr("Value");
	wgtDataTable->setHorizontalHeaderLabels( headers );
	wgtDataTable->setColumnWidth( 0, 200 );
	wgtDataTable->setColumnWidth( 1, 300 );

	for(i=0; i<n; i++){
		DaoTuple *itup = items->items.pTuple[i];
		rowlabs<<QString::number(i);
		for(j=0; j<2; j++){
			wgtDataTable->setItem( i, j, new QTableWidgetItem( itup->items[j]->xString.data->mbs ) );
		}
	}
	wgtDataTable->setVerticalHeaderLabels( rowlabs );
}
void DaoMonitor::ViewMap( DaoTuple *tuple )
{
	EnableOneTable( NULL );

	QTableWidgetItem *it;
	QStringList headers;
	DArray *keys = & tuple->items[INDEX_CONSTS]->xList.items;
	DArray *values = & tuple->items[INDEX_VARS]->xList.items;
	size_t i, j, n = keys->size;

	wgtDataTable->setRowCount( n );
	wgtDataTable->setColumnCount( 4 );
	headers<<tr("Key Type")<<tr("Key Data")<<tr("Value Type")<<tr("Value Data");
	wgtDataTable->setHorizontalHeaderLabels( headers );
	wgtDataTable->setColumnWidth( 0, 100 );
	wgtDataTable->setColumnWidth( 1, 200 );
	wgtDataTable->setColumnWidth( 2, 100 );
	wgtDataTable->setColumnWidth( 3, 200 );
	for(i=0; i<n; i++){
		DaoTuple *itup = keys->items.pTuple[i];
		for(j=0; j<2; j++){
			wgtDataTable->setItem( i, j, new QTableWidgetItem( itup->items[j]->xString.data->mbs ) );
		}
		itup = values->items.pTuple[i];
		for(j=0; j<2; j++){
			wgtDataTable->setItem( i, j+2, new QTableWidgetItem( itup->items[j]->xString.data->mbs ) );
		}
	}
}
void DaoMonitor::ViewRoutine( DaoTuple *tuple )
{
	DArray *extras = & tuple->items[INDEX_EXTRAS]->xList.items;
	size_t i, j;

	EnableThreeTable( NULL );
	wgtDataTableGroup->setTitle( tr("Local Constants") );

	QStringList rowlabs;
	wgtInfoTable->setRowCount( extras->size );
	wgtInfoTable->setColumnCount(1);
	wgtInfoTable->setColumnWidth(0, 150);
	rowlabs<<tr("Namespace")<<tr("Class");
	wgtInfoTable->setHorizontalHeaderLabels( QStringList( tr("Data") ) );
	wgtInfoTable->setVerticalHeaderLabels( rowlabs );
	rowlabs.clear();

	for(i=0; i<extras->size; i++){
		DaoTuple *itup = extras->items.pTuple[i];
		for(j=0; j<1; j++){
			wgtInfoTable->setItem( i, j, new QTableWidgetItem( itup->items[j]->xString.data->mbs ) );
		}
	}
	FillTable( wgtDataTable, (DaoList*)tuple->items[INDEX_CONSTS] );
	FillCodes( wgtExtraTable, (DaoList*)tuple->items[INDEX_CODES] );
}
void DaoMonitor::ViewFunction( DaoTuple *tuple )
{
	EnableNoneTable(); // XXX
}
void DaoMonitor::ViewRoutines( DaoTuple *tuple )
{
	DArray *extras = & tuple->items[INDEX_EXTRAS]->xList.items;
	size_t i, j;
	EnableOneTable( NULL );
	wgtDataTableGroup->setTitle( tr("Overloaded Functions") );
	wgtDataTable->setRowCount( extras->size );
	wgtDataTable->setColumnCount(2);
	wgtDataTable->setColumnWidth(0, 200);
	wgtDataTable->setColumnWidth(1, 400);
	for(i=0; i<extras->size; i++){
		DaoTuple *itup = extras->items.pTuple[i];
		for(j=0; j<2; j++){
			wgtDataTable->setItem( i, j, new QTableWidgetItem( itup->items[j]->xString.data->mbs ) );
		}
	}
}
void DaoMonitor::ViewClass( DaoTuple *tuple )
{
	EnableThreeTable( NULL );

	QStringList rowlabs;
	DArray *extras = & tuple->items[INDEX_EXTRAS]->xList.items;
	size_t i, j;

	wgtInfoTable->setRowCount( extras->size );
	wgtInfoTable->setColumnCount(1);
	wgtInfoTable->setColumnWidth(0, 150);
	wgtInfoTable->setHorizontalHeaderLabels( QStringList( tr("Data") ) );
	for(i=0; i<extras->size; i++){
		DaoTuple *itup = extras->items.pTuple[i];
		rowlabs<<tr("Parent/Mixin")+QString::number(i+1);
		for(j=0; j<1; j++){
			wgtInfoTable->setItem( i, j, new QTableWidgetItem( itup->items[j]->xString.data->mbs ) );
		}
	}
	wgtInfoTable->setVerticalHeaderLabels( rowlabs );
	rowlabs.clear();

	wgtDataTableGroup->setTitle( tr("Constants") );
	wgtExtraTableGroup->setTitle( tr("Static Variables") );

	FillTable( wgtDataTable, (DaoList*)tuple->items[INDEX_CONSTS] );
	FillTable( wgtExtraTable, (DaoList*)tuple->items[INDEX_VARS] );
}
void DaoMonitor::ViewObject( DaoTuple *tuple )
{
	EnableOneTable( NULL );
	wgtInfoTableGroup->show();

	QStringList rowlabs;
	DArray *extras = & tuple->items[INDEX_EXTRAS]->xList.items;
	size_t i, j;

	wgtInfoTable->setRowCount( extras->size );
	wgtInfoTable->setColumnCount(1);
	wgtInfoTable->setColumnWidth(0, 150);
	wgtInfoTable->setHorizontalHeaderLabels( QStringList( tr("Data") ) );
	for(i=0; i<extras->size; i++){
		DaoTuple *itup = extras->items.pTuple[i];
		if( i == 0 ){
			rowlabs<<tr("Class");
		}else{
			rowlabs<<tr("Parent Object")+QString::number(i);
		}
		for(j=0; j<1; j++){
			wgtInfoTable->setItem( i, j, new QTableWidgetItem( itup->items[j]->xString.data->mbs ) );
		}
	}
	wgtInfoTable->setVerticalHeaderLabels( rowlabs );
	rowlabs.clear();

	wgtDataTableGroup->setTitle( tr("Instance Variables") );
	FillTable( wgtDataTable, (DaoList*)tuple->items[INDEX_VARS] );
}
void DaoMonitor::ViewNamespace( DaoTuple *tuple )
{
	EnableThreeTable( NULL );
	wgtDataTableGroup->setTitle( tr("Constants") );
	wgtExtraTableGroup->setTitle( tr("Variables") );

	size_t i, j;
	QTableWidgetItem *it;
	QStringList headers;
	wgtDataValue->clear();

	DArray *extras = & tuple->items[INDEX_EXTRAS]->xList.items;
	wgtInfoTable->setRowCount( extras->size );
	wgtInfoTable->setColumnCount( 2 );
	headers<<tr("Loaded Modules")<<tr("Module Files");
	wgtInfoTable->setHorizontalHeaderLabels( headers );
	for(i=0; i<extras->size; i++){
		DaoTuple *itup = extras->items.pTuple[i];
		for(j=0; j<2; j++){
			wgtInfoTable->setItem( i, j, new QTableWidgetItem( itup->items[j]->xString.data->mbs ) );
		}
	}
	FillTable( wgtDataTable, (DaoList*)tuple->items[INDEX_CONSTS] );
	FillTable( wgtExtraTable, (DaoList*)tuple->items[INDEX_VARS] );

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
void DaoMonitor::ViewProcess( DaoTuple *tuple )
{
	size_t i, j;
	DArray *array = & tuple->items[INDEX_CONSTS]->xList.items;
	EnableOneTable( NULL );

	wgtDataValue->clear();

	wgtDataTableGroup->setTitle( tr("Function Call Stack") );
	wgtDataTable->setRowCount( array->size );
	wgtDataTable->setColumnCount( 2 );
	wgtDataTable->setColumnWidth( 0, 200 );
	wgtDataTable->setColumnWidth( 1, 300 );

	QStringList headers;
	headers<<tr("Function Name")<<tr("Function Type");
	wgtDataTable->setHorizontalHeaderLabels( headers );
	wgtDataTable->setEditTriggers( QAbstractItemView::NoEditTriggers );

	for(i=0; i<array->size; i++){
		DaoTuple *itup = array->items.pTuple[i];
		for(j=0; j<2; j++){
			wgtDataTable->setItem( i, j, new QTableWidgetItem( itup->items[j]->xString.data->mbs ) );
		}
	}
}
void DaoMonitor::ViewStackFrame( DaoTuple *tuple )
{
	EnableThreeTable( NULL );
	wgtDataTableGroup->setTitle( tr("Local Variables") );
	wgtInfoTableGroup->setTitle( tr("Associated Data") );
	wgtExtraTableGroup->setTitle( tr("VM Instructions") );

	wgtDataValue->clear();

	QStringList rowlabs;
	int i, j, entry = tuple->items[INDEX_ENTRY]->xInteger.value;

	DArray *extras = & tuple->items[INDEX_EXTRAS]->xList.items;
	wgtInfoTable->setRowCount( extras->size );
	wgtInfoTable->setColumnCount(1);
	wgtInfoTable->setColumnWidth(0, 150);
	rowlabs<<tr("Namespace")<<tr("Routine");
	if( extras->size == 3 ) rowlabs<<tr("Object");
	wgtInfoTable->setHorizontalHeaderLabels( QStringList( tr("Data") ) );
	wgtInfoTable->setVerticalHeaderLabels( rowlabs );
	rowlabs.clear();

	for(i=0; i<extras->size; i++){
		DaoTuple *itup = extras->items.pTuple[i];
		for(j=0; j<2; j++){
			wgtInfoTable->setItem( i, j, new QTableWidgetItem( itup->items[j]->xString.data->mbs ) );
		}
	}

	FillTable( wgtDataTable, (DaoList*)tuple->items[INDEX_VARS] );
	FillCodes( wgtExtraTable, (DaoList*)tuple->items[INDEX_CODES] );
	for(j=1; j<4; j++) wgtExtraTable->item( entry, j )->setBackground( QColor(200,200,250) );
	QIcon icon( QPixmap( ":/images/start.png" ) );
	wgtExtraTable->item( entry, 0 )->setIcon( icon );
}
void DaoMonitor::FillTable( QTableWidget *table, DaoList *list )
{
	QTableWidgetItem *it;
	QStringList headers, rowlabs;

	table->setRowCount( list->items.size );
	table->setColumnCount( 3 );
	headers<<tr("Name")<<tr("Type")<<tr("Value");
	table->setHorizontalHeaderLabels( headers );
	table->setColumnWidth( 0, 150 );
	table->setColumnWidth( 1, 150 );
	table->setColumnWidth( 2, 300 );
	for(int i=0; i<list->items.size; i++){
		DaoTuple *itup = list->items.items.pTuple[i];
		rowlabs<<QString::number(i);
		for(int j=0; j<3; j++){
			it = new QTableWidgetItem( itup->items[j]->xString.data->mbs );
			table->setItem( i, j, it );
		}
	}
	table->setVerticalHeaderLabels( rowlabs );
}
void DaoMonitor::FillCodes( QTableWidget *table, DaoList *list )
{
	int i, j, n = list->items.size;
	QStringList rowlabs;
	rowlabs<<tr("Opcode")<<"A"<<"B"<<"C"<<tr("Line")<<tr("Notes");
	table->setRowCount( n );
	table->setColumnCount( 6 );
	table->setHorizontalHeaderLabels( rowlabs );
	rowlabs.clear();
	for(i=1; i<5; i++) table->setColumnWidth( i, 60 );
	table->setColumnWidth( 5, 200 );
	for(i=0; i<n; i++){
		DaoTuple *itup = list->items.items.pTuple[i];
		rowlabs<<QString::number(i);
		table->setItem( i, 0, new QTableWidgetItem( DaoVmCode_GetOpcodeName( itup->items[0]->xInteger.value ) ) );
		for(j=1; j<5; j++){
			QString s = QString::number( itup->items[j]->xInteger.value );
			table->setItem( i, j, new QTableWidgetItem( s ) );
		}
		table->setItem( i, 5, new QTableWidgetItem( itup->items[5]->xString.data->mbs ) );
		table->item( i, 0 )->setToolTip( 
				tr("Double click on an Opcode cell to change\nthe current execution point") );
		table->item( i, 0 )->setBackground( QColor(200,250,200) );
		table->item( i, 4 )->setBackground( QColor(230,250,230) );
	}
	table->setVerticalHeaderLabels( rowlabs );
}
void DaoMonitor::SendDataRequest()
{
	QLocalSocket dataSocket;
	dataSocket.connectToServer( DaoStudioSettings::socket_data );
	if( dataSocket.waitForConnected( 1000 ) ==0 ){
		printf( "cannot connect to the virtual machine\n" );
		fflush( stdout );
		return;
	}
	if( vmspace == NULL ) return;
	DaoNamespace *nspace = vmspace->mainNamespace;
	DaoProcess *process = vmspace->mainProcess;
	if( DaoValue_Serialize( (DaoValue*) requestTuple, daoString, nspace, process )){
		dataSocket.write( daoString->mbs );
		dataSocket.flush();
		dataSocket.disconnectFromServer();
	}
}
void DaoMonitor::slotDataTableClicked(int row, int col)
{
	if( wgtDataList->count() == 0 ) return;
	DString_SetMBS( requestTuple->items[0]->xString.data, wgtDataList->item(0)->text().toUtf8().data() );
	requestTuple->items[1]->xInteger.value = DATA_TABLE;
	requestTuple->items[2]->xInteger.value = row;
	requestTuple->items[3]->xInteger.value = col;
	SendDataRequest();
}
void DaoMonitor::slotInfoTableClicked(int row, int col)
{
	if( wgtDataList->count() == 0 ) return;
	DString_SetMBS( requestTuple->items[0]->xString.data, wgtDataList->item(0)->text().toUtf8().data() );
	requestTuple->items[1]->xInteger.value = INFO_TABLE;
	requestTuple->items[2]->xInteger.value = row;
	requestTuple->items[3]->xInteger.value = col;
	SendDataRequest();
}
void DaoMonitor::slotExtraTableClicked(int row, int col)
{
	if( wgtDataList->count() == 0 ) return;
	if( currentType == DAO_FRAME_ROUT ){
		if( row == currentEntry || col ) return;
		if( row > currentEntry ){
			QMessageBox::warning( this, tr("DaoStudio - Change execution point"),
					tr("Only possible to go back to a previously executed point"), 
					QMessageBox::Cancel );
			return;
		}
		if( currentTop == 0 ){
			int ret = QMessageBox::warning( this, tr("DaoStudio - Change execution point"),
					tr("This frame is not in the top of the execution stack.\n"
						"Setting this new execution point will remove those frames "
						"that are above this one.\n\nProceed?"), 
					QMessageBox::Yes | QMessageBox::No );
			if( ret == QMessageBox::No ) return;
		}
		QIcon icon( QPixmap( ":/images/start.png" ) );
		wgtExtraTable->item( currentEntry, 0 )->setIcon( QIcon() );
		wgtExtraTable->item( row, 0 )->setIcon( icon );
		currentEntry = row;
	}
	DString_SetMBS( requestTuple->items[0]->xString.data, wgtDataList->item(0)->text().toUtf8().data() );
	requestTuple->items[1]->xInteger.value = CODE_TABLE;
	requestTuple->items[2]->xInteger.value = row;
	requestTuple->items[3]->xInteger.value = col;
	SendDataRequest();
}
void DaoMonitor::EnableNoneTable()
{
	wgtDataTable->clear();
	wgtExtraTable->clear();
	//wgtValuePanel->show();
	wgtDataTableGroup->setTitle( "" );
	wgtDataTableGroup->hide();
	wgtInfoTableGroup->hide();
	wgtExtraTableGroup->hide();
}
void DaoMonitor::EnableOneTable( DaoValue *p )
{
	//currentValue = p;
	wgtDataValue->clear();
	wgtDataTable->clear();
	wgtExtraTable->clear();
	wgtDataTableGroup->show();
	wgtInfoTableGroup->hide();
	wgtExtraTableGroup->hide();
	QList<int> sizes;
	int w = width()/3;
	sizes<<w<<w+w;
	dataSplitter->setSizes( sizes );
}
void DaoMonitor::EnableTwoTable( DaoValue *p )
{
	//currentValue = p;
	wgtDataValue->clear();
	wgtDataTable->clear();
	wgtExtraTable->clear();
	wgtDataTableGroup->show();
	wgtInfoTableGroup->hide();
	wgtExtraTableGroup->show();
	wgtDataTable->setEditTriggers( QAbstractItemView::NoEditTriggers );
	wgtInfoTable->setEditTriggers( QAbstractItemView::NoEditTriggers );
	wgtExtraTable->setEditTriggers( QAbstractItemView::NoEditTriggers );
	QList<int> sizes;
	int w = width()/8;
	sizes<<w+w<<3*w<<3*w;
	dataSplitter->setSizes( sizes );
}
void DaoMonitor::EnableThreeTable( DaoValue *p )
{
	wgtDataValue->clear();
	wgtDataTable->clear();
	wgtInfoTable->clear();
	wgtExtraTable->clear();
	//wgtValuePanel->hide();
	wgtDataTableGroup->show();
	wgtInfoTableGroup->show();
	wgtExtraTableGroup->show();
	wgtDataTable->setEditTriggers( QAbstractItemView::NoEditTriggers );
	wgtInfoTable->setEditTriggers( QAbstractItemView::NoEditTriggers );
	wgtExtraTable->setEditTriggers( QAbstractItemView::NoEditTriggers );
	QList<int> sizes;
	int w = width()/8;
	sizes<<w+w<<3*w<<3*w;
	dataSplitter->setSizes( sizes );
}

