/*------------------------------------------------------------*/
/* filename -       tcluster.cpp                              */
/*                                                            */
/* function(s)                                                */
/*                  TCluster member functions                 */
/*------------------------------------------------------------*/

/*------------------------------------------------------------*/
/*                                                            */
/*    Turbo Vision -  Version 1.0                             */
/*                                                            */
/*                                                            */
/*    Copyright (c) 1991 by Borland International             */
/*    All Rights Reserved.                                    */
/*                                                            */
/*------------------------------------------------------------*/

#define Uses_TKeys
#define Uses_TCluster
#define Uses_TDrawBuffer
#define Uses_TEvent
#define Uses_TPoint
#define Uses_TSItem
#define Uses_TStringCollection
#define Uses_TGroup
#define Uses_opstream
#define Uses_ipstream
#include <tv.h>

#define cpCluster "\x10\x11\x12\x12\x21"

TCluster::TCluster( const TRect& bounds, TSItem *aStrings ) :
    TView(bounds),
    value( 0 ),
    enabled( 0xFFFF ),
    sel( 0 )
{
    options |= ofSelectable | ofFirstClick | ofPreProcess | ofPostProcess;
    short i = 0;
    TSItem *p;
    for( p = aStrings; p != 0; p = p->next )
        i++;

    strings = new TStringCollection( i, 0 );

    while( aStrings != 0 )
        {
        p = aStrings;
        strings->atInsert( strings->getCount(), newStr(aStrings->value) );
        aStrings = aStrings->next;
        delete p;
        }

    setCursor( 2, 0 );
    showCursor();
}

TCluster::~TCluster()
{
    destroy( (TCollection *)strings );
}

const char *TCluster::getItemText( ccIndex item )
{
    return (const char *)strings->at( item );
}

size_t TCluster::dataSize()
{
    return sizeof(value);
}

/*
                        H���ࠡ�⪠ � TCluster.
        (Michael Reztsov, 2:5069/2.45, Sun 11 May 97 20:53)

�᫨ � �� ��������� ����� ������⢮ ��ப �ॢ�蠥� ࠧ���
�������� ������� �� ���⨪���, ᮧ������ ���� � �.�. ������� ���
�⮡ࠦ���� ��� ��ப. ����� ��ਧ��⠫�� ࠧ��� �������祭 ���
�⮡ࠦ���� ��� �� ��ࢮ�� ᨬ���� �� ᫥���饩 �������, � � ��⮤�
drawBox �뫥���� ⠪�� �訡��: ��᫥ ���� ����樨 �� ��ਧ��⠫�
᫥���饩 ������� (��६����� col) ��뢠���� ��⮤ moveChar ���� �
��ࠬ��஬ Count ࠢ�� (size.x - col). �.�. col ����� size.x, �
��।����� ����⥫쭮� �᫮, ���஥ � ��設��� ���� �룫廊� ���
����讥 ������⥫쭮�. � �⮣� - ��室 �� �࠭��� ���� � ᥣ����
�⥪�. � DOS'� ��設� ����� ��������. ����筮, �� ��࠭�� �ࠢ��쭮�
������� ��ਧ��⠫쭮�� ࠧ��� ������� �訡�� �� ���������, �� ��
��砩����...

    ��ப� � �訡��� ��뢠�� moveChar ��� ���⪨ ���� �
��।������� ����樨. ����� �������� ��ࠦ����, ��뢠�饥 �訡�� ��
abs(size.x - col), � ����� ���������஢��� ��� ��ப� � ���⠢��� �
��砫� ���譥�� 横�� ����� ���� �� size.x �������� �������.
��ன ��ਠ�� ����� �ਣ������� � ��砥, ����� ��� ������� �����
���⨪���� ࠧ���, �ॢ���騩 ������⢮ �뢮����� ��ப (����
�뢮����� �, �� �뫮 � ����, � ������ �।���� ��ப�).
������⢨� �ࠧ� ����� ��ப �� �㦭�.

*/

void TCluster::drawBox( const char *icon, char marker)
{
    TDrawBuffer b;
    ushort color;

    ushort cDis = getColor( 0x0505 );
    ushort cNorm = state & sfDisabled ? cDis : getColor( 0x0301 );
    ushort cSel = getColor( 0x0402 );
    for( int i = 0; i <= size.y; i++ )
        {
        for( int j = 0; j <= (strings->getCount()-1)/size.y + 1; j++ )
            {
            int cur = j * size.y + i;
            int col = column( cur );
            if ( cur < strings->getCount() &&
                (size_t)(col+cstrlen(getItemText(cur))+5) < maxViewWidth &&
                col<size.x )
               {
                if ( !isEnabledItem(cur) )
                    color = cDis;
                else if( (cur == sel) && (state & sfSelected) != 0 )
                    color = cSel;
                else
                    color = cNorm;
                b.moveChar( ushort(col), ' ', color, ushort(size.x - col) );
                b.moveCStr( ushort(col), icon, color );
                if( mark(cur) )
                    b.putChar( ushort(col+2), marker );
                b.moveCStr( ushort(col+5), (char *)(strings->at(cur)), color );
                if( showMarkers && (state & sfSelected) != 0 && cur == sel )
                    {
                    b.putChar( ushort(col), specialChars[0] );
                    b.putChar( ushort(column(cur+size.y)-1), specialChars[1] );
                    }
                }
            }
        writeBuf( 0, ushort(i), ushort(size.x), 1, b );
        }
    setCursor( ushort(column(sel)+2), ushort(row(sel)) );
}

void TCluster::getData(void * rec, size_t recsize)
{
  if ( ssize_t(recsize) >= (ssize_t)sizeof(ushort) )
    *(ushort*)rec = value;
}

ushort TCluster::getHelpCtx()
{
    if( helpCtx == hcNoContext )
        return hcNoContext;
    else
        return ushort(helpCtx + sel);
}

TPalette& TCluster::getPalette() const
{
    static TPalette palette( cpCluster, sizeof( cpCluster )-1 );
    return palette;
}

void TCluster::handleEvent( TEvent& event )
{
    TView::handleEvent(event);
    if ( ! (options & ofSelectable) ) return;
    if( event.what == evMouseDown )
        {
        TPoint mouse = makeLocal( event.mouse.where );
        int i = findSel(mouse);
        if( i != -1 )
            sel = i;
        drawView();
        do  {
            mouse = makeLocal( event.mouse.where );
            if( findSel(mouse) == sel )
                showCursor();
            else
                hideCursor();
            } while( mouseEvent(event,evMouseMove) );
        showCursor();
        mouse = makeLocal( event.mouse.where );
        if( findSel(mouse) == sel )
            {
            press(sel);
            drawView();
            }
        clearEvent(event);
        }
    else if( event.what == evKeyDown )
        switch (ctrlToArrow(event.keyDown.keyCode))
            {
            case kbUp:
                if( (state & sfFocused) != 0 && enabled != 0 )
                    {
MoveUp:
                    do
                    {
                      if( --sel < 0 )
                        sel = strings->getCount()-1;
                    } while ( !isEnabledItem(sel) );
                    movedTo(sel);
                    drawView();
                    clearEvent(event);
                    }
                break;

            case kbDown:
                if( (state & sfFocused) != 0 && enabled != 0 )
                    {
MoveDown:
                    do
                    {
                      if( ++sel >= strings->getCount() )
                          sel = 0;
                    } while ( !isEnabledItem(sel) );
                    movedTo(sel);
                    drawView();
                    clearEvent(event);
                    }
                break;
            case kbRight:
                if( (state & sfFocused) != 0 && enabled != 0 )
                    {
                    sel += size.y;
                    if( sel >= strings->getCount() )
                        {
                        sel = (sel +  1) % size.y;
                        if( sel >= strings->getCount() )
                            sel =  0;
                        }
                    if ( !isEnabledItem(sel) )
                      goto MoveDown;
                    movedTo(sel);
                    drawView();
                    clearEvent(event);
                    }
                break;
            case kbLeft:
                if( (state & sfFocused) != 0 && enabled != 0 )
                    {
                    if( sel > 0 )
                        {
                        sel -= size.y;
                        if( sel < 0 )
                            {
                            sel = ((strings->getCount()+size.y-1) /size.y)*size.y + sel - 1;
                            if( sel >= strings->getCount() )
                                sel = strings->getCount()-1;
                            }
                        }
                    else
                        sel = strings->getCount()-1;
                    if ( !isEnabledItem(sel) )
                      goto MoveDown;
                    movedTo(sel);
                    drawView();
                    clearEvent(event);
                    }
                break;
            default:
                for( int i = 0; i < strings->getCount(); i++ )
                    {
                    char c = hotKey( (char *)(strings->at(i)) );
                    if( !isEnabledItem(i) )
                        continue;
                    if( getAltCode(c) == event.keyDown.keyCode ||
                        ( ( owner->phase == phPostProcess ||
                            (state & sfFocused) != 0
                          ) &&
                          c != 0 &&
                          toupper(event.keyDown.charScan.charCode) == c
                        )
                      )
                        {
                        select();
                        sel =  i;
                        movedTo(sel);
                        press(sel);
                        drawView();
                        clearEvent(event);
                        return;
                        }
                    }
                if( event.keyDown.charScan.charCode == ' ' &&
                    (state & sfFocused) != 0
                  )
                    {
                    press(sel);
                    drawView();
                    clearEvent(event);
                    }
            }
}

void TCluster::setData(void * rec)
{
    value =  *(ushort *)rec;
    drawView();
}

void TCluster::setState( ushort aState, Boolean enable )
{
    TView::setState( aState, enable );
    if( aState == sfSelected || aState == sfDisabled )
        drawView();
}

void TCluster::setEnabledItem( int item, Boolean enable )
{
    ushort mask = 1 << item;
    if ( enable )
        enabled |= mask;
    else
        enabled &= ~mask;
    drawView();
}

void TCluster::setItem( int , Boolean )
{
}

Boolean TCluster::mark( int )
{
    return False;
}

void TCluster::movedTo( int )
{
}

void TCluster::press( int item )
{
  if ( onchange != NULL )
    onchange->changed(this, item);
}

int TCluster::column( int item )
{
    if( item < size.y )
        return 0;
    else
        {
        int width = 0;
        int col = -6;
        int l = 0;
        for( int i = 0; i <= item; i++ )
            {
            if( i % size.y == 0 )
                {
                col += width + 6;
                width = 0;
                }

            if( i < strings->getCount() )
                l = cstrlen( (char *)(strings->at(i)) );
            if( l > width )
                width = l;
            }
        return col;
        }
}

int TCluster::findSel( TPoint p )
{
    TRect r = getExtent();
    if( !r.contains(p) )
        return -1;
    else
        {
        int i = 0;
        while( p.x >= column( i + size.y ) )
            i += size.y;
        int s = i + p.y;
        if( s >= strings->getCount() || !isEnabledItem(s) )
            return -1;
        else
            return s;
        }
}

int TCluster::row( int item )
{
    return item % size.y;
}

#ifndef NO_TV_STREAMS
void TCluster::write( opstream& os )
{
    TView::write( os );
    os << value << sel << strings;
}

void *TCluster::read( ipstream& is )
{
    TView::read( is );
    is >> value >> sel >> strings;
    setCursor( 2, 0 );
    showCursor();
    return this;
}

TStreamable *TCluster::build()
{
    return new TCluster( streamableInit );
}

TCluster::TCluster( StreamableInit ) : TView( streamableInit )
{
}
#endif  // ifndef NO_TV_STREAMS


