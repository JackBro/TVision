/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   DRAWBUF.H                                                             */
/*                                                                         */
/*   Copyright (c) Borland International 1991                              */
/*   All Rights Reserved.                                                  */
/*                                                                         */
/*   defines the class TDrawBuffer, which provides the high-level          */
/*   interface to the Screen Manager.                                      */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#if defined( Uses_TDrawBuffer ) && !defined( __TDrawBuffer )
#define __TDrawBuffer

class TDrawBuffer
{

    friend class TSystemError;
    friend class TView;
    friend void genRefs();
    friend class TFrame;

public:

    void moveChar( ushort indent, char c, ushort attr, ushort count );
    void moveStr( ushort indent, const char *str, ushort attrs );
    void moveCStr( ushort indent, const char *str, ushort attrs );
    void moveBuf( ushort indent, const void *source,
                  ushort attr, ushort count );

    void putAttribute( ushort indent, ushort attr );
    void putChar( ushort indent, ushort c );

protected:

    ushort data[maxViewWidth];

};

#ifdef __ppc__
#define loByte(w)    (((uchar *)(&w))[1])
#define hiByte(w)    (((uchar *)(&w))[0])
#else
#define loByte(w)    (((uchar *)(&w))[0])
#define hiByte(w)    (((uchar *)(&w))[1])
#endif

inline void TDrawBuffer::putAttribute( ushort indent, ushort attr )
{
    hiByte(data[indent]) = (uchar)attr;
}

inline void TDrawBuffer::putChar( ushort indent, ushort c )
{
    loByte(data[indent]) = (uchar)c;
}

#endif  // Uses_TDrawBuffer


