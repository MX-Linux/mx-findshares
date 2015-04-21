
/* Packet definitions */
/******************************************************************/
/* Packed data structure templates for network packets*/


struct Dummy		// Used to access the first two fields of any NFS structure
{
L Size;				// Size of any transmitted structure not counting this field
L XID;
} *dummy;

static struct PortrequestU
{
L Size;				// Size of this structure not counting this field
L XID;
L MessageType;		// Call=0 Reply=1
L RPCVersion;
L Portmap;
L PortmapVersion;
L GetPort;
L Credentials;
L Length;
L Verifier;
L Length2;
L Mount;
L MountVersion;
L ProtocolUDP;
L Port;
}__attribute__((__packed__)) portrequestU={ .XID=swap_32(0x2100464C),
  .RPCVersion=swap_32(2), .Portmap=swap_32(100000), .PortmapVersion=swap_32(2),
  .GetPort=swap_32(3), .Mount=swap_32(100005), .MountVersion=swap_32(3), .ProtocolUDP=swap_32(17),
  .Size=sizeof(struct PortrequestU) - 4 };

struct PortreplyU
{
L Size;				// Size of reply not counting this field
L XID;
L Message_Type;		// Call=0 Reply=1
L ReplyState;		// Accepted=0
L Verifier;
L Length2;
L AcceptState;		// Success=0
L Port;				// Port # of requested program, 0 if program not available
}__attribute__((__packed__)) *portreplyU;

static struct ExportrequestU
{
L Size;				// Size of this structure not counting this field
L XID;
L MessageType;		// Call=0 Reply=1
L RPCVersion;
L Mount;
L MountVersion;
L Export;
L Credentials;
L Length;
L Verifier;
L Length2;
}__attribute__((__packed__)) exportrequestU={ .XID=swap_32(0x2100464C),
  .RPCVersion=swap_32(2), .Mount=swap_32(100005), .MountVersion=swap_32(3),
  .Export=swap_32(5), .Size=sizeof(struct ExportrequestU) - 4 };

struct ExportreplyU
{
L Size;				// Size of reply not counting this field
L XID;
L Message_Type;		// Call=0 Reply=1
L ReplyState;		// Accepted=0
L Verifier;
L Length2;
L AcceptState;		// Success=0
L ListReturned;		// No NFSshares=0 NFSshares available=1
L FirstEntry;		// Contains the lenght of the first string
}__attribute__((__packed__)) *exportreplyU;


/******************************************************************/
