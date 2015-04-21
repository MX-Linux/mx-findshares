/* Templates to aid in decoding received packets */

#define STRINGLENGTH 1

struct RXwordcount10
{
// ----- NETBIOS HEADER
W sessionflags;			// 0
W length;				// # of bytes from next field to end of buffer
// ----- SMB HEADER
L ffsmb_string;			// 0xFF, 'S', 'M', 'B'
B transaction_command;	// 0x25
B error_class;			// 
B reserved;				// 0
W error_code;			//
B flags;
W flags2;
W pid_hi;				// 0
D signature;			// 0
W reserved2;			// 0
W tid;					// Tree ID, assigned by server
W pid;					// Process ID, assigned by us
W uid;					// User ID, assigned by server
W muxid;				// Multiplex ID, incremented between packets by us
// ----- Transaction Response
B wordcount;			// # of words between wordcount and bytecount
W totparamcnt;			// Total parameter bytes being sent
W totdatacnt;			// Total data bytes being sent
W reserved3;			// 0
W parametercount;		// Parameter bytes sent in this buffer
W parameteroffset;		// Offset (from ffsmb_string start) to Parameters
W parameterdisplacement;// Displacement of these Parameter bytes
W datacount;			// Data bytes sent in this buffer
W dataoffset;			// Offset (from ffsmb_string start) to data
W datadisplacement;		// Displacement of these bytes from dataoffset
B setupcount;			// # of words between here and bytecount
B padding;				// Padding for setupcount
W bytecount;			// # of bytes from next field to end of structure
B padding2;				// Pad DCE RPC Response to a long boundry
// ----- DCE RPC Response
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
B CancelCount;
B padding3;				// 0
// ----- Server Service, NetrShareEnum (NetShareEnumAll)
L Level;
L Ctr;
L ReferentID;
L Count;				// # of entries returned
L ReferentID2;
L MaxCount;				// Max # of entries available?
// The rest of this structure is just to illustrate how entries are layed out
// The next three longs repeats once for each entry returned
L ReferentID3;
L ShareType;			// Hidden, disk, printer, etc.
L ReferentID4;
// Now comes the actual share names and comments
// Starts at address &ReferentID3 + (Count * 12)
L MaxCount2;			// # of chars (words) in string including 0 terminator
L Offset;
L Count2;				// # of chars (words) in string including 0 terminator
W ShareName[STRINGLENGTH];		// Zero terminated Unicode
W padding4;				// Used to pad ShareName to a multiple of 4 bytes
L MaxCount3;			// # of chars (words) in string including 0 terminator
L Offset2;
L Count3;				// # of chars (words) in string including 0 terminator
W ShareComment[STRINGLENGTH];	// Zero terminated Unicode
W padding5;				// Used to pad ShareComment to a multiple of 4 bytes

}__attribute__((__packed__)) *RXNetrShareEnum;


#undef STRINGLENGTH
