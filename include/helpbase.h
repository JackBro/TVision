/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   HELPBASE.H                                                            */
/*                                                                         */
/*   Copyright (c) Borland International 1991                              */
/*   All Rights Reserved.                                                  */
/*                                                                         */
/*   defines the classes TParagraph, TCrossRef, THelpTopic, THelpIndex,    */
/*      THelpFile                                                          */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#ifdef NO_TV_STREAMS
#include <stdio.h>              // we use FILE * instead of fpstream.
#endif  // ifdef NO_TV_STREAMS

const int32 magicHeader = 0x46484246L; //"FBHF"

#define cHelpColor      "\x37\x3F\x3A\x13\x13\x30\x3E\x1E"
#define cHelpBlackWhite "\x07\x0F\x07\x70\x70\x07\x0F\x70"
#define cHelpMonochrome "\x07\x0F\x07\x70\x70\x07\x0F\x70"
#define cHelpViewer     "\x06\x07\x08"
#define cHelpWindow     "\x40\x41\x42\x43\x44\x45\x46\x47"

// TParagraph

class TParagraph
{

public:

    TParagraph() {}
    TParagraph *next;
    Boolean wrap;
    ushort size;
    char *text;

};

// TCrossRef

#pragma pack(1) // Make sure that sizeof(TCrossRef) is 5, not 6

class TCrossRef
{

public:

    TCrossRef() {}
    short ref;
    short offset;
    uchar length;

    TPoint loc;         // added by ig 13.05.97
};

#pragma pack()

#ifndef NO_TV_STREAMS
typedef void (*TCrossRefHandler) ( opstream&, int );
extern void notAssigned( opstream& s, int value );
class THelpTopic: public TObject, public TStreamable
#else
typedef void (*TCrossRefHandler) (FILE *, int );
extern void notAssigned( FILE *fp, int value );
class THelpTopic: public TObject
#endif  // ifndef NO_TV_STREAMS
{

public:

    THelpTopic();
    THelpTopic( StreamableInit ) {};
    virtual ~THelpTopic();

    void addCrossRef( TCrossRef ref );
    void addParagraph( TParagraph *p );
    void getCrossRef( int i, TPoint& loc, uchar& length, int& ref );
    char *getLine( int line, char *buffer, size_t buflen );
    int getNumCrossRefs();
    int numLines();
    void setCrossRef( int i, TCrossRef& ref );
    void setNumCrossRefs( int i );
    void setWidth( int aWidth );

    TParagraph *paragraphs;

    ushort numRefs;
    TCrossRef *crossRefs;

private:

    char *wrapText( char *text, int size, int& offset, Boolean wrap,
                    char *lineBuf, size_t lineBufLen );
    void disposeParagraphs();
    int width;
    int lastOffset;
    int lastLine;
    TParagraph *lastParagraph;
#ifndef NO_TV_STREAMS
    void readParagraphs( ipstream& s );
    void readCrossRefs( ipstream& s );
    void writeParagraphs( opstream& s );
    void writeCrossRefs( opstream& s );
    const char *streamableName() const
        { return name; }

protected:

    virtual void write( opstream& );
    virtual void *read( ipstream& );

public:

    static const char * const name;
    static TStreamable *build();
#else   // ifndef NO_TV_STREAMS
    void readParagraphs(FILE *fp);
    void readCrossRefs(FILE *fp);
    void writeParagraphs(FILE *fp);
    void writeCrossRefs(FILE *fp);
public:
    void write(FILE *fp);
    void *read(FILE *fp);
#endif  // ifndef NO_TV_STREAMS

};

#ifndef NO_TV_STREAMS
inline ipstream& operator >> ( ipstream& is, THelpTopic& cl )
    { return is >> (TStreamable&)cl; }
inline ipstream& operator >> ( ipstream& is, THelpTopic*& cl )
    { return is >> (void *&)cl; }

inline opstream& operator << ( opstream& os, THelpTopic& cl )
    { return os << (TStreamable&)cl; }
inline opstream& operator << ( opstream& os, THelpTopic* cl )
    { return os << (TStreamable *)cl; }
#endif  // ifndef NO_TV_STREAMS


// THelpIndex

class THelpIndex : public TObject
#ifndef NO_TV_STREAMS
                                , public TStreamable
#endif  // ifndef NO_TV_STREAMS
{
public:


    THelpIndex();
    THelpIndex( StreamableInit ) {};
    virtual ~THelpIndex();

    int32 position( int );
    void add( int, int32 );

    ushort size;
    int32 *index;

private:

#ifndef NO_TV_STREAMS
    const char *streamableName() const
        { return name; }

protected:

    virtual void write( opstream& );
    virtual void *read( ipstream& );

public:

    static const char * const name;
    static TStreamable *build();
#else
public:


    void read( FILE *fp );
    void write( FILE *fp );

#endif  // ifndef NO_TV_STREAMS

};

#ifndef NO_TV_STREAMS
inline ipstream& operator >> ( ipstream& is, THelpIndex& cl )
    { return is >> (TStreamable&)cl; }
inline ipstream& operator >> ( ipstream& is, THelpIndex*& cl )
    { return is >> (void *&)cl; }

inline opstream& operator << ( opstream& os, THelpIndex& cl )
    { return os << (TStreamable&)cl; }
inline opstream& operator << ( opstream& os, THelpIndex* cl )
    { return os << (TStreamable *)cl; }
#endif  // ifndef NO_TV_STREAMS


// THelpFile

class THelpFile : public TObject
{

public:

#ifndef NO_TV_STREAMS
    THelpFile( fpstream& s );
    fpstream *stream;
#else
    THelpFile( FILE *fp );              // we use FILE * interface
    FILE *hfp;
#endif  // ifndef NO_TV_STREAMS
    virtual ~THelpFile();

    THelpTopic *getTopic( int );
    THelpTopic *invalidTopic();
    void recordPositionInIndex( int );
    void putTopic( THelpTopic* );

    Boolean modified;

    THelpIndex *index;
    int32 indexPos;
};

extern TCrossRefHandler crossRefHandler;

extern const char *tv_dynhelp;   // 02.08.98: dynamic help message for forms
extern int tv_helpIndex;
extern int tv_idcIndex;
extern int tv_helponhelp;

