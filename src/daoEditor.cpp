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

#include<QFile>
#include<QFileInfo>
#include<QTextStream>
#include<QMouseEvent>
#include<QPaintEvent>
#include<QPainter>
#include<QVBoxLayout>
#include<QScrollBar>
#include<QMessageBox>
#include<QApplication>
#include<QTextCodec>
#include<QLocalSocket>
#include<QClipboard>

#include<daoEditor.h>
#include<daoStudio.h>
#include<daoStudioMain.h>

#include<math.h>

void DaoTabEditor::focusInEvent ( QFocusEvent * event )
{
	//emit signalFocusIn();
	QTabWidget::focusInEvent( event );
}
void DaoTabEditor::mousePressEvent ( QMouseEvent * event )
{
	emit signalFocusIn();
	QTabWidget::mousePressEvent( event );
}
const char* const DaoKeyWords[] =
{
	//	"use",
	"load",
	"import",
	"require",
	//	"by",
	//	"as",
	"syntax",
	"typedef",
	"final",
	"class",
	"sub",
	"routine",
	"function",
	"self",
	"none",
	//	"int",
	"float",
	"double",
	"complex",
	"string",
	"long",
	"array",
	"tuple",
	//	"map",
	"list",
	//	"any",
	//	"pair",
	"curry",
	"stream",
	"buffer",
	//	"io",
	//	"std",
	"math",
	"reflect",
	"coroutine",
	"network",
	//	"mpi",
	"thread",
	"mtlib",
	"mutex",
	"condition",
	"semaphore",
	//	"and",
	//	"or",
	//	"not",
	//	"if",
	"else",
	//	"for",
	//	"in",
	//	"do",
	"while",
	"until",
	"switch",
	"case",
	"default",
	"break",
	"skip",
	"return",
	"yield",
	//	"my",
	"const",
	"local",
	"global",
	"enum",
	"private",
	"protected",
	"public",
	"virtual",
	//	"try",
	"retry",
	"rescue",
	"raise",
	"async",
	"hurry",
	"join",

	"fold",
	"unfold",
	"reduce",
	"select",
	"index",
	"count",
	"each",
	"repeat",
	"sort",
	"apply",

	// frequently used methods:
	"size",
	"write",
	"writef",
	"writeln",
	"backup",
	"restore",
	NULL,
};
DaoWordList::DaoWordList( int n, int m )
{
	lexer = DaoLexer_New();
	numComplete = n;
	minLength = m;
	int i = 0;
	while( DaoKeyWords[i] ){
		QString word( DaoKeyWords[i++] );
		if( word.size() <m || wordList.find( word ) != wordList.end() ) continue;
		wordList[ word ] = 1;
	}
}
void DaoWordList::Extract( const QString & source )
{
	size_t i, j;
	DaoLexer_Tokenize( lexer, source.toLocal8Bit().data(), 0 );
	for(i=0; i<lexer->tokens->size; i++){
		DaoToken *tk = lexer->tokens->items.pToken[i];
		if( tk->type == DTOK_IDENTIFIER ) AddWord( tk->string.mbs );
	}
	return;
	for(i=0; i+1<lexer->tokens->size; i++){
		DaoToken *tk = lexer->tokens->items.pToken[i];
		switch( tk->name ){
		case DKEY_SUB :
		case DKEY_ENUM :
		case DKEY_CLASS :
		case DKEY_ROUTINE :
		case DKEY_FUNCTION :
			AddWord( lexer->tokens->items.pToken[i+1]->string.mbs );
			break;
		case DKEY_VAR :
		case DKEY_CONST :
		case DKEY_STATIC :
		case DKEY_GLOBAL :
			if( i+2 >= lexer->tokens->size ) break;
			for(j=1; j<=2; j++){
				tk = lexer->tokens->items.pToken[i+j];
				if( tk->type == DTOK_IDENTIFIER && tk->name <= DAO_NOKEY1 )
					AddWord( tk->string.mbs );
			}
			break;
		case DTOK_DOT :
			tk = lexer->tokens->items.pToken[i+1];
			if( tk->type == DTOK_IDENTIFIER ) AddWord( tk->string.mbs );
			break;
		default : break;
		}
	}
}
static int StringCommon( const QString & s1, const QString & s2 )
{
	int n1 = s1.size();
	int n2 = s2.size();
	int i=0, m = n1 < n2 ? n1 : n2;
	for(i=0; i<m; i++) if( s1[i] != s2[i] ) return i;
	return 0;
}
DaoWordNode::DaoWordNode( const QString & word )
{
#if 0
	data = 0;
	count = 0;
	memset( nexts, 0, 256*sizeof(DaoWordNode*) );
#else
	if( word.size() == 0 ) return;
	data = word[0];
	count = 0;
	if( word.size() > 1 ) nexts[ word[1] ] = DaoWordNode( word.mid( 1 ) );
#endif
}
DaoWordNode::DaoWordNode( const QChar *chars, int n )
{
	if( chars == NULL or n ==0 ) return;
	data = chars[0];
	count = 1;
	if( n >1 ) nexts[ chars[1] ] = DaoWordNode( chars+1, n-1 );
}
#if 0
void DaoWordNode::SetWord( const QString & word = "" )
{
	QByteArray bytes = word.toUtf8();
	unsigned char *utf8 = bytes.data();
	int n = bytes.size();
	if( n ==0 ) return;
	data = utf8[0]
}
#endif
DaoWordNode::~DaoWordNode()
{
}
void DaoWordList::AddWord( const QString & word )
{
#if 0
	if( word.size() < minLength ) return;
	if( wordList.find( word ) != wordList.end() ) return;
	wordList[ word ] = 1;
	return;
#if 1
	DaoWordNode *node = & wordTree;
	{
		int i, n = word.size();
		for(i=0; i<n; i++){
			QChar ch = word[i];
			if( 0 and node->nexts.contains( ch ) ){
				node = & node->nexts[ch];
				node->count ++;
				continue;
			}else{
				//node->nexts[ch] = DaoWordNode( word.mid( i ) );
				node->nexts[ch] = DaoWordNode( word.mid( i ).data(), n-i );
				break;
			}
		}
	}
#endif
	return;
#endif

	if( word.size() < minLength ) return;
	if( wordList.find( word ) != wordList.end() ) return;
	QMap<QString,int>::iterator it1, it2, it, end;
	wordList[ word ] = 1;
	it = wordList.find( word );
	end = wordList.end();
	it1 = --it;
	it2 = ++it;
	int c, n, i=0, j=0, c1 = 0, c2 = 0;
	while( it1 != end ){
		c = StringCommon( word, it1.key() );
		if( c < c1 ) break;
		c1 = c;
		++i;
		--it1;
	}
	while( it2 != end ){
		c = StringCommon( word, it2.key() );
		if( c < c2 ) break;
		c2 = c;
		++j;
		++it2;
	}
	c = 0;
	n = 0;
	if( c1 > c2 && c1 >= minLength && i > numComplete ){
		c = c1;
		n = i;
	}else if( c2 > c1 && c2 >= minLength && j > numComplete ){
		c = c2;
		n = j;
	}else if( c1 == c2 && c1 >= minLength && (i+j) >numComplete ){
		c = c1;
		n = i + j;
	}
	if( c ){
		QString extra = word.mid( 0, c );
		if( wordList.find( extra ) == end ) wordList[ extra ] = n;
	}
	//printf( "%i;  %i  %i;  %i  %i\n", c, i, c1, j, c2 );
}
void DaoWordList::AddWords( const QList<QString> & words )
{
	for(int i=0; i<words.size(); i++) AddWord( words[i] );
}
QStringList DaoWordList::GetWords( const QString & word )
{
	QStringList wl;
	QMap<QString,int>::iterator it = wordList.upperBound( word );
	if( it == wordList.end() ) return wl;
	if( word.size() ==0 ) return wl;
	for(; it != wordList.end(); ++it){
		//printf( "%s\n", it.key().toLocal8Bit().data() );
		if( it.key().indexOf( word ) !=0 ) break;
		wl<<it.key();
		if( wl.size() >= numComplete ) break;
	}
	return wl;
}

DaoWordPopup::DaoWordPopup( DaoTextEdit *parent, DaoWordList *wl ) : QWidget( parent )
{
	isTip = false;
	wordList = wl;
	editor = parent;
	QVBoxLayout *layout = new QVBoxLayout( this );
	int i;
	QString style = "color: black; background-color: rgb(150, 250, 150, 200);"
		"border-style: outset; border-width: 1px;"
		"border-radius: 10px; border-color: rgb(0, 250, 0, 120);";
	setStyleSheet( style );
	style = "background-color: rgb(255, 255, 255, 0); border: none;";
	for(i=0; i<3; i++){
		QLabel *lab = new QLabel( "", this );
		lab->setFont( DaoStudioSettings::codeFont );
		lab->setStyleSheet( style );
		layout->addWidget( lab );
		labWords.append( lab );
	}
	setLayout( layout );
	layout->setContentsMargins( 5,5,5,5 );
	layout->setSpacing( 0 );
}
void DaoWordPopup::SetTip( const QString & tip )
{
	isTip = true;
	labWords[0]->show();
	labWords[1]->hide();
	labWords[2]->hide();
	labWords[0]->setText( tip );
	resize( sizeHint() );
}
void DaoWordPopup::SetWords( const QStringList & words )
{
	int i;
	for(i=0; i<3; i++){
		if( i >= words.size() ){
			labWords[i]->hide();
		}else{
			labWords[i]->show();
			labWords[i]->setText( words[i] );
		}
	}
	resize( sizeHint() );
	isTip = false;
}
QString DaoWordPopup::GetWord( int i )
{
	if( i >= 3 || labWords[i]->isVisible() == false ) return "";
	return labWords[i]->text();
}
void DaoWordPopup::EnsureVisible( int xoffset, bool exactpos )
{
	QRect rect = editor->cursorRect();
	int x = rect.x()+xoffset+5;
	int y = rect.y()+rect.height()+5;
	if( x + width() >= editor->width() ) x = editor->width()-width()-5;
	if( y + height() >= editor->height() ) y = rect.y()-5-height();
	if( exactpos ) y = rect.y();
	move( x, y );
	show();
}
void DaoWordPopup::TryCompleteWord( QKeyEvent *event, int xoffset )
{
	int key = event->key();
	int mod = event->modifiers();
	if( wordList == NULL ) return;
	QString txt = event->text();
	if( isVisible() && key != Qt::Key_Control &&
			( (mod & Qt::ControlModifier) || key == Qt::Key_Tab) ){
		QTextCursor cursor = editor->textCursor();
		cursor.movePosition( QTextCursor::Left );
		cursor.select( QTextCursor::WordUnderCursor );
		QString txt = cursor.selectedText();
		QString pro;
		switch( key ){
		case Qt::Key_Tab :
		case Qt::Key_J : pro = GetWord( 0 ); break;
		case Qt::Key_K : pro = GetWord( 1 ); break;
		case Qt::Key_L : pro = GetWord( 2 ); break;
		default: hide(); break;
		}
		if( pro.size() > txt.size() ){
			//printf( "proposed:  %s\n", pro.toLocal8Bit().data() );
			if( key == Qt::Key_Tab ){
				QTextCursor cursor = editor->textCursor();
				cursor.movePosition( QTextCursor::Left, QTextCursor::KeepAnchor );
				/* for some reason there is a duplicated tab key event */
				if( cursor.selectedText() != "\t" ) return;
				cursor.removeSelectedText();
			}
			editor->insertPlainText( pro.mid( txt.size() ) );
			QStringList words = wordList->GetWords( pro );
			if( words.size() ==0 ){ hide(); return; }
			SetWords( words );
			EnsureVisible( xoffset );
		}
	}else if( (key >= Qt::Key_A && key <= Qt::Key_Z) || key == Qt::Key_Underscore
			|| key == Qt::Key_Control || (key >= Qt::Key_0 && key <= Qt::Key_9) ){
		QTextCursor cursor = editor->textCursor();
		cursor.movePosition( QTextCursor::Right, QTextCursor::KeepAnchor );
		QString ch = cursor.selectedText();
		if( ch.size() and ch[0].isLetterOrNumber() ){ hide(); return; }
		cursor = editor->textCursor();
		cursor.movePosition( QTextCursor::Left );
		cursor.select( QTextCursor::WordUnderCursor );
		QString txt = cursor.selectedText();
		if( txt.size() and txt == ch ){ hide(); return; }
		QStringList words = wordList->GetWords( txt );
		if( words.size() ==0 ){ hide(); return; }
		SetWords( words );
		EnsureVisible( xoffset );
		//printf( "%i  %i  %s\n", words.size(), bl, txt.toLocal8Bit().data() );
	}else{
		hide();
	}
}

void DaoNumbering::paintEvent ( QPaintEvent * event )
{
	editor->PaintNumbering( event );
}
void DaoNumbering::mouseDoubleClickEvent( QMouseEvent * event )
{
	editor->ChangeMark( event->y() );
	repaint();
}
void DaoNumbering::mousePressEvent ( QMouseEvent * event )
{
	if( event->button() != Qt::RightButton ) return;
	editor->MarkLine( event->y() );
	repaint();
}

void DaoLangLabels::paintEvent( QPaintEvent * event )
{
	editor->PaintQuickScroll( event );
}
void DaoLangLabels::mouseDoubleClickEvent( QMouseEvent * event )
{
}
const int zoom_fonts[] = { 0, 9, 4, 1 };
void DaoLangLabels::mousePressEvent ( QMouseEvent * event )
{
	int x = event->x();
	int y = event->y();
	if( y < (top-5) or y > (bottom+5) ) return;
	QTextCursor cursor = editor->textCursor();
	int block = cursor.block().blockNumber();
	if( x < 0 or x > width() ){
		if( zoom ){
			zoom -= 1;
			if( zoom ==0 ){
				releaseMouse();
				editor->SetFontSize( DaoStudioSettings::codeFont.pointSize() );
			}else{
				editor->SetFontSize( zoom_fonts[zoom] );
			}
		}
	}else if( zoom ){
		if( zoom < 3 ) zoom += 1;
		editor->SetFontSize( zoom_fonts[zoom] );
	}else{
		zoom += 1;
		grabMouse();
		editor->SetFontSize( zoom_fonts[zoom] );
	}
	editor->ensureCursorVisible();
	editor->UpdateCursor( true );
	editor->setFocus();
}

void DaoLangLabels::mouseMoveEvent ( QMouseEvent * event )
{
	int x = event->x();
	if( zoom == 0 ){
		update();
		return;
	}
	int i, y = event->y();
	ymouse = y;
	int lines = editor->blockCount();
	int line = (y * lines) / height();
	QTextCursor cursor = editor->textCursor();
	int oldln = cursor.block().blockNumber();
	QTextCursor cursor2;
	bool found = false;
	if( line > oldln ){
		for(i=oldln; i<line; i++){
			cursor.movePosition( QTextCursor::Down, QTextCursor::MoveAnchor );
			if( i+5 >= line ){
				QTextBlock block = cursor.block();
				DaoCodeLineData *ud = (DaoCodeLineData*) block.userData();
				if( ud and (ud->def_class or ud->def_method) ){
					cursor2 = cursor;
					found = true;
				}
			}
		}
	}else{
		cursor.movePosition( QTextCursor::Up, QTextCursor::MoveAnchor, oldln-line );
		for(i=oldln; i>line; i--){
			cursor.movePosition( QTextCursor::Up, QTextCursor::MoveAnchor );
			if( i-5 <= line ){
				QTextBlock block = cursor.block();
				DaoCodeLineData *ud = (DaoCodeLineData*) block.userData();
				if( ud and (ud->def_class or ud->def_method) ){
					cursor2 = cursor;
					found = true;
				}
			}
		}
	}
	if( found ) cursor = cursor2;
	editor->setTextCursor( cursor );
	editor->ensureCursorVisible();
}
void DaoLangLabels::enterEvent ( QEvent * event )
{
	if( not editor->codeThumb->isVisible() ){
		editor->codeThumb->show();
		editor->codeThumb->update();
		editor->codeThumb->horizontalScrollBar()->setSliderPosition(0);
	}
	return;
	if( not hasMouseTracking() ) return;
	entered = true;
}
void DaoLangLabels::leaveEvent ( QEvent * event )
{
	return;
	if( not hasMouseTracking() ) return;
	if( ymouse < (top-5) or ymouse > (bottom+5) ) return;
	entered = false;
	editor->SetFontSize( DaoStudioSettings::codeFont.pointSize() );
}

QStringList DaoCommandEdit::searchHistory;
QStringList DaoCommandEdit::commandHistory;
QStringList DaoCommandEdit::historyBrowsing;
int DaoCommandEdit::indexBrowsing = -1;

DaoCommandEdit::DaoCommandEdit( DaoEditWidget *parent ) : QLineEdit( parent )
{
	search = false;
	wgtParent = parent;
	connect( this, SIGNAL(textChanged(const QString &)),
			this, SLOT(slotTextChanged(const QString &)));
}
void DaoCommandEdit::slotTextChanged( const QString &text )
{
	QRect rect = cursorRect();
	int w = width();
	if( rect.x() > 0.9 * w and w < 0.8*wgtParent->parentWidget()->width() ){
		resize( 1.5*w, height() );
		wgtParent->resize( wgtParent->width() + 0.5*w, wgtParent->height() );
		wgtParent->EnsureVisible( false );
	}
}
void DaoCommandEdit::keyPressEvent( QKeyEvent *event )
{
	int key = event->key();
	if( key == Qt::Key_Up ){
		if( historyBrowsing.size() ==0 ) return;
		indexBrowsing -= 1;
		if( indexBrowsing < 0 ) indexBrowsing = historyBrowsing.size() - 1;
		setText( historyBrowsing[ indexBrowsing ] );
	}else if( key == Qt::Key_Down ){
		if( historyBrowsing.size() ==0 ) return;
		indexBrowsing += 1;
		if( indexBrowsing >= historyBrowsing.size() ) indexBrowsing = 0;
		setText( historyBrowsing[ indexBrowsing ] );
	}else{
		QLineEdit::keyPressEvent( event );
		FilterHistory( search ? searchHistory : commandHistory, text() );
	}
}
void DaoCommandEdit::FilterHistory( QStringList & history, const QString prefix )
{
	if( prefix.size() ==0 ){
		historyBrowsing = history;
		return;
	}
	historyBrowsing.clear();
	int i;
	for(i=0; i<history.size(); i++){
		QString item = history[i];
		if( item.indexOf( prefix ) ==0 ) historyBrowsing.append( item );
	}
	historyBrowsing.append( prefix );
	indexBrowsing = historyBrowsing.size() - 1;
}
void DaoCommandEdit::FilterSearchHistory( const QString prefix )
{
	FilterHistory( searchHistory, prefix );
}
void DaoCommandEdit::FilterCommandHistory( const QString prefix )
{
	FilterHistory( commandHistory, prefix );
}
DaoEditWidget::DaoEditWidget( DaoTextEdit *parent ) : QWidget( parent )
{
	QVBoxLayout *vbox = new QVBoxLayout;
	QHBoxLayout *hbox = new QHBoxLayout;
	setLayout(vbox);
	editor = parent;
	labMessage = new QLabel( "", this );
	labPrompt = new QLabel( ":", this );
	cmdEdit = new DaoCommandEdit( this );
	hbox->addWidget( labPrompt );
	hbox->addWidget( cmdEdit );
	vbox->addLayout( hbox );
	vbox->addWidget( labMessage );

	QString style = "background-color: rgb(150, 250, 150, 200);"
		"border-style: outset; border-width: 1px;"
		"border-radius: 8px; border-color: rgb(0, 250, 0, 120);";
	setStyleSheet( style );
	style = "color: black; background-color: rgb(220, 250, 220, 220);"
		"border-radius: 4px; border-color: rgb(0, 150, 0, 200);";
	cmdEdit->setStyleSheet( style );
	style = "font-weight: bold; color: red;"
		"border-radius: 4px; border-width: 0px;";
	labPrompt->setStyleSheet( style );
	style = "font-weight: bold; color: red;"
		"border-radius: 4px; border-width: 1px;";
	labMessage->setStyleSheet( style );
	labMessage->hide();

	connect( cmdEdit, SIGNAL(returnPressed()),
			parent, SLOT(slotSearchOrReplace()));
}
void DaoEditWidget::StartSearch()
{
	cmdEdit->SetMode( true );
	labMessage->hide();
	labPrompt->setText( "/" );
	resize( sizeHint() );
	QTextCursor cursor = editor->textCursor();
	if( cursor.hasSelection() ){
		//QString txt = cursor.selectedText();
		if( DaoTextEdit::buftype == BUF_WORDS )
			cmdEdit->setText( "{{" + DaoTextEdit::buffer + "}}" );
	}
	EnsureVisible();
	cmdEdit->setFocus();
	DaoCommandEdit::FilterSearchHistory( cmdEdit->text() );
}
void DaoEditWidget::StartCommand()
{
	cmdEdit->SetMode( false );
	labMessage->hide();
	labPrompt->setText( ":" );
	resize( sizeHint() );
	EnsureVisible();
	cmdEdit->setFocus();
	DaoCommandEdit::FilterCommandHistory( cmdEdit->text() );
}
void DaoEditWidget::AppendSearchHistory( const QString text )
{
	DaoCommandEdit::searchHistory.removeAll( text );
	DaoCommandEdit::searchHistory.append( text );
}
void DaoEditWidget::AppendCommandHistory( const QString text )
{
	DaoCommandEdit::commandHistory.removeAll( text );
	DaoCommandEdit::commandHistory.append( text );
}
void DaoEditWidget::EnsureVisible( bool select )
{
	QRect rect = editor->cursorRect();
	int xoffset = editor->LeftMargin();
	int x = rect.x()+xoffset+5;
	int y = rect.y()+rect.height()+5;
	if( x + width() >= editor->width() ) x = editor->width()-width()-5;
	if( y + height() >= editor->height() ) y = rect.y()-5-height();
	move( x, y );
	show();
	if( select ) cmdEdit->setSelection( 0, cmdEdit->maxLength() );
}
void DaoEditWidget::HideMessage()
{
	labMessage->setText( "" );
	labMessage->hide();
	resize( sizeHint() );
}
void DaoEditWidget::ShowMessage( const QString & txt )
{
	EnsureVisible();
	labMessage->show();
	labMessage->setText( txt );
	resize( sizeHint() );
}
void DaoEditWidget::AddMessage( const QString & txt )
{
	QString txt2 = labMessage->text();
	if( txt2.size() ) txt2 += "; ";
	txt2 += txt;
	ShowMessage( txt2 );
}

class DaoCursor : public QWidget
{
	DaoTextEdit *editor;
	public:
	DaoCursor( DaoTextEdit *parent=NULL ) : QWidget( parent ){ editor = parent; }

	protected:
	void keyPressEvent( QKeyEvent * e ){ e->ignore(); }
	void keyReleaseEvent( QKeyEvent * e ){ e->ignore(); }
	void mousePressEvent( QMouseEvent * e ){ e->ignore(); }
	void mouseReleaseEvent( QMouseEvent * e ){ e->ignore(); }
	void mouseDoubleClickEvent( QMouseEvent * e ){
		QPoint pos = editor->viewport()->mapFromGlobal( e->globalPos() );
		//QPoint pos2 = mapToParent( e->pos() );
		//QPoint pos( pos2.x() - editor->LeftMargin(), pos2.y() );
		QMouseEvent event( e->type(), pos, e->button(), e->buttons(), e->modifiers() );
		editor->mouseDoubleClickEvent( & event );
	}
	void contextMenuEvent(QContextMenuEvent *event){
		editor->contextMenuEvent( event );
	}
};
class DaoCursorLabel : public QLabel
{
	public:
		DaoCursorLabel( QWidget *parent=NULL ) : QLabel( parent ){}

	protected:
		void keyPressEvent( QKeyEvent * e ){ e->ignore(); }
		void keyReleaseEvent( QKeyEvent * e ){ e->ignore(); }
		void mousePressEvent( QMouseEvent * e ){ e->ignore(); }
		void mouseReleaseEvent( QMouseEvent * e ){ e->ignore(); }
		void mouseDoubleClickEvent( QMouseEvent * e ){ e->ignore(); }
};


int DaoTextEdit::buftype = BUF_EMPTY;
QString DaoTextEdit::buffer;
QHash<QString,DaoRegex*> DaoTextEdit::patterns;

DaoTextEdit::DaoTextEdit( QWidget *parent, DaoWordList *wlist ) : 
	QPlainTextEdit( parent ), codehl( this->document() )
{
	blockEntry = document()->end();
	lineColor = QColor(Qt::yellow).lighter(170);
	setFont( DaoStudioSettings::codeFont );
	codehl.SetState(DAO_HLSTATE_NORMAL);
	wordList = wlist;
	wgtWords = new DaoWordPopup( this, wlist );
	wgtWords->hide();
	labPopup = new QLabel( "", this );
	QString style = "background-color: rgb(150, 250, 150, 200);"
		"border-style: outset; border-width: 1px;"
		"border-radius: 10px; border-color: rgb(0, 250, 0, 120);";
	labPopup->setStyleSheet( style );
	labPopup->hide();

	cmdPrompt = new DaoEditWidget( this );
	cmdPrompt->hide();

	myCursor = new DaoCursor( this );
	//labCursor = new DaoCursorLabel( myCursor );
	style = "background-color: rgb(0, 0, 255, 127);"
		"border-style: solid; border-width: 1px;"
		"border-color: rgb(0, 0, 255, 255);";
	myCursor->setStyleSheet( style );
	style = "background-color: rgb(150, 250, 150, 100); border: none;";
	//labCursor->setStyleSheet( style );
	//labCursor->resize( 0, 0 );
	myCursor->hide();
	showCursor = false;

	tabWidth = 4;
	QFontMetrics fm( DaoStudioSettings::codeFont );
	// tab stop width should be slightly smaller than font width
	// of tabWidth letters, so that ther will be no more very narrow tab: 
	// class WordCou   ntTable
	QString tab = "";
	int i;
	for(i=0; i<tabWidth; i++) tab += ' ';
	setTabStopWidth( fm.width( tab )-5 );
	setCursorWidth( fm.width( 'M' ) );
	setCursorWidth( 0 );
	//QPalette p = palette();
	//p.setColor( QPalette::Text, QColor( 255, 255, 0, 200) );
	//setPalette( p );
	////if( parentWidget() ) parentWidget()->setPalette( p );
	wgtEditMode = NULL;
	keyMode = KEY_MODE_EDIT;
	editMode = 0;

	connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(HighlightCurrentLine()));
	connect( &timer, SIGNAL(timeout()), this, SLOT(slotBlink()));
	timer.start( 700 );

	mbs = DString_New(1);
	wcs = DString_New(0);
	lexer = DaoLexer_New();
}
DaoTextEdit::~DaoTextEdit()
{
	DString_Delete( mbs );
	DString_Delete( wcs );
	DaoLexer_Delete( lexer );
}
void DaoTextEdit::Undo()
{
	undo();
	UpdateCursor( showCursor );
}
void DaoTextEdit::Redo()
{
	redo();
	UpdateCursor( showCursor );
}
void DaoTextEdit::slotBlink()
{
	if( showCursor ) myCursor->setVisible( not myCursor->isVisible() );
}
void DaoTextEdit::UpdateCursor( bool show )
{
	if( show ){
		QFont font = GetFont();
		QTextBlock block = textCursor().block();
		DaoCodeLineData *ud = (DaoCodeLineData*) block.userData();
		if( ud and ud->font_size ) font.setPointSize( ud->font_size );
		QFontMetrics fm( font );
		QRect rect = cursorRect();
		QString txt = "M";
		int w = fm.width( txt );
		int h = rect.height();
		if( w < 5 ){
			w += 2;
			h += 2;
		}
		myCursor->show();
		myCursor->resize( w, h );
		myCursor->move( rect.x()+LeftMargin()+1, rect.y()+2 );
		showCursor = true;
	}else{
		showCursor = false;
		myCursor->hide();
	}
}
void DaoTextEdit::HighlightFullLine( QTextCursor cursor )
{
	QList<QTextEdit::ExtraSelection> selections = this->extraSelections();
	QTextEdit::ExtraSelection selection;
	selection.format.setBackground(lineColor);
	selection.format.setProperty(QTextFormat::FullWidthSelection, true);
	selection.cursor = cursor;
	selection.cursor.clearSelection();
	selections.append(selection);
	setExtraSelections(selections);
}
void DaoTextEdit::scrollContentsBy( int dx, int dy )
{
	QPlainTextEdit::scrollContentsBy( dx, dy );
	QPointF offset = contentOffset();
	QRect rect = cursorRect();
	int height = viewport()->height();
	if( rect.y() < offset.y() ){
		rect.setY( offset.y() );
	}else if( rect.y() + rect.height() > offset.y() + height ){
		rect.setY( offset.y() + height - rect.height() );
	}else{
		UpdateCursor( showCursor );
		return;
	}
	QTextCursor cursor = cursorForPosition( rect.topLeft() );
	setTextCursor( cursor );
	UpdateCursor( showCursor );
}
void DaoTextEdit::HighlightCurrentLine()
{
	UpdateCursor( true );
}
void DaoTextEdit::focusInEvent( QFocusEvent * event )
{
	UpdateCursor( true );
}
void DaoTextEdit::focusOutEvent( QFocusEvent * event )
{
	UpdateCursor( false );
}
void DaoTextEdit::ShowMessage( const QString & msg )
{
	QRect rect = cursorRect();
	int x = rect.x() + LeftMargin() + 5;
	int y = rect.y() + rect.height() + 5;
	int h = labPopup->height();
	if( y + h > height() ) y = rect.y()-5-h;
	wgtWords->hide();
	labPopup->setText( msg );
	labPopup->move( x, y );
	labPopup->show();
}
void DaoTextEdit::SetFontSize( int size )
{
	QFont font = DaoStudioSettings::codeFont;
	font.setPointSize( size );
	setFont( font );
	codehl.SetFontSize( size );
	QFontMetrics fm( DaoStudioSettings::codeFont );
	// tab stop width should be slightly smaller than font width
	// of tabWidth letters, so that ther will be no more very narrow tab: 
	// class WordCou   ntTable
	QString tab = "";
	int i;
	for(i=0; i<tabWidth; i++) tab += ' ';
	setTabStopWidth( fm.width( tab ) - 5 );
	RedoHighlight();
	UpdateCursor( true );
}
void DaoTextEdit::SetFontFamily( const QString & family )
{
	codehl.SetFontFamily( family );
	RedoHighlight();
	UpdateCursor( true );
}
void DaoTextEdit::SetColorScheme( int scheme )
{
	QTextCharFormat oldFormatOutput = codehl.formatOutput;
	codehl.SetColorScheme( scheme );
	switch( scheme ){
	case 0 :
		lineColor = QColor(200, 200, 120);
		viewport()->setStyleSheet( "background-color: #FFFFFF" );
		break;
	case 1 :
		lineColor = QColor(200, 200, 100);
		viewport()->setStyleSheet( "background-color: #DDDDDD" );
		break;
	case 2 :
		lineColor = QColor(80, 80, 0);
		viewport()->setStyleSheet( "background-color: #222222" );
		break;
	default :
		lineColor = QColor(80, 80, 40);
		viewport()->setStyleSheet( "background-color: #000000" );
		break;
	}
	QList<QTextEdit::ExtraSelection> selections = extraSelections();
	int i;
	for(i=0; i<selections.size(); i++){
		selections[i].format.setBackground(lineColor);
	}
	setExtraSelections(selections);
	RedoHighlight();
}
void DaoTextEdit::SetTabVisibility( int vid )
{
	codehl.SetTabVisibility( vid );
	RedoHighlight();
}
void DaoTextEdit::ExtractWords()
{
	QString source = toPlainText();
	DaoLexer *lexer = codehl.lexer;
	size_t i, j;
	if( codehl.language ){
		codehl.language->Tokenize( lexer, source.toLocal8Bit().data() );
	}else{
		DaoLexer_Tokenize( lexer, source.toLocal8Bit().data(), 0 );
	}
	for(i=0; i<lexer->tokens->size; i++){
		DaoToken *tk = lexer->tokens->items.pToken[i];
		if( tk->type == DTOK_IDENTIFIER and tk->string.size >=5 ){
			wordList->AddWord( tk->string.mbs );
		}
	}
}
void DaoTextEdit::slotSearchOrReplace()
{
	QString prompt = cmdPrompt->GetPrompt();
	QString command = cmdPrompt->GetCommand().trimmed();
	if( command.size() ==0 ){
		DaoRegex *oldpat = codehl.SetPattern( NULL );
		if( oldpat != NULL ) RedoHighlight();
		return;
	}
	if( prompt == "/" ){
		if( not patterns.contains( command ) ){
			QByteArray bytes = command.toUtf8();
			DString *wcs = DString_New(0);
			DString_SetDataMBS( wcs, bytes.data(), bytes.size() );
			patterns[ command ] = DaoRegex_New( wcs );
			DString_Delete( wcs );
		}
		DaoRegex *newpat = patterns[ command ];
		DaoRegex *oldpat = codehl.SetPattern( newpat );
		if( newpat != oldpat ) RedoHighlight();
		MoveSearchingCursor( textCursor().position() + (newpat == oldpat) );
		cmdPrompt->AppendSearchHistory( command );
		return;
	}
	cmdPrompt->AppendCommandHistory( command );
	cmdPrompt->HideMessage();
	QTextCursor cursor = textCursor();
	QByteArray bytes = command.toUtf8();
	DString_SetDataMBS( wcs, bytes.data(), bytes.size() );
	int i=0, n=wcs->size;
	int start = cursor.blockNumber() + 1;
	int end = start;
	int num = 0;
	int pm = 0;
	if( n == 0 ) return;
	if( wcs->wcs[0] == L'.' ){
		start = cursor.blockNumber() + 1;
		end = start;
		i += 1;
	}else if( wcs->wcs[0] == L'$' ){
		start = end = blockCount();
		i += 1;
	}else if( wcs->wcs[0] == L'%' ){
		start = 1;
		end = blockCount();
		i += 1;
		if( wcs->wcs[i] == L',' ){
			cmdPrompt->ShowMessage( tr("Unexpected comma") );
			return;
		}
	}else if( iswdigit( wcs->wcs[0] ) ){
		start = 0;
		while( iswdigit( wcs->wcs[i] ) ){
			start = start*10 + (wcs->wcs[i] - L'0');
			i += 1;
		}
		end = start;
	}
	if( wcs->wcs[i] == L',' ){
		i += 1;
		if( wcs->wcs[i] == L'.' ){
			end = cursor.blockNumber() + 1;
			i += 1;
		}else if( wcs->wcs[i] == L'$' ){
			end = blockCount();
			i += 1;
		}else{
			if( wcs->wcs[i] == L'+' ){
				pm = 1;
				i += 1;
			}else if( wcs->wcs[i] == L'-' ){
				pm = -1;
				i += 1;
			}
			if( not iswdigit( wcs->wcs[i] ) ){
				cmdPrompt->ShowMessage( tr("Invalid format") );
				return;
			}
			num = 0;
			while( iswdigit( wcs->wcs[i] ) ){
				num = num*10 + (wcs->wcs[i] - L'0');
				i += 1;
			}
			end = num;
			if( pm >0 ){
				end = start + num;
			}else if( pm <0 ){
				end = start;
				start = end - num;
			}
			if( start <= 0 ) start = 1;
		}
	}
	int count = blockCount();
	int start2 = start;
	int start3 = start;
	int end2 = end;
	int end3 = end;
	if( start3 >= count ) start3 = count;
	if( end3 >= count ) end3 = count;
	QTextBlock block = document()->findBlockByNumber( start3-1 );
	cursor = QTextCursor( block );
	//cursor.movePosition( QTextCursor::Start );
	//cursor.movePosition( QTextCursor::NextBlock, QTextCursor::MoveAnchor, start-1 );
	if( start == end and i == wcs->size ){
		setTextCursor( cursor );
		ensureCursorVisible();
		cmdPrompt->EnsureVisible();
		return;
	}
	AdjustCursor( & cursor );
	AdjustMove( & cursor, Qt::Key_Up );
	cursor.movePosition( QTextCursor::StartOfBlock );
	int pstart = cursor.position();
	start2 = cursor.block().blockNumber() + 1;
	//cursor.movePosition( QTextCursor::Start );
	//cursor.movePosition( QTextCursor::NextBlock, QTextCursor::MoveAnchor, end-1 );
	block = document()->findBlockByNumber( end3-1 );
	cursor = QTextCursor( block );
	AdjustCursor( & cursor );
	AdjustMove( & cursor, Qt::Key_Down );
	cursor.movePosition( QTextCursor::EndOfBlock );
	int pend = cursor.position();
	end2 = cursor.block().blockNumber() + 1;
	if( start2 > end or start > end2 ){
		cmdPrompt->ShowMessage( tr("Invalid range") 
				+ " " + QString::number( start2 )
				+ "-" + QString::number( end2 ) );
		return;
	}else if( start2 != start or end2 != end ){
		cmdPrompt->ShowMessage( tr("Range adjusted to") 
				+ " " + QString::number( start2 )
				+ "-" + QString::number( end2 ) );
	}
	start = start2;
	end = end2;
	QString rest;
	if( i+1 < wcs->size ) rest = QString::fromWCharArray( wcs->wcs+i+1, wcs->size-i-1 );
	if( wcs->wcs[i] == L'q' ){
		setUndoRedoEnabled( false );
		setUndoRedoEnabled( true );
		cmdPrompt->hide();
		setFocus();
	}else if( wcs->wcs[i] == L'w' ){
		studio->slotSave();
		if( wcs->wcs[i+1] == L'q' ){
			setUndoRedoEnabled( false );
			setUndoRedoEnabled( true );
		}
		cmdPrompt->hide();
		setFocus();
	}else if( wcs->wcs[i] == L'd' ){
		cursor.setPosition( pstart );
		cursor.setPosition( pend, QTextCursor::KeepAnchor );
		if( not cursor.atEnd() ){
			cursor.movePosition( QTextCursor::Right, QTextCursor::KeepAnchor );
			DaoCodeLineData *ud = (DaoCodeLineData*)cursor.block().userData();
			if( ud && ud->state == CLS_READONLY )
				cursor.movePosition( QTextCursor::Left, QTextCursor::KeepAnchor );
		}
		buftype = BUF_BLOCKS;
		buffer = cursor.selectedText();
		cursor.removeSelectedText();
		setTextCursor( cursor );
		if( i+1 < wcs->size )
			cmdPrompt->AddMessage( tr("Unexpected: ") + rest );
	}else if( wcs->wcs[i] == L'y' ){
		cursor.setPosition( pstart );
		cursor.setPosition( pend, QTextCursor::KeepAnchor );
		if( not cursor.atEnd() ){
			cursor.movePosition( QTextCursor::Right, QTextCursor::KeepAnchor );
			DaoCodeLineData *ud = (DaoCodeLineData*)cursor.block().userData();
			if( ud && ud->state == CLS_READONLY )
				cursor.movePosition( QTextCursor::Left, QTextCursor::KeepAnchor );
		}
		buftype = BUF_BLOCKS;
		buffer = cursor.selectedText();
		setTextCursor( cursor );
		if( i+1 < wcs->size )
			cmdPrompt->AddMessage( tr("Unexpected: ") + rest );
	}else if( wcs->wcs[i] == L's' ){
		int slash = DString_FindWChar( wcs, L'/', i+2 );
		if( i+1 >= wcs->size || wcs->wcs[i+1] != L'/' || slash == MAXSIZE ){
			cmdPrompt->AddMessage( tr("need Dao regular expression") );
			return;
		}
		int offset = i + 2;
		DString *s = DString_New(0);
		DaoRegex *pat = NULL;
		while( slash != MAXSIZE ){
			DString_SetDataWCS( s, wcs->wcs+offset, slash-offset );
			pat = DaoRegex_New( s );
			bool valid = true;
			for( i=0; i<pat->count; i++ ){
				DaoRgxItem *it = pat->items + i;
				if( it->type ==0 ){
					if( it->length != (slash - offset) ){
						free( pat );
						DString_Delete( s );
						cmdPrompt->AddMessage( tr("Invalid Dao regular expression") + ", " 
								+ tr("at char ") + QString::number( it->length+offset ) );
						return;
					}
					valid = false;
					pat = NULL;
					free( pat );
					break;
				}
			}
			if( valid ) break;
			slash = DString_FindWChar( wcs, L'/', slash + 1 );
		}
		if( pat == NULL ){
			DString_Delete( s );
			cmdPrompt->AddMessage( tr("need valid Dao regular expression") );
			return;
		}
		QString rs = QString::fromWCharArray( s->wcs, s->size );
		patterns[ rs ] = pat;
		int size = wcs->size;
		int index = 1;
		cmdPrompt->AppendSearchHistory( rs );
		if( size > slash+2 ){
			if( wcs->wcs[size-1] == L'g' and wcs->wcs[size-2] == L'/' ){
				size -= 1;
				index = 0;
			}
		}
		if( size > slash+1 and wcs->wcs[size-1] == L'/' ) size -= 1;
		DString_SetDataWCS( s, wcs->wcs+slash+1, size - slash - 1 );
		DString *source = DString_New(0);

		int k=0;
		for(i=0; i<s->size; i++, k++){
			if( s->wcs[i] != L'\\' ){
				s->wcs[k] = s->wcs[i];
				continue;
			}
			if( i+1 >= s->size ) break;
			switch( s->wcs[i+1] ){
			case L't' : s->wcs[k] = L'\t'; break;
			case L'n' : s->wcs[k] = L'\n'; break;
			case L'r' : s->wcs[k] = L'\r'; break;
			default : s->wcs[k] = s->wcs[i+1]; break;
			}
			i += 1;
		}
		s->wcs[k] = L'\0';
		s->size = k;

		QTextBlock block = document()->findBlockByNumber( start-1 );
		for( ; block.isValid(); block = block.next() ){
			int bn = block.blockNumber() + 1;
			if( bn < start or bn > end ) continue;
			DaoCodeLineData *ud = (DaoCodeLineData*) block.userData();
			if( ud and ud->state == CLS_READONLY ) continue;
			QTextCursor c( block );
			c.movePosition( QTextCursor::EndOfBlock, QTextCursor::KeepAnchor );
			QByteArray bytes = c.selectedText().toUtf8();
			DString_SetDataMBS( source, bytes.data(), bytes.size() );
			int count = DaoRegex_Change( pat, source, s, index );
			if( count ==0 ) continue;
			QString rpl = QString::fromWCharArray( source->wcs, source->size );
			c.insertText( rpl );
		}
		DaoRegex *oldpat = codehl.SetPattern( pat );
		if( pat != oldpat ) RedoHighlight();
		DString_Delete( source );
		DString_Delete( s );
		//cmdPrompt->AddMessage( QString::fromWCharArray( s->wcs, s->size ) );
	}else{
		cmdPrompt->AddMessage( tr("Invalid operaion") 
				+ " \"" + wcs->wcs[i] + "\"" );
	}
	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText(buffer);
	//cmdPrompt->ShowMessage( QString::number( start ) + " " + QString::number( end ) );
}

bool DaoTextEdit::MoveSearchingCursor( QTextBlock block, int start )
{
	while( block.isValid() ){
		QTextLayout *layout = block.layout();
		if( layout == NULL ) break;
		QList<QTextLayout::FormatRange> ranges = layout->additionalFormats();
		QTextLayout::FormatRange range;
		int offset = block.position();
		int i, n = ranges.size();
		for(i=0; i<n; i++){
			range = ranges[i];
			if( (offset + range.start) < start ) continue;
			if( range.format.hasProperty( 0x100001 ) ){
				QTextCursor cursor = textCursor();
				cursor.setPosition( offset + range.start );
				setTextCursor( cursor );
				return true;
			}
		}
		block = block.next();
	}
	return false;
}
void DaoTextEdit::MoveSearchingCursor( int start )
{
	QTextCursor cursor = textCursor();
	QTextBlock block = cursor.block();
	bool bl = MoveSearchingCursor( block, start );
	cmdPrompt->hide();
	setFocus();
	if( not bl ){
		cmdPrompt->ShowMessage( tr("Searching from the beginning") );
		block = document()->firstBlock();
		bl = MoveSearchingCursor( block, 0 );
	}
	if( bl ){
		cmdPrompt->EnsureVisible();
	}else{
		cmdPrompt->ShowMessage( tr("Text pattern not found") );
	}
}
void DaoTextEdit::RedoHighlight()
{
	int i = 0, entry = -1;
	QTextBlock block = document()->firstBlock();
	while( block.isValid() ){
		if( block == blockEntry ) entry = i;
		block = block.next();
		++i;
	}
	BeforeRedoHighlight();
	short hlstate = codehl.hlstate;
	short textSkip = codehl.textSkip;
	codehl.hlstate = DAO_HLSTATE_REDO;
	codehl.textSkip = 0;
	codehl.rehighlight();
	codehl.hlstate = hlstate;
	codehl.textSkip = textSkip;
	i = 0;
	block = document()->firstBlock();
	while( block.isValid() ){
		if( i == entry ) blockEntry = block;
		SetIndentData( block );
		block = block.next();
	}
	AfterRedoHighlight();
	repaint();
}
void DaoTextEdit::PreparePrinting( const QString & name )
{
	int i = 0;
	int max = qMax(1, blockCount());
	int digits = QString::number( max ).size();
	QTextCursor cursor = textCursor();
	cursor.movePosition( QTextCursor::Start );
	while(1){
		QString number = QString::number( ++i );
		while( number.size() < digits ) number = " " + number;
		cursor.insertText( number + ": " );
		if( cursor.atEnd() ) break;
		cursor.movePosition( QTextCursor::NextBlock );
	}
	cursor.movePosition( QTextCursor::Start );
	if( name.size() ) cursor.insertText( tr("File: ") + "\"" + name  + "\"\n" );
	repaint();
}
void DaoTextEdit::AdjustMove( QTextCursor *cursor, int key, QTextCursor::MoveMode m )
{
	DaoCodeLineData *ud = (DaoCodeLineData*)cursor->block().userData();
	if( ud && ud->state == CLS_READONLY ){
		switch( key ){
		case Qt::Key_B :
		case Qt::Key_Up :
		case Qt::Key_Left :
			do{
				cursor->movePosition( QTextCursor::NextBlock, m );
				ud = (DaoCodeLineData*)cursor->block().userData();
			}while( ud && ud->state == CLS_READONLY && not cursor->atEnd() );
			break;
		case Qt::Key_W :
		case Qt::Key_Down :
		case Qt::Key_Right :
			do{
				cursor->movePosition( QTextCursor::PreviousBlock, m );
				cursor->movePosition( QTextCursor::EndOfBlock, m );
				ud = (DaoCodeLineData*)cursor->block().userData();
			}while( ud && ud->state == CLS_READONLY && not cursor->atStart() );
			break;
		default : return;
		}
	}
}
void DaoTextEdit::VimModeMove( int dir, int m )
{
	QTextCursor::MoveOperation op;
	switch( dir ){
	case Qt::Key_Up : op = QTextCursor::Up; break;
	case Qt::Key_Down : op = QTextCursor::Down; break;
	case Qt::Key_Left : op = QTextCursor::Left; break;
	case Qt::Key_Right : op = QTextCursor::Right; break;
	default : return;
	}
	QTextCursor cursor = textCursor();
	cursor.movePosition( op, QTextCursor::MoveAnchor, m );

	AdjustCursor( & cursor );
	AdjustMove( & cursor, dir );

	setTextCursor( cursor );
	ensureCursorVisible();
}
void DaoTextEdit::VimModeCopyDelete( int dir, int m, bool copy )
{
	QTextCursor cursor = textCursor();
	QTextCursor::MoveOperation op;
	switch( dir ){
	case Qt::Key_Up :
		cursor.movePosition( QTextCursor::EndOfBlock );
		if( copy and not cursor.atEnd() ){
			cursor.movePosition( QTextCursor::Right );
			cursor.movePosition( QTextCursor::Left, QTextCursor::KeepAnchor );
		}
		op = QTextCursor::Up; break;
	case Qt::Key_Down :
		cursor.movePosition( QTextCursor::StartOfBlock );
		op = QTextCursor::Down; break;
	case Qt::Key_Left : op = QTextCursor::Left; break;
	case Qt::Key_Right : op = QTextCursor::Right; break;
	default : return;
	}
	AdjustCursor( & cursor );
	cursor.movePosition( op, QTextCursor::KeepAnchor, m-1 );
	switch( dir ){
	case Qt::Key_Up :
		cursor.movePosition( QTextCursor::StartOfBlock, QTextCursor::KeepAnchor );
		if( not copy ) cursor.movePosition( QTextCursor::Left, QTextCursor::KeepAnchor );
		break;
	case Qt::Key_Down :
		cursor.movePosition( QTextCursor::EndOfBlock, QTextCursor::KeepAnchor );
		cursor.movePosition( QTextCursor::Right, QTextCursor::KeepAnchor );
		break;
	}
	int before = cursor.position();
	AdjustCursor( & cursor, QTextCursor::KeepAnchor );
	AdjustMove( & cursor, dir, QTextCursor::KeepAnchor );
	int right = 1;
	int anchor = cursor.anchor();
	if( cursor.position() != before ){
		if( dir == Qt::Key_Up && cursor.atBlockStart() ){ /* meet read only text */
			cursor.setPosition( cursor.position() );
			cursor.setPosition( anchor, QTextCursor::KeepAnchor );
			cursor.movePosition( QTextCursor::Right, QTextCursor::KeepAnchor );
			DaoCodeLineData *ud = (DaoCodeLineData*)cursor.block().userData();
			if( ud && ud->state == CLS_READONLY )
				cursor.movePosition( QTextCursor::Left, QTextCursor::KeepAnchor );
			right = 0;
		}else if( dir == Qt::Key_Down && cursor.atBlockEnd() ){ /* meet read only text */
			cursor.setPosition( cursor.position() );
			cursor.setPosition( anchor, QTextCursor::KeepAnchor );
			cursor.movePosition( QTextCursor::Left, QTextCursor::KeepAnchor );
			DaoCodeLineData *ud = (DaoCodeLineData*)cursor.block().userData();
			if( ud && ud->state == CLS_READONLY )
				cursor.movePosition( QTextCursor::Right, QTextCursor::KeepAnchor );
			right = 0;
		}
	}
	AdjustCursor( & cursor, QTextCursor::KeepAnchor );
	buffer = cursor.selectedText();
	buftype = BUF_BLOCKS;
	if( not copy ) cursor.removeSelectedText();
	if( not copy and dir == Qt::Key_Up && right ) cursor.movePosition( QTextCursor::Right );
	setTextCursor( cursor );
	ensureCursorVisible();
	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText(buffer);
}
int DaoTextEdit::FindRangeIndex( const QTextBlock & block, int pos )
{
	QTextLayout *layout = block.layout();
	if( layout == NULL ) return -1;
	QList<QTextLayout::FormatRange> ranges = layout->additionalFormats();
	QTextLayout::FormatRange range;
	int start = block.position();
	int i, n = ranges.size();
	int k = pos - start;
	for(i=0; i<n; i++){
		range = ranges[i];
		if( range.start <= k && (range.start + range.length) >= k ) return i;
	}
	return -1;
}
int DaoTextEdit::ForwardTokenEnd( int pos, int step )
{
	QTextBlock block = textCursor().block();
	QTextLayout *layout = block.layout();
	if( layout == NULL ) return -1;
	QList<QTextLayout::FormatRange> ranges = layout->additionalFormats();
	QTextLayout::FormatRange range, range2;
	int newpos = pos;
	while( ranges.size() ==0 ){ /* skip empty blocks */
		if( not block.next().isValid() ) return newpos;
		block = block.next();
		layout = block.layout();
		pos = newpos = block.position();
		if( layout == NULL ) return newpos;
		ranges = layout->additionalFormats();
	}
	int start = block.position();
	int i, n = ranges.size();
	int id = FindRangeIndex( block, pos );
	int k = pos - start;
	//printf( "id = %3i\n", id );
	if( id < 0 ) return newpos;
	range = ranges[id];
	if( k < range.start + range.length ){
		newpos = start + range.start + range.length;
		if( range.format.objectType() >= TXT_CHAR_OUTPUT ) step --;
	}
	int t, type = range.format.objectType();
	bool check_equal = step ==0;
	while( step or check_equal ){
		id ++;
		while( id >= n ){
			block = block.next();
			if( not block.isValid() ) break;
			ranges = block.layout()->additionalFormats();
			newpos = block.position();
			n = ranges.size();
			id = 0;
		}
		if( id >= n ) break;
		range2 = ranges[id];
		t = range2.format.objectType();
		if( check_equal and t >= TXT_CHAR_OUTPUT and t != type ) break;
		if( t >= TXT_CHAR_OUTPUT and t != type ) step --;
		start = block.position();
		range = range2;
		type = t;
		newpos = start + range.start + range.length;
		check_equal = step == 0;
	}
	return newpos;
}
int DaoTextEdit::BackwardTokenStart( int pos, int step )
{
	QTextBlock block = textCursor().block();
	QTextLayout *layout = block.layout();
	if( layout == NULL ) return -1;
	QList<QTextLayout::FormatRange> ranges = layout->additionalFormats();
	QTextLayout::FormatRange range, range2;
	int newpos = pos;
	while( ranges.size() ==0 ){ /* skip empty blocks */
		if( not block.previous().isValid() ) return newpos;
		block = block.previous();
		layout = block.layout();
		pos = newpos = block.position() + block.length() - 1;
		if( layout == NULL ) return newpos;
		ranges = layout->additionalFormats();
	}
	int start = block.position();
	int i, n = ranges.size();
	int id = FindRangeIndex( block, pos );
	int k = pos - start;
	//printf( "id = %3i\n", id );
	if( id < 0 ) return newpos;
	range = ranges[id];
	if( k > range.start ){
		newpos = start + range.start;
		if( range.format.objectType() >= TXT_CHAR_OUTPUT ) step --;
	}
	int t, type = range.format.objectType();
	bool check_equal = step ==0;
	while( step or check_equal ){
		id --;
		while( id < 0 ){
			block = block.previous();
			if( not block.isValid() ) break;
			start = block.position();
			ranges = block.layout()->additionalFormats();
			newpos = block.position() + block.length() - 1;
			n = ranges.size();
			id = n-1;
		}
		if( id < 0 ) break;
		range = ranges[id];
		t = range.format.objectType();
		if( check_equal and t >= TXT_CHAR_OUTPUT and t != type ) break;
		if( t >= TXT_CHAR_OUTPUT and t != type ) step --;
		start = block.position();
		type = t;
		newpos = start + range.start;
		check_equal = step == 0;
	}
	return newpos;
}
void DaoTextEdit::VimModeMoveByWord( int dir, int n, bool token )
{
	QTextCursor cursor = textCursor();
	QTextCharFormat format = cursor.charFormat();
	int type = format.objectType();
	int pos = cursor.position();
	int t, m = 0;
	if( token ){
		if( dir == Qt::Key_B ){
			pos = BackwardTokenStart( pos, n );
		}else{
			pos = ForwardTokenEnd( pos, n );
		}
		//printf( "position: %4i %4i\n", pos, cursor.position() );
		if( pos <0 ){
			token = 0;
		}else{
			cursor.setPosition( pos );
		}
	}
	if( token == false ){
		if( dir == Qt::Key_B ){
			cursor.movePosition( QTextCursor::PreviousWord, QTextCursor::MoveAnchor, n );
		}else{
			cursor.movePosition( QTextCursor::NextWord, QTextCursor::MoveAnchor, n );
		}
	}
	AdjustCursor( & cursor );
	AdjustMove( & cursor, dir );
	setTextCursor( cursor );
	ensureCursorVisible();
}
void DaoTextEdit::VimModeCopyDeleteByWord( int dir, int n, bool token, bool copy )
{
	QTextCursor cursor = textCursor();
	QTextCharFormat format = cursor.charFormat();
	int type = format.objectType();
	int pos = cursor.position();
	int t, m = 0;
	if( token ){
		if( dir == Qt::Key_B ){
			pos = BackwardTokenStart( pos, n );
		}else{
			pos = ForwardTokenEnd( pos, n );
		}
		//printf( "position: %4i %4i\n", pos, cursor.position() );
		if( pos <0 ){
			token = 0;
		}else{
			cursor.setPosition( pos, QTextCursor::KeepAnchor );
		}
	}
	if( token == false ){
		if( dir == Qt::Key_B ){
			cursor.movePosition( QTextCursor::PreviousWord, QTextCursor::KeepAnchor, n );
		}else{
			cursor.movePosition( QTextCursor::NextWord, QTextCursor::KeepAnchor, n );
		}
	}
	AdjustCursor( & cursor, QTextCursor::KeepAnchor );
	AdjustMove( & cursor, dir, QTextCursor::KeepAnchor );
	buffer = cursor.selectedText();
	buftype = BUF_WORDS;
	if( not copy ) cursor.removeSelectedText();
	setTextCursor( cursor );
	ensureCursorVisible();
	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText(buffer);
}
void DaoTextEdit::VimModeCommenting( int dir, int n, bool remove )
{
	QString open = "#{";
	QString close = "#}";
	QString cmt = "#";
	if( codehl.language and codehl.language->cmtLine1 )
		cmt = codehl.language->cmtLine1->mbs;
	if( codehl.language and codehl.language->cmtOpen1 ){
		open = codehl.language->cmtOpen1->mbs;
		close = codehl.language->cmtClose1->mbs;
	}

	QTextCursor cursor = textCursor();
	int start = cursor.block().blockNumber();
	switch( dir ){
	case Qt::Key_Up : 
		cursor.movePosition( QTextCursor::Up, QTextCursor::MoveAnchor, n-1 );
		break;
	case Qt::Key_Down :
		cursor.movePosition( QTextCursor::Down, QTextCursor::MoveAnchor, n-1 );
		break;
	default : break;
	}
	AdjustCursor( & cursor );
	AdjustMove( & cursor, dir );
	int i, end = cursor.block().blockNumber();
	if( start > end ){
		i = start;
		start = end;
		end = i;
	}
	for(i=start; i<=end; i++){
		QTextBlock block = document()->findBlockByNumber(i);
		QTextCursor csr = QTextCursor( block );
		DaoCodeLineData *ud = (DaoCodeLineData*) block.userData();
		if( ud->state == CLS_READONLY ) return;
		csr.movePosition( QTextCursor::Right, QTextCursor::MoveAnchor,
				ud->leadSpaces + ud->leadTabs );
		int pos = csr.position();
		if( remove and open.size() >0 ){
			csr.movePosition( QTextCursor::Right, QTextCursor::KeepAnchor, open.size() );
			if( csr.selectedText() == open ) continue;
			csr.setPosition( pos );
			csr.movePosition( QTextCursor::Right, QTextCursor::KeepAnchor, close.size() );
			if( csr.selectedText() == close ) continue;
			csr.setPosition( pos );
		}
		csr.movePosition( QTextCursor::Right, QTextCursor::KeepAnchor, cmt.size() );
		if( csr.selectedText() == cmt and remove ) csr.removeSelectedText();
		csr.setPosition( pos );
		if( not remove ) csr.insertText( cmt );
		IndentLine( csr );
	}
	switch( dir ){
	case Qt::Key_Up : cursor.movePosition( QTextCursor::Up ); break;
	case Qt::Key_Down : cursor.movePosition( QTextCursor::Down ); break;
	default : break;
	}
	AdjustCursor( & cursor );
	AdjustMove( & cursor, dir );
	setTextCursor( cursor );
}
bool DaoTextEdit::EditModeVIM( QKeyEvent * event )
{
	QTextCursor cursor;
	QTextBlock block;
	int mode = wgtEditMode ? wgtEditMode->currentIndex() : 0;
	int n = digits.toInt();
	int dir = 0;
	int key = event->key();
	QString syms = event->text();
	bool caps = syms[0].isUpper();
	bool eqv = false;
	labPopup->hide();
	if( n == 0 ) n = 1;
	if( mode != editMode ){
		editMode = mode;
		keyMode = (KeyMode) mode;
	}
	if( keyMode == KEY_MODE_VIM ){
		Qt::KeyboardModifier ctrl = Qt::ControlModifier;
#ifdef Q_WS_MAC
//		ctrl = Qt::MetaModifier;
#endif
		if( event->modifiers() & ctrl ){
			int newkey = 0;
			switch( key ){
			case Qt::Key_R : Redo(); break;
			case Qt::Key_F : newkey = Qt::Key_PageDown; break;
			case Qt::Key_B : newkey = Qt::Key_PageUp; break;
			default : return false;
			}
			if( newkey ){
				QKeyEvent newevent( event->type(), newkey, Qt::NoModifier );
				QPlainTextEdit::keyPressEvent( & newevent );
			}
			return true;
		}
		if( caps ){
			eqv = true;
			switch( key ){
			case Qt::Key_I : key = Qt::Key_Up; dir = -1; break;
			case Qt::Key_J : key = Qt::Key_Left; break;
			case Qt::Key_K : key = Qt::Key_Down; dir = 1; break;
			case Qt::Key_L : key = Qt::Key_Right; break;
			default : eqv = false; break;
			}
			if( eqv ){
				if( keys.size() ==0 ){
					VimModeMove( key, n );
				}else if( keys == "d" ){
					VimModeCopyDelete( key, n );
				}else if( keys == "y" ){
					VimModeCopyDelete( key, n, true );
				}else if( keys == "c" ){
					VimModeCommenting( key, n, false );
				}else if( keys == "C" ){
					VimModeCommenting( key, n, true );
				}else if( keys == "=" ){
					IndentLines( textCursor(), dir * n );
				}
				keys.clear();
				digits.clear();
				return true;
			}
		}
		switch( key ){
		case Qt::Key_Escape :
			cmdPrompt->hide();
			setFocus();
			break;
		case Qt::Key_Return :
		case Qt::Key_CapsLock :
		case Qt::Key_Backspace :
			break;
		case Qt::Key_Slash :
			cmdPrompt->StartSearch();
			break;
		case Qt::Key_Colon :
			cmdPrompt->StartCommand();
			if( digits.size() ){
				n = digits.toInt();
				cmdPrompt->SetText( ".,+" + QString::number( n-1 ) );
				digits.clear();
			}
			break;
		case Qt::Key_I :
			ShowMessage( tr("Switch to " ) + "<font color=\"red\">"
					+ tr("Normal Edit") + "</font>" + tr( " mode" ) );
			keyMode = KEY_MODE_EDIT; break;
		case Qt::Key_O :
			cursor = textCursor();
			if( caps ){
				cursor.movePosition( QTextCursor::StartOfBlock );
				cursor.insertBlock( );
				cursor.movePosition( QTextCursor::PreviousBlock );
			}else{
				cursor.movePosition( QTextCursor::EndOfBlock );
				cursor.insertBlock( );
			}
			setTextCursor( cursor );
			IndentLine( cursor );
			ShowMessage( tr("Switch to " ) + "<font color=\"red\">"
					+ tr("Normal Edit") + "</font>" + tr( " mode" ) );
			keyMode = KEY_MODE_EDIT; break;
		case Qt::Key_Space:
			if( event->modifiers() & Qt::AltModifier ) return false;
			break;
		case Qt::Key_U :
			Undo();
			break;
		case Qt::Key_Up :   case Qt::Key_Down :
		case Qt::Key_Left : case Qt::Key_Right :
			if( digits.size() ==0 && keys.size() ==0 ) return false;
			if( keys.size() ==0 ){
				VimModeMove( key, n );
			}else if( keys == "d" ){
				VimModeCopyDelete( key, n );
			}else if( keys == "y" ){
				VimModeCopyDelete( key, n, true );
			}else if( keys == "c" ){
				VimModeCommenting( key, n, false );
			}else if( keys == "C" ){
				VimModeCommenting( key, n, true );
			}else if( keys == "=" ){
				dir = 0;
				if( key == Qt::Key_Up ) dir = -1;
				if( key == Qt::Key_Down ) dir = 1;
				IndentLines( textCursor(), dir * n );
			}
			keys.clear();
			digits.clear();
			break;
		case Qt::Key_G : case Qt::Key_H :
			n *= caps ? 100 : 10;
			if( key == Qt::Key_G ){
				key = Qt::Key_Down;
			}else{
				key = Qt::Key_Up;
			}
			if( keys.size() ==0 ){
				VimModeMove( key, n );
			}else if( keys == "d" ){
				VimModeCopyDelete( key, n );
			}else if( keys == "y" ){
				VimModeCopyDelete( key, n, true );
			}else if( keys == "c" ){
				VimModeCommenting( key, n, false );
			}else if( keys == "C" ){
				VimModeCommenting( key, n, true );
			}
			keys.clear();
			digits.clear();
			break;
		case Qt::Key_C :
			if( keys == syms and event->modifiers() ==0 ){
				VimModeCommenting( Qt::Key_Down, n, caps );
				keys.clear();
				digits.clear();
				break;
			}else{
				keys = "";
			}
			keys += syms;
			break;
		case Qt::Key_D :
			if( keys == "d" ){
				if( textCursor().block().next().isValid() )
					VimModeCopyDelete( Qt::Key_Down, n );
				else
					VimModeCopyDelete( Qt::Key_Up, n );
				keys.clear();
				digits.clear();
				break;
			}
			keys += "d";
			break;
		case Qt::Key_Y :
			if( keys == "y" ){
				if( textCursor().block().next().isValid() )
					VimModeCopyDelete( Qt::Key_Down, n, true );
				else
					VimModeCopyDelete( Qt::Key_Up, n, true );
				keys.clear();
				digits.clear();
				break;
			}
			keys += "y";
			break;
		case Qt::Key_B :
		case Qt::Key_W :
		case Qt::Key_F :
		case Qt::Key_J :
			if( caps == false and key == Qt::Key_F ){
				key = Qt::Key_W;
				caps = true;
			}else if( key == Qt::Key_J ){
				key = Qt::Key_B;
				caps = true;
			}
			if( key == Qt::Key_F ){
				if( keys == "d" ){
					VimModeCopyDelete( Qt::Key_Right, n );
				}else if( keys == "y" ){
					VimModeCopyDelete( Qt::Key_Right, n, true );
				}else{
					VimModeMove( Qt::Key_Right, n );
				}
			}else if( keys == "d" ){
				VimModeCopyDeleteByWord( key, n, caps );
			}else if( keys == "y" ){
				VimModeCopyDeleteByWord( key, n, caps, true );
			}else{
				VimModeMoveByWord( key, n, caps );
			}
			keys.clear();
			digits.clear();
			break;
		case Qt::Key_Equal :
			if( keys == "=" ){
				IndentLines( textCursor(), n );
				keys.clear();
				digits.clear();
				break;
			}
			keys += "=";
			break;
		case Qt::Key_K :
			JoinNextLine( textCursor() );
			keys.clear();
			digits.clear();
			break;
		case Qt::Key_0 : case Qt::Key_1 : case Qt::Key_2 : 
		case Qt::Key_3 : case Qt::Key_4 : case Qt::Key_5 : 
		case Qt::Key_6 : case Qt::Key_7 : case Qt::Key_8 : case Qt::Key_9 :
			digits += event->text();
			break;
		case Qt::Key_P :
			if( buftype == BUF_WORDS ){
				insertPlainText( buffer );
			}else if( buftype == BUF_BLOCKS ){
				QString txt = buffer;
				QChar newline = QChar( QChar::ParagraphSeparator );
				if( txt.size() and txt[0] == newline ) txt = txt.mid(1);
				if( event->text() == "P" ){
					moveCursor( QTextCursor::StartOfBlock );
					if( txt.size() and txt[txt.size()-1] != newline ) txt += '\n';
				}else{
					if( not textCursor().block().next().isValid() ){
						txt = '\n' + txt;
						moveCursor( QTextCursor::EndOfBlock );
					}else if( txt.size() and txt[txt.size()-1] != newline ){
						txt += '\n';
						moveCursor( QTextCursor::NextBlock );
					}else{
						moveCursor( QTextCursor::NextBlock );
					}
				}
				QTextCursor cursor = textCursor();
				int pos = cursor.position();
				insertPlainText( txt );
				if( event->text() == "P" ){
					cursor.setPosition( pos );
					setTextCursor( cursor );
				}else{
					moveCursor( QTextCursor::PreviousBlock );
				}
			}else{
				ShowMessage( tr("Buffer is empty") );
			}
			break;
		default :
			if( event->modifiers() ==0 ){
				keys.clear();
				digits.clear();
				ShowMessage( tr("Invalid DS-VIM command key") );
			}
			break;
		}
		return true;
	}
	if( keyMode == KEY_MODE_EDIT && mode ){
		if( key == Qt::Key_Escape ){
			keyMode = (KeyMode) mode;
			QString name = wgtEditMode->currentText() + tr(" Command");
			ShowMessage( tr("Switch to " ) + "<font color=\"red\">"
					+ name + "</font>" + tr( " mode" ) );
			return true;
		}
	}
	return false;
}
void DaoTextEdit::JoinNextLine( QTextCursor cursor )
{
	if( cursor.block().blockNumber() + 1 >= blockCount() ) return;
	QTextBlock block = cursor.block().next();
	DaoCodeLineData *ud = (DaoCodeLineData*) block.userData();
	if( ud->state == CLS_READONLY ) return;
	cursor.movePosition( QTextCursor::EndOfBlock );
	cursor.movePosition( QTextCursor::PreviousWord );
	cursor.movePosition( QTextCursor::EndOfWord );
	cursor.movePosition( QTextCursor::EndOfBlock, QTextCursor::KeepAnchor );
	cursor.movePosition( QTextCursor::NextBlock, QTextCursor::KeepAnchor );
	while( ud->firstToken == 0 and not cursor.atEnd() ){
		cursor.movePosition( QTextCursor::NextBlock, QTextCursor::KeepAnchor );
		ud = (DaoCodeLineData*) cursor.block().userData();
	}
	cursor.movePosition( QTextCursor::Right, QTextCursor::KeepAnchor,
			ud->leadSpaces + ud->leadTabs );
	cursor.insertText( " " );
	setTextCursor( cursor );
}
DaoCodeLineData* DaoTextEdit::SetIndentData( QTextBlock block )
{
	if( not block.isValid() ) return NULL;
	DaoCodeLineData *ud = (DaoCodeLineData*) block.userData();
	if( ud ) return ud;
	ud = new DaoCodeLineData(false, CLS_NORMAL, block.blockNumber()+1 );
	block.setUserData( ud );
	return ud;
}
static QString DaoToken_MakeShortCodes( DArray *tokens )
{
	QString codes;
	int i;
	for(i=0; i<tokens->size; i++){
		DaoToken *token = tokens->items.pToken[i];
		switch( token->type ){
		case DTOK_NONE :
		case DTOK_CMT_OPEN :
		case DTOK_COMMENT :
			break;
		case DTOK_MBS_OPEN :
		case DTOK_WCS_OPEN :
			codes += "'";
			break;
		case DTOK_DIGITS_DEC :
		case DTOK_NUMBER_HEX :
		case DTOK_NUMBER_DEC :
		case DTOK_DOUBLE_DEC :
		case DTOK_NUMBER_SCI :
			codes += '0';
			break;
		case DTOK_MBS :
		case DTOK_WCS :
			codes += "''";
			break;
		case DTOK_NEWLN :
			codes += ' ';
			break;
		default:
			codes += token->string.mbs;
			break;
		}
	}
	return codes;
}
void DaoTextEdit::IndentLine( QTextCursor cursor, bool indent )
{
#if 0
	normal indent;
	normal indent;
	if( incomplete start
		incomplete
		condition )
		if( condition )
			statement;
	for( ; ; )
		for( condition ) statement;
	normal indentation;
#endif

	QTextBlock bk, current_block = cursor.block();
	DaoCodeLineData *ud, *current_data = (DaoCodeLineData*) current_block.userData();
	if( current_data == NULL ) current_data = SetIndentData( current_block );
	if( current_data == NULL ) return;

	QTextBlock previous_block;
	QTextBlock incomplete_start_block;
	QTextBlock previous_start_block;
	QString joint_lines = current_block.text();
	QString previous_lines;
	QString reference_line;
	DaoCodeLineData *previous_data = NULL;
	if( current_block.previous().isValid() ){
		bk = current_block.previous();
		if( not bk.previous().isValid() ) IndentLine( QTextCursor( bk ), false );
		ud = (DaoCodeLineData*) bk.userData();
		previous_block = bk;
		previous_data = ud;
		if( ud && ud->incomplete ){
			incomplete_start_block = bk;
			joint_lines = bk.text() + '\n' + joint_lines;
			while( bk.previous().isValid() ){
				bk = bk.previous();
				ud = (DaoCodeLineData*) bk.userData();
				if( ud && ud->token_line == true ) continue;
				if( ud == NULL or ud->incomplete == false ) break;
				incomplete_start_block = bk;
				joint_lines = bk.text() + '\n' + joint_lines;
			}
		}
		previous_lines = previous_block.text();
		previous_start_block = previous_block;
		ud = (DaoCodeLineData*) previous_start_block.userData();
		while( ud && ud->extend_line ){
			previous_start_block = previous_start_block.previous();
			previous_lines = previous_start_block.text() + '\n' + previous_lines;
			ud = (DaoCodeLineData*) previous_start_block.userData();
		}
		bk = previous_block;
		ud = (DaoCodeLineData*) bk.userData();
		while( ud && ud->reference_line == false ){
			bk = bk.previous();
			ud = (DaoCodeLineData*) bk.userData();
		}
		if( ud && ud->reference_line == true ) reference_line = bk.text();
	}
	DaoBasicSyntax *language = codehl.language;
	if( language == NULL ) language = DaoBasicSyntax::dao;
	language->Tokenize( lexer, reference_line.toUtf8().data() );

	DArray *tokens = lexer->tokens;
	QString reference_indent;
	bool begin = true;
	int i;
	for(i=0; i<tokens->size; i++){
		DaoToken *token = tokens->items.pToken[i];
		switch( token->type ){
		case DTOK_BLANK : 
		case DTOK_TAB:
			if( begin ) reference_indent += token->string.mbs;
			break;
		default :
			begin = false;
			break;
		}
	}

	language->Tokenize( lexer, previous_lines.toUtf8().data() );
	QString previous_indent;
	begin = true;
	for(i=0; i<lexer->tokens->size; i++){
		DaoToken *token = lexer->tokens->items.pToken[i];
		switch( token->type ){
		case DTOK_BLANK : 
		case DTOK_TAB:
			if( begin ) previous_indent += token->string.mbs;
			break;
		default :
			begin = false;
			break;
		}
	}

	int b=0, cb=0, sb = 0;
	int open_brace =  0;
	for(i=0; i<tokens->size; i++){
		DaoToken *token = tokens->items.pToken[i];
		switch( token->type ){
		case DTOK_LCB : cb += 1; if( cb > open_brace ) open_brace = cb; break;
		case DTOK_RCB : cb -= 1; if( cb <= 0 ) open_brace = cb = 0; break;
		}
	}
	QString previous_codes = DaoToken_MakeShortCodes( tokens );

	QString old_indent;
	int close_brace = 0;
	language->Tokenize( lexer, current_block.text().toUtf8().data() );
	cb = 0;
	begin = true;
	for(i=0; i<tokens->size; i++){
		DaoToken *token = tokens->items.pToken[i];
		switch( token->type ){
		case DTOK_BLANK : 
		case DTOK_TAB:
			if( begin ) old_indent += token->string.mbs;
			break;
		case DTOK_LCB :
			begin = false;
			cb += 1; 
			break;
		case DTOK_RCB :
			begin = false;
			cb -= 1;
			if( cb < close_brace ) close_brace = cb;
			break;
		default :
			begin = false;
			break;
		}
	}
	QString current_codes = DaoToken_MakeShortCodes( tokens );
	QString new_indent = old_indent;

#ifdef DEBUG
	printf( "=======================\n%s\n\n%s\n\n", 
			joint_lines.toUtf8().data(), previous_codes.toUtf8().data() );
	if( previous_data ) printf( "%p %i %i\n", previous_data, previous_data->open_token, previous_data->incomplete );
	if( previous_data && previous_data->open_token ){
		printf( "manual indentation\n" );
	}else if( incomplete_start_block.isValid() ){
		printf( "incomplete line indentation\n" );
	}else{
		printf( "normal indentation:  %i %i\n", open_brace, close_brace );
	}
#endif

	current_data->reference_line = true;
	if( previous_data && previous_data->open_token ){
		// manual indentation; namely do nothing here:
		current_data->token_line = true;
		current_data->extend_line = true;
		current_data->reference_line = false;
	}else if( incomplete_start_block.isValid() ){
		// indent incomplete line:
		current_data->extend_line = true;
		current_data->reference_line = false;
		new_indent = previous_indent + '\t';
	}else{
		// indent normally:
		new_indent = reference_indent;
		DString_SetMBS( mbs, previous_codes.toUtf8().data() );
		if( language->IndentMore( previous_codes ) ){
			new_indent = previous_indent;
			open_brace += 1;
			current_data->reference_line = false;
		}else if( language->IndentMore2( previous_codes ) ){
			new_indent = previous_indent;
			open_brace += 1;
		}
		DString_SetMBS( mbs, current_codes.toUtf8().data() );
		if( language->IndentNone( current_codes ) ){
			new_indent = "";
			open_brace = close_brace = 0;
			current_data->reference_line = false;
		}else if( language->IndentLess( current_codes ) ){
			close_brace -= 1;
		}
		while( (open_brace--) > 0 ) new_indent += '\t';
		while( (close_brace++) < 0 and new_indent.size() ){
			if( new_indent[new_indent.size()-1] == '\t' ){
				new_indent.chop(1);
			}else{
				int tab = 4;
				while( (tab--) > 0 and new_indent.size() and new_indent[new_indent.size()-1] == ' ' )
					new_indent.chop(1);
			}
		}
	}
#ifdef DEBUG
	printf( "prev:%s;\n", previous_indent.toUtf8().data() );
	printf( "old:%s;\n", old_indent.toUtf8().data() );
	printf( "new:%s;\n", new_indent.toUtf8().data() );
#endif
	if( new_indent != old_indent ){
		cursor = textCursor();
		cursor.movePosition( QTextCursor::StartOfBlock );
		cursor.movePosition( QTextCursor::Right, QTextCursor::KeepAnchor, old_indent.size() );
		//printf( "%i===%s===%s===\n", id, ws.toUtf8().data(), leading.toUtf8().data() );
		cursor.insertText( new_indent );
	}

	language->Tokenize( lexer, joint_lines.toUtf8().data() );
	b = cb = sb = 0;
	for(i=0; i<tokens->size; i++){
		DaoToken *token = tokens->items.pToken[i];
		//printf( "%3i:  %3i  %s\n", i, token->type, token->string.mbs );
		switch( token->type ){
		case DTOK_LB  : b  += 1; break;
		case DTOK_RB  : b  -= 1; break;
		case DTOK_LCB : cb += 1; break;
		case DTOK_RCB : cb -= 1; break;
		case DTOK_LSB : sb += 1; break;
		case DTOK_RSB : sb -= 1; break;
		}
	}
	//printf( "%3i %3i %3i\n", b, cb, sb );
	current_data->incomplete = b > 0 or sb > 0;
	current_data->brace_count = cb;
	current_data->open_token = false;

	if( tokens->size ==0 ) return;
	int toktype = tokens->items.pToken[tokens->size-1]->type;
	if( toktype >= DTOK_CMT_OPEN && toktype <= DTOK_WCS_OPEN ){
		current_data->open_token = true;
		current_data->incomplete = true;
	}else{
		current_data->token_line = false;
	}
#ifdef DEBUG
	printf( "current line:::::::\n%s\n", current_block.text().toUtf8().data() );
	printf( "reference line: %i\n", current_data->reference_line );
	printf( "incoumplete: %i\n", current_data->incomplete );
	printf( "extended line: %i\n", current_data->extend_line );
	printf( "open token: %i\n", current_data->open_token );
	printf( "token line: %i\n", current_data->token_line );
#endif
}
void DaoTextEdit::IndentLines( QTextCursor cursor, int n )
{
	if( n <0 ){
		n = -n;
		cursor.movePosition( QTextCursor::PreviousBlock,
				QTextCursor::MoveAnchor, n-1 );
	}
	IndentLine( cursor );
	while( (--n) >0 ){
		cursor.movePosition( QTextCursor::NextBlock );
		setTextCursor( cursor );
		IndentLine( cursor );
		if( cursor.block().blockNumber() + 1 >= blockCount() ) break;
	}
}
void DaoTextEdit::keyPressEvent ( QKeyEvent * event )
{
	int key = event->key();
	int mod = event->modifiers();
	QString leadingSpace;
	QChar lastChar;
	if( key == Qt::Key_Return and cmdPrompt->isVisible() ){
		return;
	}
	if( EditModeVIM( event ) ) return;
	if( wgtEditMode && (mod & Qt::AltModifier) && key == Qt::Key_Space ){
		int id = wgtEditMode->currentIndex();
		int nn = wgtEditMode->count();
		QString name;
		if( mod & Qt::ShiftModifier ){
			id = (id + 1) % nn;
			keyMode = (KeyMode)(keyMode + 1);
			wgtEditMode->setCurrentIndex( id );
			name = wgtEditMode->currentText();
			if( id ) name += tr(" Command"); else name += tr(" Edit");
		}else{
			if( keyMode ){
				keyMode = KEY_MODE_EDIT;
				name = tr( "Normal Edit" );
			}else if( id ){
				keyMode = (KeyMode) id;
				name = wgtEditMode->currentText() + tr(" Command");
			}else{
				return;
			}
		}
		ShowMessage( tr("Switch to " ) + "<font color=\"red\">"
				+ name + "</font>" + tr( " mode" ) );
		return;
	}else if( (mod & Qt::AltModifier) && (key == Qt::Key_Up || key == Qt::Key_Down) ){
		if( this == studio->Console() ){
			studio->slotMaxEditor();
		}else{
			studio->slotMaxConsole();
		}
		return;
	}
	/* Ctrl + K, delete text in the line after the cursor position.
	 * disable some default hotkey when the word completion widget is visiable: */
	if( ! wgtWords->isVisible() || ! (mod & Qt::ControlModifier) ){
		QPlainTextEdit::keyPressEvent( event );
	}
	if( key == Qt::Key_Return ){
		QTextCursor cursor = textCursor();
		cursor.movePosition( QTextCursor::Up );
		setTextCursor( cursor );
		IndentLine( cursor, codehl.language != DaoBasicSyntax::python );
		cursor = textCursor();
		cursor.movePosition( QTextCursor::Down );
		setTextCursor( cursor );
		IndentLine( cursor );
	}
	if( wgtWords && not labPopup->isVisible() )
		wgtWords->TryCompleteWord( event, LeftMargin() );
}
void DaoTextEdit::contextMenuEvent(QContextMenuEvent *event)
{
	QMenu *menu = createStandardContextMenu();
	QString style = "color: black; background-color: rgb(250, 250, 250, 220);"
		"selection-color: rgb(128, 128, 128, 250);"
		"selection-background-color: rgb(128, 128, 255, 250);"
		;
	int i;
	menuPosition = event->pos();
	for(i=0; i<extraMenuActions.size(); i++) menu->addAction( extraMenuActions[i] );
	menu->setStyleSheet( style );
	menu->setBackgroundRole( QPalette::NoRole );
	menu->exec(event->globalPos());
	delete menu;
}

#define MARK_WIDTH 20

DaoEditor::DaoEditor( DaoTabEditor *parent, DaoWordList *wlist )
: DaoTextEdit( parent, wlist )
{
	tabWidget = parent;
	state = false;
	ready = false;
	outlineChanged = true;
	setViewportMargins( 20, 0, MARK_WIDTH, 0 );
	wgtLangLabels = new DaoLangLabels( this );
	wgtLangLabels->setMouseTracking( true );
	wgtNumbering = new DaoNumbering( this );
	wgtNumbering->setToolTip( tr("Right click to:") + "\n"
			+ "(1) " + tr("set or unset break points;") + "\n"
			+ "(2) " + tr("change execution point.") );

	codeThumb = new DaoCodeThumb( this );
	codeThumb->hide();

	connect( & codehl, SIGNAL(signalOutlineChanged()), this, SLOT(slotOutlineChanged()));
	connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(UpdateNumberingWidth(int)));
	connect(this, SIGNAL(updateRequest(const QRect &, int)), this, SLOT(UpdateNumbering(const QRect &, int)));
	connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(HighlightCurrentLine()));
	connect( this, SIGNAL(cursorPositionChanged()), this, SLOT(slotBoundEditor()) );

	UpdateNumberingWidth(0);
	connect( this, SIGNAL(textChanged()), this, SLOT(slotTextChanged()) );
	connect( & watcher, SIGNAL(fileChanged(const QString &)),
			this, SLOT(slotFileChanged(const QString &)) );

	connect( document(), SIGNAL(contentsChange(int,int,int)),
			this, SLOT(slotContentsChange(int,int,int)) );
}
QString DaoEditor::GuessFileType( const QString & source )
{
	QRegExp makefile( "(^\\w+\\s*:|^\\s*\\w+\\s*=|\\$\\(\\w+\\))" );
	int npatt = source.count( makefile );
	int nline = source.count( '\n' );
	//printf( "%3i  %3i\n", npatt, nline );
	if( npatt >= 0.2*nline ) return "makefile";
	return "";
}
void DaoEditor::LoadFile( const QString & name )
{
	QFileInfo info( name );
	QFile file( name );
	if( name != fullName ){
		if( watcher.files().contains( fullName ) ) watcher.removePath( fullName );
		watcher.addPath( name );
		fullName = name;
	}
	if( file.open( QFile::ReadOnly )) {
		QTextStream fin( & file );
		this->name = info.fileName();
		disconnect( this, SIGNAL(textChanged()), this, SLOT(slotTextChanged()) );
		QString ftype = info.suffix();
		codehl.SetLanguage( ftype );
		codeThumb->codehl.SetLanguage( ftype );
		textOnDisk = fin.readAll();
		if( ftype == "" ){
			ftype = GuessFileType( textOnDisk );
			//printf( "%s\n", ftype.toUtf8().data() );
			codehl.SetLanguage( ftype );
			codeThumb->codehl.SetLanguage( ftype );
		}
		setPlainText( textOnDisk );
		UpdateOutline();
		emit signalTextChanged( false );
		connect( this, SIGNAL(textChanged()), this, SLOT(slotTextChanged()) );
	}
	ResetBlockState();
	wordList->AddWords( codehl.GetWords() );
}
void DaoEditor::slotFileChanged( const QString & fname )
{
	QFile file( fname );
	QString text;
	if( file.open( QFile::ReadOnly )) {
		QTextStream fin( & file );
		text = fin.readAll();
	}
	if( text != textOnDisk ){
		tabWidget->setCurrentWidget( this );
		int ret = QMessageBox::warning(this, tr("DaoStudio"),
				tr("The copy of the file on harddisk has been modified .\n"
					"Do you want to reload the file?"),
				QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
		if( ret == QMessageBox::Yes ) LoadFile( fname );
	}

}
void DaoEditor::Save( const QString & fname )
{
	if( blockEntry.isValid() ){
		ShowMessage( tr("Saving is disable for the file under debugging!")
				+ "\n" + tr("Press the debugging button to resume the execution!") );
		return;
	}
	QString ftype;
	if( fname.size() ){
		QFileInfo info( fname );
		if( fname != fullName ){
			ftype = info.suffix();
			if( fname != fullName ){
				if( watcher.files().contains( fullName ) ) watcher.removePath( fullName );
				watcher.addPath( fname );
			}
		}
		fullName = fname;
		name = info.fileName();
	}
	QFile file( fullName );
	if( file.open( QFile::WriteOnly )) {
		QTextStream fout( & file );
		textOnDisk = toPlainText();
		fout<<textOnDisk;
	}
	if( ftype.size() ){
		codehl.SetLanguage( ftype );
		codeThumb->codehl.SetLanguage( ftype );
		RedoHighlight();
	}
	state = false;
	ResetBlockState();
	wordList->AddWords( codehl.GetWords() );
	emit signalTextChanged( false );
}
void DaoEditor::focusInEvent ( QFocusEvent * event )
{
	//emit signalFocusIn();
	DaoTextEdit::focusInEvent( event );
}
void DaoEditor::mousePressEvent ( QMouseEvent * event )
{
	QPlainTextEdit::mousePressEvent( event );
	emit signalFocusIn();
}
void DaoEditor::resizeEvent ( QResizeEvent * event )
{
	QPlainTextEdit::resizeEvent( event );
	QRect cr = contentsRect();
	wgtNumbering->setGeometry(QRect(cr.left(), cr.top(), NumberingWidth(), cr.height()));
	cr = viewport()->geometry();
	wgtLangLabels->setGeometry(QRect(cr.right(), cr.top(), MARK_WIDTH, cr.height()));
	wgtLangLabels->update();
	int twidth = cr.width() / 2.5;
	codeThumb->setGeometry(QRect(cr.right()-twidth, cr.top(), twidth, cr.height()));
}
void DaoEditor::keyPressEvent ( QKeyEvent * event )
{
	int key = event->key();
	int mod = event->modifiers();
	if( mod & Qt::AltModifier ){
		int i = tabWidget->currentIndex();
		int n = tabWidget->count();
		if( key == Qt::Key_Up || key == Qt::Key_Down ){
			studio->slotMaxConsole();
			return;
		}else if( key == Qt::Key_Left ){
			tabWidget->setCurrentIndex( (i+n-1)%n );
			return;
		}else if( key == Qt::Key_Right ){
			tabWidget->setCurrentIndex( (i+1)%n );
			return;
		}
	}
	QTextCursor cursor = textCursor();
	slotBoundEditor();
	DaoTextEdit::keyPressEvent( event );
	if( outlineChanged ) UpdateOutline();
	if( isReadOnly() ){
		wgtWords->SetTip( tr("Read only area") );
		wgtWords->EnsureVisible( LeftMargin(), true );
		setTextCursor( cursor );
	}else if( wgtWords->IsTip() ){
		wgtWords->hide();
	}
	cursor = textCursor();
	QTextBlock block = document()->findBlock( cursor.position() );
	if( blockEntry.isValid() && block == blockEntry ) repaint();
}
void DaoEditor::ChangeMark( int y )
{
}
void DaoEditor::MarkLine( int y )
{
	QTextBlock block = firstVisibleBlock();
	int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
	int bottom = top + (int) blockBoundingRect(block).height();
	while (block.isValid() && top <= y ) {
		if (block.isVisible() && bottom >= y ) break;
		block = block.next();
		top = bottom;
		bottom = top + (int) blockBoundingRect(block).height();
	}
	int blockNumber = block.blockNumber();
	DaoCodeLineData *ud = (DaoCodeLineData*) block.userData();
	QString it = ud->breaking ? tr("Unset break point") : tr("Set break point");
	QMenu menu;
	QAction *breakAct = menu.addAction( it );
	if( blockEntry.isValid() && ud->state <= CLS_CHANGED )
		menu.addAction( tr("Move exection point to here") );
	QAction *act = menu.exec( QCursor::pos() );
	if( act == breakAct ){
		ud->breaking = not ud->breaking;
		QMap<int,int>::iterator it = breakPoints.find( blockNumber + 1 );
		if( it == breakPoints.end() ){
			breakPoints[ blockNumber + 1 ] = 1;
		}else{
			breakPoints.erase( it );
		}
		QLocalSocket socket;
		socket.connectToServer( DaoStudioSettings::socket_breakpoints );
		if( socket.waitForConnected( 5000 ) ){
			QByteArray data;
			data.append( fullName );
			data += '\0' + QByteArray::number( ud->breaking );
			data += '\0' + QByteArray::number( blockNumber + 1 );
			socket.write( data );
			socket.flush();
			socket.disconnectFromServer();
		}
	}else if( act ){
		blockEntry = block;
	}
}
void DaoEditor::SetFontSize( int size )
{
	disconnect( this, SIGNAL(textChanged()), this, SLOT(slotTextChanged()) );
	DaoTextEdit::SetFontSize( size );
	connect( this, SIGNAL(textChanged()), this, SLOT(slotTextChanged()) );
}
void DaoEditor::SetFontFamily( const QString & family )
{
	disconnect( this, SIGNAL(textChanged()), this, SLOT(slotTextChanged()) );
	DaoTextEdit::SetFontFamily( family );
	connect( this, SIGNAL(textChanged()), this, SLOT(slotTextChanged()) );
}
void DaoEditor::BeforeRedoHighlight()
{
	disconnect( document(), SIGNAL(contentsChange(int,int,int)),
			this, SLOT(slotContentsChange(int,int,int)) );
	disconnect( this, SIGNAL(textChanged()), this, SLOT(slotTextChanged()) );
}
void DaoEditor::AfterRedoHighlight()
{
	connect( this, SIGNAL(textChanged()), this, SLOT(slotTextChanged()) );
	connect( document(), SIGNAL(contentsChange(int,int,int)),
			this, SLOT(slotContentsChange(int,int,int)) );
	UpdateOutline();
}
void DaoEditor::SetColorScheme( int scheme )
{
	DaoTextEdit::SetColorScheme( scheme );
	HighlightCurrentLine();
	if( scheme <= 1 ){
		wgtNumbering->setStyleSheet( "color: black; background-color: #FFA" );
	}else{
		wgtNumbering->setStyleSheet( "color: white; background-color: #220" );
	}
}
int DaoEditor::NumberingWidth()
{
	int max = qMax(1, blockCount());
	int digits = QString::number( max ).size();
	QFontMetrics fm( DaoStudioSettings::codeFont );
	int space = 20 + fm.width(QLatin1Char('9')) * digits;
	return space;
}
void DaoEditor::UpdateNumberingWidth(int)
{
	setViewportMargins(NumberingWidth(), 0, MARK_WIDTH, 0);
}
void DaoEditor::UpdateNumbering(const QRect &rect, int dy)
{
	if (dy){
		wgtNumbering->scroll(0, dy);
		wgtLangLabels->scroll(0, dy);
	}else{
		wgtNumbering->update(0, rect.y(), wgtNumbering->width(), rect.height());
		wgtLangLabels->update(0, rect.y(), wgtLangLabels->width(), rect.height());
	}

	if (rect.contains(viewport()->rect()))
		UpdateNumberingWidth(0);
	wgtLangLabels->update();
}
void DaoEditor::HighlightCurrentLine()
{
	return;
	QList<QTextEdit::ExtraSelection> extraSelections;
	if (!isReadOnly()) {
		QTextEdit::ExtraSelection selection;
		selection.format.setBackground(lineColor);
		selection.format.setProperty(QTextFormat::FullWidthSelection, true);
		selection.cursor = textCursor();
		selection.cursor.clearSelection();
		extraSelections.append(selection);
	}
	setExtraSelections(extraSelections);
}
void DaoEditor::scrollContentsBy( int dx, int dy )
{
	DaoTextEdit::scrollContentsBy( dx, dy );
	wgtNumbering->scroll( dx, dy );
	wgtLangLabels->scroll( dx, dy );
	wgtWords->hide();
	update();
	wgtLangLabels->update();
}
void DaoEditor::PaintNumbering ( QPaintEvent * event )
{
	QFont font = codehl.GetFont();
	QPainter painter(wgtNumbering);
	painter.fillRect(event->rect(), Qt::lightGray);
	painter.setPen(Qt::darkGray);
	painter.setFont( font );
	QTextBlock block = firstVisibleBlock();
	int blockNumber = block.blockNumber();
	int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
	int bottom = top + (int) blockBoundingRect(block).height();
	int w = wgtNumbering->width() - 8;
	while (block.isValid() && top <= event->rect().bottom()) {
		if (block.isVisible() && bottom >= event->rect().top()) {
			QString number = QString::number(blockNumber + 1);
			DaoCodeLineData *ud = (DaoCodeLineData*) block.userData();
			QFont font2 = font;
			if( ud && ud->font_size ) font2.setPointSize( ud->font_size );
			QFontMetrics fm( font2 );
			painter.setFont( font2 );
			if( block == blockEntry )
				painter.fillRect( QRect(2, top, w+1, fm.height()+2), Qt::green);
			if( ud && ud->breaking ){
				painter.setPen(Qt::red);
				painter.drawText(0, top, w, fm.height(), Qt::AlignRight, number);
				painter.setPen(Qt::darkGray);
			}else{
				painter.drawText(0, top, w, fm.height(), Qt::AlignRight, number);
			}
			if( ud ){
				if( ud->state == CLS_READONLY )
					painter.fillRect( QRect(w+2, top-1, w+8, fm.height()+6), Qt::red);
				else if( ud->state == CLS_CHANGED )
					painter.fillRect( QRect(w+2, top-1, w+8, fm.height()+6), Qt::blue);
			}
		}
		block = block.next();
		top = bottom;
		bottom = top + (int) blockBoundingRect(block).height();
		++blockNumber;
	}
}
void DaoEditor::PaintQuickScroll ( QPaintEvent * event )
{
	QFontMetrics fm( codehl.GetFont() );
	QPainter painter(wgtLangLabels);
	painter.fillRect(event->rect(), Qt::lightGray);
	painter.fillRect( 0, 0, wgtLangLabels->width(), wgtLangLabels->height(), Qt::lightGray);
	painter.setPen(Qt::black);
	QTextBlock block = firstVisibleBlock();
	int first = block.blockNumber();
	int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
	int bottom = top + (int) blockBoundingRect(block).height();
	int last = first;
	int viewheight = viewport()->height();
	int height = 0;
	int B = blockCount();
	int H = wgtLangLabels->height();
	int W = wgtLangLabels->width();
	while (block.isValid() && height < viewheight ) {
#if 0
		DaoCodeLineData *ud = (DaoCodeLineData*) block.userData();
		if( ud && (ud->def_class or ud->def_method) ){
			int y = (block.blockNumber() * H) / B;
			painter.drawText(0, y, 2*W, fm.height(), Qt::AlignRight, "testtesttestest");
		}
#endif
		height += (int) blockBoundingRect(block).height();
		block = block.next();
		last += 1;
	}
	block = document()->begin();
	while (block.isValid() ) {
		DaoCodeLineData *ud = (DaoCodeLineData*) block.userData();
		if( ud && (ud->def_class or ud->def_method) ){
			int y = (block.blockNumber() * H) / B;
			painter.fillRect( 0, y, W, 1, ud->def_class ? Qt::green : Qt::yellow );
		}
		block = block.next();
	}
	top = (first * wgtLangLabels->height()) / blockCount();
	bottom = (last * wgtLangLabels->height()) / blockCount();

	QRect rect( 0, top-5, wgtLangLabels->width(), bottom-top+10 );
	QLinearGradient linearGradient( 0, top-5, 0, bottom+5 );
	linearGradient.setColorAt(0.0, QColor( 0, 0, 255, 30 ) );
	linearGradient.setColorAt(0.03, QColor( 0, 0, 255, 50 ) );
	linearGradient.setColorAt(0.06, QColor( 0, 0, 255, 80 ) );
	linearGradient.setColorAt(0.1, QColor( 0, 0, 255, 90 ) );
	linearGradient.setColorAt(0.2,  QColor( 0, 0, 255, 110 ) );
	linearGradient.setColorAt(0.5,  QColor( 0, 0, 255, 120 ) );
	linearGradient.setColorAt(0.8,  QColor( 0, 0, 255, 110 ) );
	linearGradient.setColorAt(0.9, QColor( 0, 0, 255, 90 ) );
	linearGradient.setColorAt(0.94, QColor( 0, 0, 255, 80 ) );
	linearGradient.setColorAt(0.97, QColor( 0, 0, 255, 50 ) );
	linearGradient.setColorAt(1.0, QColor( 0, 0, 255, 30 ) );
	painter.fillRect( rect, linearGradient );
	wgtLangLabels->top = top;
	wgtLangLabels->bottom = bottom;
}
void DaoEditor::slotTextChanged()
{
	//UpdateCursor( showCursor ); // crash in mac
	if( toPlainText().size() ) ready = true;
	if( ready && state == false ){
		emit signalTextChanged( true );
		state = true;
	}
}
void DaoEditor::ResetBlockState()
{
	QTextBlock block = document()->firstBlock();
	while( block.isValid() ){
		DaoCodeLineData *ud = (DaoCodeLineData*) block.userData();
		if( ud == NULL ){
			block.setUserData( new DaoCodeLineData(false, CLS_NORMAL, block.blockNumber()+1) );
		}else{
			ud->breaking = false;
			ud->state = CLS_NORMAL;
		}
		SetIndentData( block );
		block = block.next();
	}
	repaint();
}
void DaoEditor::slotContentsChange( int pos, int rem, int add )
{
	//printf( "%4i  %4i  %4i\n", pos, rem, add );
	QTextBlock block1 = document()->findBlock( pos );
	QTextBlock block2 = document()->findBlock( pos + add );
	DaoCodeLineData *ud1 = (DaoCodeLineData*) block1.userData();
	DaoCodeLineData *ud2 = (DaoCodeLineData*) block2.userData();
	if( ud1 == NULL ){
		block1.setUserData( new DaoCodeLineData(false, CLS_CHANGED) );
	}else{
		ud1->breaking = false;
		ud1->state = CLS_CHANGED;
	}
	if( add ){
		if( ud2 == NULL ){
			block2.setUserData( new DaoCodeLineData(false, CLS_CHANGED) );
		}else{
			ud2->breaking = false;
			ud2->state = CLS_CHANGED;
		}
		while( block1 != block2 ){
			block1 = block1.next();
			if( not block1.isValid() ) break;
			ud1 = (DaoCodeLineData*) block1.userData();
			if( ud1 == NULL ){
				block1.setUserData( new DaoCodeLineData(false, CLS_CHANGED) );
			}else{
				ud1->breaking = false;
				ud1->state = CLS_CHANGED;
			}
		}
	}
	if( blockEntry.isValid() && block1 < blockEntry ) blockEntry = block1;
}
void DaoEditor::SetState( bool s )
{
	state = s;
	emit signalTextChanged( state );
}
void DaoEditor::SetExePoint( int entry )
{
	//QMessageBox::about( this, "test", QString::number( entry ) );
	blockEntry = document()->findBlockByNumber( entry - 1 );
	repaint();
}
void DaoEditor::SetEditLines( int start, int end )
{
	routCodes.clear();
	QTextBlock block = document()->firstBlock();
	while( block.isValid() ){
		int id = block.blockNumber() + 1;
		enum CodeLineStates state = id >= start && id <= end ? CLS_NORMAL : CLS_READONLY;
		if( state == CLS_NORMAL ) routCodes << block.text();
		DaoCodeLineData *ud = (DaoCodeLineData*) block.userData();
		ud->state = state;
		ud->line = id - start;
		block = block.next();
	}
	//printf( "%s\n", routCodes.join("\n").toLocal8Bit().data() );
	repaint();
}
void DaoEditor::slotBoundEditor()
{
	QTextBlock block = textCursor().block();
	DaoCodeLineData *ud = (DaoCodeLineData*)block.userData();
	if( ud ) setReadOnly( ud->state == CLS_READONLY );
	if( not isReadOnly() && wgtWords->IsTip() ) wgtWords->hide();
}
QString NormalizeCodes( const QString & source, DaoLexer *lexer )
{
	QString norm;
	DaoLexer_Tokenize( lexer, source.toLocal8Bit().data(), 0 );
	for(size_t i=0; i<lexer->tokens->size; i++){
		norm += lexer->tokens->items.pToken[i]->string.mbs;
		norm += '\n';
	}
	return norm;
}
bool DaoEditor::EditContinue2()
{
	int i, j, k, dest = 0;
	QTextBlock block = document()->firstBlock();
	newCodes.clear();
	lineMap.clear();
	breakPoints.clear();
	while( block.isValid() ){
		DaoCodeLineData *ud = (DaoCodeLineData*) block.userData();
		if( ud->breaking ) breakPoints[ block.blockNumber() + 1 ] = 1;
		if( ud->state <= CLS_CHANGED ){
			QString line = block.text();
			newCodes << line;
			if( ud->state ){
				if( dest < routCodes.size() && line == routCodes[dest] ){
					lineMap.append( dest );
				}else{
					lineMap.append( -1 );
				}
			}else{
				lineMap.append( ud->state == CLS_CHANGED ? -1 : ud->line );
			}
			dest = lineMap.back() + 1;
			if( dest ==0 ) dest = 1E9;
		}
		block = block.next();
	}
	newEntryLine = -1;
	if( blockEntry.isValid() && blockEntry.blockNumber()+1 != newEntryLine ){
		newEntryLine = blockEntry.blockNumber()+1;
	}
	//for(i=0; i<lineMap.size(); i++) printf( "%3i  %3i\n", i, lineMap[i] );
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
		if( eq ) return false;
	}
	//printf( "=======%s\n", newCodes.join("\n").toLocal8Bit().data() );

	Save();
	return true;
}
void DaoEditor::slotOutlineChanged()
{
	outlineChanged = true;
}
void DaoEditor::UpdateOutline()
{
	outlineChanged = false;
	codeThumb->clear();
	codeThumb->lines.clear();
	QTextBlock block = document()->firstBlock();
	while( block.isValid() ){
		DaoCodeLineData *ud = (DaoCodeLineData*) block.userData();
		if( ud and (ud->def_class or ud->def_method) ){
			QString text = block.text();
			if( text.indexOf( "static " ) ==0 ) text = text.mid( 7 );
			codeThumb->appendPlainText( text );
			codeThumb->lines.append( block.blockNumber() );
		}
		block = block.next();
	}
	codeThumb->lines.append( blockCount() );
	codeThumb->UpdateOutline();
	codeThumb->horizontalScrollBar()->setSliderPosition(0);
}


DaoCodeThumb::DaoCodeThumb( DaoEditor *parent ) 
: QPlainTextEdit( parent ), codehl( this->document() )
{
	font_size = 10;
	editor = parent;
	viewport()->setCursor( Qt::CrossCursor );
	setMouseTracking( true );
	setViewportMargins( 5, 10, 5, 10 );
	setLineWrapMode( QPlainTextEdit::NoWrap );
	setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

	QString style = "background-color: rgb(200, 250, 200, 255);"
		"border-style: outset; border-width: 1px;"
		"border-radius: 8px; border-color: rgb(0, 250, 0, 150);";
	setReadOnly( true );
	setStyleSheet( style );
	codehl.SetState(DAO_HLSTATE_NORMAL);
	codehl.SetColorScheme( 0 );
	codehl.SetFontSize( font_size );
}
void DaoCodeThumb::UpdateOutline()
{
	QTextBlock block = document()->firstBlock();
	DaoCodeLineData *last = NULL;
	int prev = 0;
	float ratio = 1 + exp( 1 - lines.size() / 10.0 );
	while( block.isValid() ){
		DaoCodeLineData *ud = (DaoCodeLineData*) block.userData();
		int cur = lines[ block.blockNumber()];
		int size = ratio*(8 + log( cur - prev ));
		if( size > 12 ) size = 12;
		if( last ) last->font_size = last->font_size2 = size;
		block = block.next();
		prev = cur;
		last = ud;
	}
	int cur = lines[ lines.size() - 1];
	int size = ratio*(8 + log( cur - prev ));
	if( size > 12 ) size = 12;
	if( last ) last->font_size = last->font_size2 = size;
	codehl.rehighlight();
}
void DaoCodeThumb::mousePressEvent ( QMouseEvent * event )
{
	QTextCursor cursor = cursorForPosition( event->pos() );
	QTextBlock block = cursor.block();
	int line = lines[block.blockNumber()];
	block = editor->document()->findBlockByLineNumber( line );
	cursor = QTextCursor( block );
	editor->setTextCursor( cursor );
	editor->centerCursor();
	editor->UpdateCursor( true );
}
void DaoCodeThumb::mouseMoveEvent ( QMouseEvent * event )
{
	int i, last = -1;
	int y = event->y();
	int max = editor->GetFont().pointSize();
	if( max < 15 ) max = 15;
	if( y <= 10 or y > viewport()->height()-10 ){
		QPoint pos = event->pos();
		if( y <= 10 ) pos.setY( 11 );
		if( y > (viewport()->height()-10) ) pos.setY( viewport()->height()-11 );
		pos = viewport()->mapToGlobal( pos );
		setMouseTracking( false );
		QCursor::setPos( event->globalX(), pos.y() );
		setMouseTracking( true );
	}
#if 0
#endif
	for(i=0; i<5; i++){
		QTextBlock block = firstVisibleBlock();
		int mid = block.blockNumber();
		int height = 0;
		while( block.isValid() ){
			DaoCodeLineData *ud = (DaoCodeLineData*) block.userData();
			QFont font = codehl.GetFont();
			if( ud and ud->font_size ) font.setPointSize( ud->font_size );
			QFontMetrics fm( font );
			height += fm.lineSpacing();
			mid = block.blockNumber();
			if( height >= y ) break;
			block = block.next();
		}
		//printf( "%i: %4i %4i %4i %s\n", i, y, height, mid, block.text().toUtf8().data() );
		if( mid == last ) break;
		block = document()->firstBlock();
		while( block.isValid() ){
			int dist = abs( block.blockNumber() - mid );
			DaoCodeLineData *ud = (DaoCodeLineData*) block.userData();
			if( ud ){
				int font_size = ud->font_size2;
				switch( dist ){
				case 0 : font_size = max; break;
				case 1 : font_size = max-1; break;
				case 2 : font_size = max-2; break;
				case 3 : font_size = max-3; break;
				case 4 : font_size = max-4; break;
				case 5 : font_size = max-5; break;
				case 6 : font_size = max-6; break;
				}
				if( font_size < ud->font_size2 ) font_size = ud->font_size2;
				ud->rehighlight = ud->rehighlight or (font_size != ud->font_size);
					ud->font_size = font_size;
			}
			block = block.next();
		}
	}
	QTextBlock block = document()->firstBlock();
	while( block.isValid() ){
		DaoCodeLineData *ud = (DaoCodeLineData*) block.userData();
		if( ud and ud->rehighlight ){
			codehl.rehighlightBlock( block );
			ud->rehighlight = false;
			QTextCursor cursor( block );
			setTextCursor( cursor );
			ensureCursorVisible();
		}
		block = block.next();
	}
	horizontalScrollBar()->setSliderPosition(0);
}
void DaoCodeThumb::leaveEvent( QEvent * event )
{
	if( isVisible() ) hide();
	editor->setFocus();
}
