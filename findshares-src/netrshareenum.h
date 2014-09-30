
static struct frame7
{
W sessionflags;
W length;
L ffsmb_string;
B NTcreate_command;
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
B reserved4;
W FileNameLen;
L CreateFlags;
L RootFID;
L AccessMask;
D AllocationSize;
L FileAttributes;
L ShareAccess;
L Disposition;
L CreateOptions;
L Impersonation;
B SecurityFlags;
W bytecount;
//B Padding;			// Used to word align Unicode strings
B Filename[8];
W ExtraBytes;
}__attribute__((__packed__)) NTcreate={ .length=swap_16(93),
  .ffsmb_string=0x424D53FF,
  .NTcreate_command=0xA2, .flags=0x08, .flags2=0x0001,
  .tid=0xFFFF, .muxid=5, .wordcount=24,
  .AndXCommand=0xFF, .FileNameLen=8, .AccessMask=0x00020089,
  .ShareAccess=0x00000001, .Disposition=0x00000001,
  .Impersonation=0x00000002,
  .bytecount=10, .Filename[0]="\\srvsvc" };

static struct frame8
{
// ----- NETBIOS HEADER
W sessionflags;
W length;				// # of bytes from next field to end of structure
// ----- SMB HEADER
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
W tid;					// Tree ID, assigned by server
W pid;					// Process ID, assigned by us
W uid;					// User ID, assigned by server
W muxid;				// Multiplex ID, incremented between packets by us
// ----- Transaction
B wordcount;			// # of words between wordcount and bytecount
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
W parameteroffset;		// In this case at least, same as dataoffset
W datacount;			// # of bytes from VersionMajor to end of structure
W dataoffset;			// # of bytes from ffsmb_string to VersionMajor
B setupcount;
B reserved5;
// ----- SMB Pipe Protocol
W TransactNmPipe;		// 0x26
W FID;					// File ID, assigned by server
W bytecount;			// # of bytes from next field to end of structure
B transaction_name[7];	// "\PIPE\"
// ----- DCE RPC Bind
B VersionMajor;
B VersionMinor;
B PacketType;
B PacketFlags;
L DataRepresentation;
W FragLength;			// # of bytes from VersionMajor to end of structure
W AuthLength;
L CallID;
W MaxXmitFrag;
W MaxRcvFrag;
L AssocGroup;
L NumCtxItems;
W ContextID;
W NumTransItems;
B SRVSVCUUID[16];
W InterfaceVer;
W InterfaceVerMinor;
B protocolUUID[16];
L ver;
}__attribute__((__packed__)) RPCbind={ .length=swap_16(146),
  .ffsmb_string=0x424D53FF, .transaction_command=0x25, .flags=0x08,
  .flags2=0x0001, .muxid=6, .wordcount=16,
  .totdatacnt=72, .maxdatacnt=4000, .flags3=0x0000,
  .parametercount=0, .parameteroffset=74, .datacount=72, .dataoffset=74,
  .setupcount=2, .TransactNmPipe=0x26, .bytecount=79,
  .transaction_name="\\PIPE\\", .VersionMajor=5, .PacketType=11,
  .PacketFlags=3, .DataRepresentation=0x10, .FragLength=72,
  .CallID=6, .MaxXmitFrag=4000, .MaxRcvFrag=4000,
  .NumCtxItems=1, .NumTransItems=1,
  .SRVSVCUUID={0xC8, 0x4F, 0x32, 0x4B, 0x70, 0x16, 0xD3, 0x01, 0x12,
  							0x78, 0x5A, 0x47, 0xBF, 0x6E, 0xE1, 0x88},
  .InterfaceVer=3,
  .protocolUUID={0x04, 0x5D, 0x88, 0x8A, 0xEB, 0x1C, 0xC9, 0x11, 0x9F,
  							0xE8, 0x08, 0x00, 0x2B, 0x10, 0x48, 0x60},
  .ver=2 };

static struct frame9
{
// ----- NETBIOS HEADER
W sessionflags;
W length;				// # of bytes from next field to end of structure
// ----- SMB HEADER
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
W tid;					// Tree ID, assigned by server
W pid;					// Process ID, assigned by us
W uid;					// User ID, assigned by server
W muxid;				// Multiplex ID, incremented between packets by us
// ----- Transaction
B wordcount;			// # of words between wordcount and bytecount
W totparamcnt;
W totdatacnt;			// # of bytes from VersionMajor to end of structure
W maxparamcnt;
W maxdatacnt;
B maxsetupcnt;
B reserved3;
W flags3;
L timeout;
W reserved4;
W parametercount;
W parameteroffset;		// In this case at least, same as dataoffset
W datacount;			// # of bytes from VersionMajor to end of structure
W dataoffset;			// # of bytes from ffsmb_string to VersionMajor
B setupcount;
B reserved5;
// ----- SMB Pipe Protocol
W TransactNmPipe;		// 0x26
W FID;					// File ID, assigned by server
W bytecount;			// # of bytes from next field to end of structure
B transaction_name[7];	// "\PIPE\"
// ----- DCE RPC Request
B VersionMajor;
B VersionMinor;
B PacketType;
B PacketFlags;
L DataRepresentation;
W FragLength;			// # of bytes from VersionMajor to end of structure
W AuthLength;
L CallID;
L AllocHint;			// # of bytes in Server Service section
W ContextID;
W Opnum;				// 15=NetrShareEnum
// ----- Server Service, NetrShareEnum (NetShareEnumAll)
L ReferentID;
L MaxCount;				// Size of ServerUnc
L Offset;
L ActualCount;			// # of bytes in server name
B ServerUnc[64];		// Server name + space to append the next 8 longs
L Level;
L Ctr;
L ReferentID2;
L Count;
L NullPtr;
L MaxBuffer;
L ReferentID3;
L ResumeHandle;
}__attribute__((__packed__)) NetrShareEnum={ .length=swap_16(162),
  .ffsmb_string=0x424D53FF, .transaction_command=0x25, .flags=0x08,
  .flags2=0x0001, .muxid=7, .wordcount=16,
  .totdatacnt=88, .maxdatacnt=4000, .flags3=0x0000,
  .parametercount=0, .parameteroffset=74, .datacount=88, .dataoffset=74,
  .setupcount=2, .TransactNmPipe=0x26, .bytecount=95,
  .transaction_name="\\PIPE\\", .VersionMajor=5, .PacketType=0,
  .PacketFlags=3, .DataRepresentation=0x10, .FragLength=88,
  .CallID=7, .AllocHint=64, .Opnum=15, .ReferentID=0x00020000,
  .MaxCount=16, .Level=1, .Ctr=1, .ReferentID2=0x0002004,
  .MaxBuffer=4000, .ReferentID3=0x00020008 };



