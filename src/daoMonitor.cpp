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
#include<daoStudio.h>


DaoValueItem::DaoValueItem( DaoValue *p, QListWidget *w ) : QListWidgetItem(w)
{
	type = p->type;
	value = p;
	frame = NULL;
	parent = NULL;
	dataWidget = NULL;
}
DaoDataWidget::DaoDataWidget( QWidget *parent ) : QWidget( parent )
{
	setupUi(this);
	dataSplitter->setStretchFactor( 0, 1 );
	dataSplitter->setStretchFactor( 1, 2 );
	//dataSplitter->setStretchFactor( 2, 3 );
	wgtInfoPanel->hide();

	hlDataInfo = new DaoCodeSHL( wgtDataInfo->document() );
	hlDataValue = new DaoCodeSHL( wgtDataValue->document() );
	hlDataInfo->SetState(DAO_HLSTATE_NORMAL);
	hlDataValue->SetState(DAO_HLSTATE_NORMAL);

	currentValue = NULL;
	currentFrame = NULL;
	currentAddress = 0;
	currentType = 0;
	currentEntry = 0;
	currentTop = 0;

	requestTuple = NULL;

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

	connect( wgtDataList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), 
			this, SLOT(slotValueActivated(QListWidgetItem*)) );
	connect( btnUpdateValue, SIGNAL(clicked()), this, SLOT(slotUpdateValue()));

	if( QFile::exists( DaoStudioSettings::socket_monitor ) )
		QFile::remove( DaoStudioSettings::socket_monitor );
	monitorServer.listen( DaoStudioSettings::socket_monitor );
	connect( & monitorServer, SIGNAL(newConnection()), this, SLOT(slotUpdateMonitor()));
}
DaoDataWidget::~DaoDataWidget()
{
	DArray_Delete( tokens );
	DString_Delete( daoString );
	DLong_Delete( daoLong );
	delete hlDataInfo;
	delete hlDataValue;
}
void DaoDataWidget::keyPressEvent ( QKeyEvent * event )
{
	Qt::KeyboardModifiers mdf = event->modifiers();
	int key = event->key();
	if( mdf & Qt::AltModifier ){
		if( key == Qt::Key_Up || key == Qt::Key_Down ){
			studio->slotMaxEditor();
			return;
		}else if( key == Qt::Key_Left ){
			scriptTab->setCurrentIndex( 0 );
			return;
		}else if( key == Qt::Key_Right ){
			scriptTab->setCurrentIndex( 0 );
			return;
		}
	}
	QWidget::keyPressEvent( event );
}
void DaoDataWidget::focusInEvent ( QFocusEvent * event )
{
	emit signalFocusIn();
}
void DaoDataWidget::slotUpdateMonitor()
{
	monitorSocket = monitorServer.nextPendingConnection();
	if( monitorSocket->waitForReadyRead(1000) == false ) return;
	QByteArray info = monitorSocket->readAll();
	DaoValue *value = NULL;
	DString_SetDataMBS( daoString, info.data(), info.size() );
	if( DaoValue_Deserialize( & value, daoString, mainNamespace, NULL ) == 0 ) return;

	if( requestTuple == NULL ){
		DaoType *type = DaoParser_ParseTypeName( DATA_REQUEST_TYPE, mainNamespace, NULL );
		requestTuple = DaoTuple_Create( type, 1 );
	}

	printf( "value = %p\n", value );
	DaoTuple *tuple = (DaoTuple*) value;
	if( tuple->items[INDEX_INIT]->xInteger.value == 1 ) wgtDataList->clear();// ClearDataStack();
#if 0
	if( tuple->items[INDEX_ADDR]->xInteger.value == currentAddress ){
		DaoTuple_Delete( tuple );
		return;
	}
#endif
	currentType = tuple->items[INDEX_TYPE]->xInteger.value;
	currentEntry = tuple->items[INDEX_ENTRY]->xInteger.value;
	currentTop = tuple->items[INDEX_TOP]->xInteger.value;
	currentAddress = tuple->items[INDEX_ADDR]->xInteger.value;

	QListWidgetItem *it = new QListWidgetItem( tuple->items[INDEX_NAME]->xString.data->mbs );
	wgtDataList->insertItem( 0, it );

	wgtDataInfo->setPlainText( tuple->items[INDEX_INFO]->xString.data->mbs );
	wgtDataValue->setPlainText( tuple->items[INDEX_VALUE]->xString.data->mbs );

	switch( currentType ){
	case DAO_ARRAY : ViewArray( tuple ); break;
	case DAO_TUPLE : ViewTuple( tuple ); break;
	case DAO_LIST : ViewList( tuple ); break;
	case DAO_MAP : ViewMap( tuple ); break;
	case DAO_NAMESPACE : ViewNamespace( tuple ); break;
	case DAO_PROCESS : ViewProcess( tuple ); break;
	case DAO_FRAME_ROUT : ViewStackFrame( tuple ); break;
	case DAO_FRAME_FUNC : ViewStackFrame( tuple ); break;
	default : EnableNoneTable(); break;
	}
	DaoTuple_Delete( tuple );
}
void DaoDataWidget::slotValueActivated( QListWidgetItem *item )
{
	//ReduceValueItems( item );
	//ViewValue( dataWidget, (DaoValueItem*) item );
}
void DaoDataWidget::ClearDataStack()
{
	printf( "wgtDataList = %p\n", wgtDataList );
	for(int i=0; i<wgtDataList->count(); i++){
		QListWidgetItem *it = wgtDataList->item(i);
		if( it->text() == "Process[Debugging]" ){
			it = (QListWidgetItem*) wgtDataList->takeItem(i);
			delete it;
			break;
		}
	}
}
void DaoDataWidget::ViewArray( DaoTuple *tuple )
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
	wgtDataTable->setEditTriggers( QAbstractItemView::DoubleClicked );

	tmp.resize( array->ndim );
	row = -1;
	col = 0;
	for(i=0; i<array->size; i++){
		size_t *dims = array->dims;
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
void DaoDataWidget::ViewTuple( DaoTuple *tuple )
{
	EnableOneTable( (DaoValue*) tuple );
	labDataTable->setText( tr("Items") );
	labExtraTable->setText( "" );

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
void DaoDataWidget::ViewList( DaoTuple *tuple )
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
void DaoDataWidget::ViewMap( DaoTuple *tuple )
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
void DaoDataWidget::ViewNamespace( DaoTuple *tuple )
{
	EnableThreeTable( NULL );
	labDataTable->setText( tr("Constants") );
	labExtraTable->setText( tr("Variables") );

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
void DaoDataWidget::ViewProcess( DaoTuple *tuple )
{
	size_t i, j;
	DArray *array = & tuple->items[INDEX_CONSTS]->xList.items;
	EnableOneTable( NULL );

	wgtDataValue->clear();

	labDataTable->setText( tr("Function Call Stack") );
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
void DaoDataWidget::ViewStackFrame( DaoTuple *tuple )
{
	EnableThreeTable( NULL );
	labDataTable->setText( tr("Local Variables") );
	labInfoTable->setText( tr("Associated Data") );
	labExtraTable->setText( tr("VM Instructions") );

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
void DaoDataWidget::FillTable( QTableWidget *table, DaoList *list )
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
void DaoDataWidget::FillCodes( QTableWidget *table, DaoList *list )
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
		table->setItem( i, 0, new QTableWidgetItem( getOpcodeName( itup->items[0]->xInteger.value ) ) );
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
void DaoDataWidget::SendDataRequest()
{
	QLocalSocket dataSocket;
	dataSocket.connectToServer( DaoStudioSettings::socket_data );
	if( dataSocket.waitForConnected( 1000 ) ==0 ){
		printf( "cannot connect to the virtual machine\n" );
		fflush( stdout );
		return;
	}
	if( DaoValue_Serialize( (DaoValue*) requestTuple, daoString, mainNamespace, NULL )){
		dataSocket.write( daoString->mbs );
		dataSocket.flush();
		dataSocket.disconnectFromServer();
	}
}
void DaoDataWidget::slotDataTableClicked(int row, int col)
{
	DString_SetMBS( requestTuple->items[0]->xString.data, wgtDataList->item(0)->text().toUtf8().data() );
	requestTuple->items[1]->xInteger.value = DATA_TABLE;
	requestTuple->items[2]->xInteger.value = row;
	requestTuple->items[3]->xInteger.value = col;
	SendDataRequest();
}
void DaoDataWidget::slotInfoTableClicked(int row, int col)
{
	DString_SetMBS( requestTuple->items[0]->xString.data, wgtDataList->item(0)->text().toUtf8().data() );
	requestTuple->items[1]->xInteger.value = INFO_TABLE;
	requestTuple->items[2]->xInteger.value = row;
	requestTuple->items[3]->xInteger.value = col;
	SendDataRequest();
}
void DaoDataWidget::slotExtraTableClicked(int row, int col)
{
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
void DaoDataWidget::ResetExecutionPoint(int row, int col)
{
}
void DaoDataWidget::EnableNoneTable()
{
	wgtDataTable->clear();
	wgtExtraTable->clear();
	//wgtValuePanel->show();
	labDataTable->setText( "" );
	wgtDataTable->hide();
	wgtInfoPanel->hide();
	wgtExtraPanel->hide();
}
void DaoDataWidget::EnableOneTable( DaoValue *p )
{
	//currentValue = p;
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
	//currentValue = p;
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
	wgtDataValue->clear();
	wgtDataTable->clear();
	wgtInfoTable->clear();
	wgtExtraTable->clear();
	//wgtValuePanel->hide();
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
	EnableOneTable( (DaoValue*) array );
	itemName = "Array[" + StringAddress(  array ) + "]";

	DaoType *type = DaoNamespace_GetType( nameSpace, (DaoValue*)array );
	QVector<int> tmp;
	QStringList headers;
	QString info = "# address:\n" + StringAddress( array ) + "\n# type:\n";
	size_t i, n, row, col;
	info += type ? type->name->mbs : "?";
	info += "\n# elements:\n" + QString::number( array->size );
	info += "\n# shape:\n[ ";
	for(i=0; i<array->ndim; i++){
		if( i ) info += ", ";
		info += QString::number( array->dims[i] );
	}
	info += " ]";
	wgtDataInfo->setPlainText( info );
	n = array->dims[array->ndim-1];
	wgtDataTable->setRowCount( array->size / n );
	wgtDataTable->setColumnCount( n );
	for(i=0; i<n; i++) wgtDataTable->setColumnWidth( i, 80 );
	wgtDataTable->setEditTriggers( QAbstractItemView::DoubleClicked );

	tmp.resize( array->ndim );
	row = -1;
	col = 0;
	for(i=0; i<array->size; i++){
		size_t *dims = array->dims;
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
void DaoDataWidget::ViewList( DaoList *list )
{
	EnableOneTable( (DaoValue*) list );
	itemName = "List[" + StringAddress( list ) + "]";

	DaoType *itp, *type = DaoNamespace_GetType( nameSpace, (DaoValue*)list );
	QTableWidgetItem *it;
	QStringList headers, rowlabs;
	QString info = "# address:\n" + StringAddress( list ) + "\n# type:\n";
	int i, n = list->items.size;
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
		DaoValue *val = list->items.items.pValue[i];
		rowlabs<<QString::number(i);
		//itemValues[i] = list->items->data + i;
		itp = sametype ? type : DaoNamespace_GetType( nameSpace, val );
		if( itp ){
			it = new QTableWidgetItem( itp->name->mbs );
			wgtDataTable->setItem( i, 0, it );
		}
		DaoValue_GetString( val, daoString );
		it = new QTableWidgetItem( DString_GetMBS( daoString ) );
		wgtDataTable->setItem( i, 1, it );
	}
	wgtDataTable->setVerticalHeaderLabels( rowlabs );
}
void DaoDataWidget::ViewMap( DaoMap *map )
{
	EnableOneTable( (DaoValue*) map );
	itemName = "Map[" + StringAddress( map ) + "]";

	DaoType *itp, *type = DaoNamespace_GetType( nameSpace, (DaoValue*)map );
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
		if( (itp = DaoNamespace_GetType( nameSpace, node->key.pValue )) ){
			it = new QTableWidgetItem( itp->name->mbs );
			wgtDataTable->setItem( i, 0, it );
		}
		DaoValue_GetString( node->key.pValue, daoString );
		it = new QTableWidgetItem( DString_GetMBS( daoString ) );
		wgtDataTable->setItem( i, 1, it );
		if( (itp = DaoNamespace_GetType( nameSpace, node->value.pValue )) ){
			it = new QTableWidgetItem( itp->name->mbs );
			wgtDataTable->setItem( i, 2, it );
		}
		DaoValue_GetString( node->value.pValue, daoString );
		it = new QTableWidgetItem( DString_GetMBS( daoString ) );
		wgtDataTable->setItem( i, 3, it );
	}
}
void DaoDataWidget::ViewTuple2( DaoTuple *tuple )
{
	EnableOneTable( (DaoValue*) tuple );
	labDataTable->setText( tr("Items") );
	labExtraTable->setText( "" );
	itemName = "Tuple[" + StringAddress( tuple ) + "]";

	size_t i;
	DNode *node;
	DaoType *type = DaoNamespace_GetType( nameSpace, (DaoValue*)tuple );
	QMap<int,QString> idnames;
	QTableWidgetItem *it;
	QStringList headers, rowlabs;
	QString info = "# address:\n" + StringAddress( tuple ) + "\n# type:\n";
	info += type ? type->name->mbs : "?";
	info += "\n# items:\n" + QString::number( tuple->size );
	wgtDataInfo->setPlainText( info );

	wgtDataTable->setRowCount( tuple->size );
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
	for(i=0; i<tuple->size; i++){
		DaoValue *val = tuple->items[i];
		rowlabs<<QString::number(i);
		if( idnames.find(i) != idnames.end() ){
			it = new QTableWidgetItem( idnames[i] );
			wgtDataTable->setItem( i, 0, it );
		}
		type = DaoNamespace_GetType( nameSpace, val );
		if( type ){
			it = new QTableWidgetItem( type->name->mbs );
			wgtDataTable->setItem( i, 1, it );
		}
		DaoValue_GetString( val, daoString );
		it = new QTableWidgetItem( DString_GetMBS( daoString ) );
		wgtDataTable->setItem( i, 2, it );
	}
	wgtDataTable->setVerticalHeaderLabels( rowlabs );
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
	//wgtValuePanel->hide();
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
void DaoDataWidget::RoutineInfo( DaoRoutine *routine, void *address )
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
	//wgtValuePanel->hide();
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
static void DaoStdioRead( DaoEventHandler2 *self, DString *buf, int count )
{
	self->monitor->mutex.lock();
	fflush( stdout );
	self->socket2.flush();
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
	self->monitor->mutex.unlock();
}
static void DaoStdioWrite( DaoEventHandler2 *self, DString *buf )
{
#if 0
	FILE *fout = fopen( "output.txt", "a+" );
	fprintf( fout, "%s\n", DString_GetMBS( buf ) );
	fclose( fout );
#endif
	self->monitor->mutex.lock();
	self->socket2.write( DString_GetMBS( buf ), DString_Size( buf ) );
	self->socket2.flush();
	self->monitor->mutex.unlock();
}
static void DaoConsDebug( DaoEventHandler2 *self, DaoProcess *process )
{
	self->monitor->mutex.lock();

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
			self->monitor, SLOT(slotExitWaiting()) );
	QByteArray data;
	self->monitor->waiting = true;
	self->monitor->debugProcess = process;
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
	//QMessageBox::about( self->monitor, "test", "test" );

	self->monitor->mutex.unlock();
}
static void DaoSetBreaks( DaoEventHandler2 *self, DaoRoutine *routine )
{
	self->debugger.SetBreakPoints( routine );
}
static void DaoProcessMonitor( DaoEventHandler2 *self, DaoProcess *process )
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


DaoMonitor::DaoMonitor( const char *cmd ) : QMainWindow()
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

	setupUi(this);

	scriptSocket = NULL;

	handler.time = 0;
	handler.monitor = this;
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
	handler.process = DaoVmSpace_MainProcess( vmSpace );
	DaoVmSpace_SetUserHandler( vmSpace, (DaoUserHandler*) & handler );
	DString_SetMBS( vmSpace->fileName, "interactive codes" );
	DString_SetMBS( vmSpace->mainNamespace->name, "interactive codes" );

	dataWidget = new DaoDataWidget;
	splitter->addWidget( dataWidget );

	DaoValueItem *vit = new DaoValueItem( (DaoValue*)vmSpace->mainNamespace, wgtDataList );
	vit->dataWidget = dataWidget;
	vit->setText( "Namespace[Console]" );

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
void DaoMonitor::slotShellFinished(int, QProcess::ExitStatus)
{
	handler.process->stopit = 1;
}
void DaoMonitor::slotStartExecution()
{
	if( vmState == DAOCON_RUN ){
		//printf( "is running\n" );
		fflush( stdout );
		return;
	}

	scriptSocket = server.nextPendingConnection();
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
	ReduceValueItems( wgtDataList->item( wgtDataList->count()-1 ) );
	EraseDebuggingProcess();
	ViewValue( dataWidget, (DaoValueItem*) wgtDataList->item( 0 ) );
	//connect( &server, SIGNAL(newConnection()), this, SLOT(slotStartExecution()));
	vmState = DAOCON_READY;

#if 0
	FILE *fout = fopen( "debug.txt", "a+" );
	fprintf( fout, "%s\n", buf );
	fclose( fout );
#endif
}
void DaoMonitor::slotStopExecution()
{
	//QMessageBox::about( this, "","" );
	DaoProcess *vmp = DaoVmSpace_MainProcess( vmSpace );
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

	DaoValueItem *vit = new DaoValueItem( (DaoValue*)debugProcess );
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
		if( it->type == DAO_PROCESS && it->text() == "Process[Debugging]" ){
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

