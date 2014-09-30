/* Packed data structure templates for network packets*/

#define B char
#define W uint16_t
#define L uint32_t
#define D uint64_t
#define swap_16(x) (((x) & 0x00ff) << 8 | ((x) & 0xff00) >> 8)
#define swap_32(x) (((x) & 0x00ff) << 24 | ((x) & 0xff00) << 8 | ((x) & 0xff0000) >> 8 | ((x) & 0xff000000) >> 24)

#include "netrshareenum.h"
#include "rxpackettemplates.h"
#include "NFSpackets.h"

static struct frame0
{
W tid;
W mode;
W qdcount;
W ancount;
W nscount;
W arcount;
B queryname[34];
W nbstat;
W in;
}__attribute__((__packed__)) statreq={ .qdcount=swap_16(0x0001),
  .queryname=" CKAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
  .in=swap_16(0x0001) };

static struct frame1
{
W sessionflags;
W length;
B callee[34];
B caller[34];
}__attribute__((__packed__)) sessionreq={ .sessionflags=swap_16(0x8100),
  .length=swap_16(0x0044) };

static struct frame2
{
W sessionflags;
W length;
L ffsmb_string;
B proto_command;
B error_class;
B reserved;
W error_code;
B flags;
W flags2;
W pid_hi;
D signature;
W reserved2;
W tid;
W pid;
W uid;
W muxid;
B wordcount;
W bytecount;
B dialect;			// Always 2. Must preceed each protocol string listed.
B string[11];		// First and in our case only protocol string.
}__attribute__((__packed__)) protocol={ .length=swap_16(47),
  .ffsmb_string=0x424D53FF, .proto_command=0x72, .flags=0x08, .flags2=0x0001,
  .tid=0xFFFF, .muxid=2, .bytecount=12, .dialect=2,
  .string[0]="NT LM 0.12" };

static struct frame3
{
W sessionflags;
W length;
L ffsmb_string;
B setup_command;
B error_class;
B reserved;
W error_code;
B flags;
W flags2;
W pid_hi;
D signature;
W reserved2;
W tid;
W pid;
W uid;
W muxid;
B wordcount;
B AndXCommand;
B reserved3;
W AndXOffset;
W maxbuffer;
W maxmpxcount;
W vcnumber;
L sessionkey;
W ansipasswordlen;
W unicodepasswordlen;
L reserved4;
L capabilities;
W bytecount;
B ansipassword;
B strings[64];
}__attribute__((__packed__)) setup={ .ffsmb_string=0x424D53FF,
  .setup_command=0x73, .flags=0x08, .flags2=0x0001,
  .muxid=3, .wordcount=13,
  .AndXCommand=0xFF, .maxbuffer=1024, .maxmpxcount=2, 
  .ansipasswordlen=1, .capabilities=0x00000203, .strings[0]="Guest" };

static struct frame4
{
W sessionflags;
W length;
L ffsmb_string;
B conn_command;
B error_class;
B reserved;
W error_code;
B flags;
W flags2;
W pid_hi;
D signature;
W reserved2;
W tid;
W pid;
W uid;
W muxid;
B wordcount;
B AndXCommand;
B reserved3;
W AndXOffset;
W flags3;
W passwordlen;
W bytecount;
B password;
B strings[32];
}__attribute__((__packed__)) treeconn={ .ffsmb_string=0x424D53FF,
  .conn_command=0x75, .flags=0x08, .flags2=0x0001,
  .tid=0xFFFF, .muxid=4, .wordcount=4,
  .AndXCommand=0xFF, .passwordlen=1, .bytecount=32, .strings[0]="\\\\" };


static struct frame5
{
W sessionflags;
W length;
L ffsmb_string;
B transaction_command;
B error_class;
B reserved;
W error_code;
B flags;
W flags2;
W pid_hi;
D signature;
W reserved2;
W tid;
W pid;
W uid;
W muxid;
B wordcount;
W totparamcnt;
W totdatacnt;
W maxparamcnt;
W maxdatacnt;
B maxsetupcnt;
B reserved3;
W flags3;
L timeout;
W reserved4;
W parametercount;
W parameteroffset;
W datacount;
W dataoffset;
B setupcount;
B reserved5;
W bytecount;
B transaction_name[13];
W functioncode;
B paramdesc[6];
B returndesc[7];
W detail_level;
W rxbufferlen;
}__attribute__((__packed__)) enumshares={ .length=swap_16(95),
  .ffsmb_string=0x424D53FF, .transaction_command=0x25, .flags=0x08,
  .flags2=0x0001, .muxid=6, .wordcount=14,
  .totparamcnt=19, .maxparamcnt=64, .maxdatacnt=4000, .flags3=0x0000,
  .parametercount=19, .parameteroffset=76, .dataoffset=95, .bytecount=32,
  .transaction_name="\\PIPE\\LANMAN", .paramdesc="WrLeh",
  .returndesc="B13BWz", .detail_level=1, .rxbufferlen=4000 };


static struct frame6
{
W sessionflags;
W length;
L ffsmb_string;
B transaction_command;
B error_class;
B reserved;
W error_code;
B flags;
W flags2;
W pid_hi;
D signature;
W reserved2;
W tid;
W pid;
W uid;
W muxid;
B wordcount;
W totparamcnt;
W totdatacnt;
W maxparamcnt;
W maxdatacnt;
B maxsetupcnt;
B reserved3;
W flags3;
L timeout;
W reserved4;
W parametercount;
W parameteroffset;
W datacount;
W dataoffset;
B setupcount;
B reserved5;
W bytecount;
B transaction_name[13];
W functioncode;
B paramdesc[8];
B returndesc[8];
W detail_level;
W rxbufferlen;
L servertype;
B workgroup[16];
}__attribute__((__packed__)) enumservers2={ .length=swap_16(118),
  .ffsmb_string=0x424D53FF, .transaction_command=0x25, .flags=0x08, .flags2=0x0001,
  .muxid=8, .wordcount=14,
  .totparamcnt=40, .maxparamcnt=8, .maxdatacnt=4000, .flags3=0x0001,
  .parametercount=40, .parameteroffset=76, .dataoffset=116, .bytecount=53,
  .transaction_name="\\PIPE\\LANMAN", .functioncode=104, .paramdesc="WrLehDz",
  .returndesc="B16BBDz", .detail_level=1, .rxbufferlen=4000, .servertype=0xffffffff };

// This call is not supported by Samba

static struct frame6A
{
W sessionflags;
W length;
L ffsmb_string;
B transaction_command;
B error_class;
B reserved;
W error_code;
B flags;
W flags2;
W pid_hi;
D signature;
W reserved2;
W tid;
W pid;
W uid;
W muxid;
B wordcount;
W totparamcnt;
W totdatacnt;
W maxparamcnt;
W maxdatacnt;
B maxsetupcnt;
B reserved3;
W flags3;
L timeout;
W reserved4;
W parametercount;
W parameteroffset;
W datacount;
W dataoffset;
B setupcount;
B reserved5;
W bytecount;
B transaction_name[13];
//B Padding;			// Used to word align Unicode strings
//B transaction_name[26];
//W Padding2;			// Used to word align Unicode strings
W functioncode;
B paramdesc[5];
B returndesc[8];
W detail_level;
W rxbufferlen;
// Non Unicode version
}__attribute__((__packed__)) netservergetinfo={ .length=swap_16(95),
  .ffsmb_string=0x424D53FF, .transaction_command=0x25, .flags=0x08,
  .flags2=0x0001, .muxid=9, .wordcount=14,
  .totparamcnt=19, .maxparamcnt=16, .maxdatacnt=4000, .flags3=0x0000,
  .parametercount=19, .parameteroffset=76, .dataoffset=95, .bytecount=32,
  .transaction_name="\\PIPE\\LANMAN", .functioncode=13, .paramdesc="WrLh",
  .returndesc="B16BBDz", .detail_level=1, .rxbufferlen=4000 };

/* 
// Unicode version
}__attribute__((__packed__)) netservergetinfo={ .length=swap_16(111),
  .ffsmb_string=0x424D53FF, .transaction_command=0x25, .flags=0x08,
  .flags2=0x8001, .muxid=9, .wordcount=14,
  .totparamcnt=19, .maxparamcnt=6, .maxdatacnt=4000, .flags3=0x0000,
  .parametercount=19, .parameteroffset=92, .dataoffset=111, .bytecount=48,
  .transaction_name="\\\0P\0I\0P\0E\0\\\0L\0A\0N\0M\0A\0N\0\0", .functioncode=13, .paramdesc="WrLh",
  .returndesc="B16BBDz", .detail_level=1, .rxbufferlen=4000 };
*/

#undef B
#undef W
#undef L
#undef D
#undef swap_16
#undef swap_32
