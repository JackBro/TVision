
TVision by Borland ported to OS/2, MS Windows, DOS32, Linux, Mac OS X
---------------------------------------------------------------------

                                          Version 2009a (for IDA v5.5)

This port is partially based on the port of TVision to FreeBSD made by
Sergio Sigala.

I stripped down the source code and removed the curses stuff. Also
FreeBSD and GPM stuff was removed. In fact there is a lot removed from
the modifications made by Sergio Sigala, but since I have used his texts
as the starting point, I kept his copyright. More than that, the Linux
part has been completely rewritten by Yury Haron <yjh@styx.cabel.net>.

I also removed GNU configurator, 16-bit MS DOS support and various
obsolete stuff. The code is supposed to run on:

        MS Windows 32-bit
        MS Windows 64-bit
        Linux
        DOS32
        OS/2
        Mac OS X

I tested it on MS Windows (32 and 64), Linux, and Mac OS X machines

If you find any problems with the code, feel free to report them or even
better, to fix the found bugs :)

With best regards,
IDA Pro Team <support@hex-rays.com>

