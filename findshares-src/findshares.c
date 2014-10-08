/* findshares was written by and is Copyright (C) Richard A. Rost  April 23,2011
 * 
 * This program attempts to locate remote shares on Windows and Samba
 * servers as well as NFS servers in the same subnet as the local IP
 * address. Any remote shares found that are mounted in the local file
 * system are indicated.
 * 
 * It was written to be a lightweight alternative to Samba's smbtree
 * and a friendlier alternative to NFS's showmount.
 * 
 * My Copyright terms are simple. You may copy, modify, and use this
 * program as you see fit provided you meet the following terms:
 * 
 * 1. You leave my name as the author and do not take credit for my work.
 * 2. While I hope this program will be useful it is offered WITHOUT ANY
 *    WARRANTY and you agree not to hold me liable for any damages.
 * 3. You leave this header in it's entirety at the top of this and all
 *    other files for the original program. You may append notes, list
 *    changes, and add the authors of any changes to the end of this header.
 * 4. You do not collect a fee for this program as is or modified.
 * 5. You do not collect a fee for any changes you make to this program.
 * 6. You do not include or package it with any software for which a fee
 *    is collected.
 * 7. Items 4, 5, and 6 apply to the source code as well as the executable.
 * 8. You must make the full source code with a functioning compile script
 *    available in one of the following ways.
 *   A. Packaged with the executable when you distribute it.
 *   B. As a separate package available from where you distribute the
 *      executable. If distributed on a CD a web link to the source package
 *      is acceptable.
 * 
 * Thanks go out to:
 * SamK for his help in testing, debugging, formatting of output, and for
 * writing the text of the program's help message.
 * 
 * Information about the cifs, RAP, and RPC protocols as well as information
 * used to create the header files came from the following sources:
 * Implementing CIFS by Christopher R. Hertel at ubiqx.org.
 * Observing network traffic from Windows and Samba servers using Wireshark.
 * Information about some of the fields in network packets came from MSDN.
 * 
 * This program was written using the Geany 0.20 fast and lightweight IDE.
 * Tabs are set to 4.
 * -----------------------End of original header----------------------------
 */



#include <stdio.h>          /* These are the usual header files */
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
//#include <byteswap.h>
//#include <sched.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <net/if.h> // For IFF_LOOPBACK
#include <ifaddrs.h>
#include <arpa/inet.h>
//#include <sys/wait.h>
//#include <signal.h>
#include "packetdata.h"
#ifndef uchar
#define uchar unsigned char
#endif /* uchar */


/* Some defines */
/******************************************************************/
#define VERSION "1.06 Mar 11, 2014"
/* NBT offsets */
#define NBT_TID		0	// Transaction ID
#define NBT_MODE	2	// OpCode, flags, response code
#define NBT_QDCOUNT	4	//
#define NBT_ANCOUNT	6	//
#define NBT_NSCOUNT	8	//
#define NBT_ARCOUNT	10	//
#define NBT_DATA	12	//
#define MAXDIRS 64
#define LASTDIR MAXDIRS - 1
#define MAXUSERS 64
#define LASTUSER MAXUSERS - 1
#define MAXMOUNTS 8
#define LASTMOUNT MAXMOUNTS - 1
/******************************************************************/



/* Globals */
/******************************************************************/
int dbg=1;
int udp137, tcp_port, udpsocket; // Socket file handles
char myname[16];
uint32_t net_ipaddress;  /* Ipaddress in network byte order. */
uint32_t net_ipbase;  /* Base ipaddress in network byte order. */
ushort utid;  /* Unique Transaction ID source. */
//char addresslist[160]; /* Space for 10 ip addresses, 16 chars each */
struct NICstrings
{
	char ipaddress[16];
	char NICname[16];
};
struct NICstrings *NICinfo;

struct SMBshareinfo
{
	char name[84];
	char comment[64];
	char mountpoint[8][256];
	int mounted;
};
struct SMBserver
{
	struct SMBshareinfo share[64];
	char servercomment[64];
	char groupname[16];
	char servername[16];
	char ipaddress_string[16];
	int sharecount;
};
struct SMBserver **SMBshares;

struct NFSshareinfo
{
	char name[256];				// Remote mount point
	char user[MAXUSERS][64];	// Users in the Access Control List
	char mountpoint[MAXMOUNTS][256];	// Local mount point(s)
	int usercount;				// # of users found
	int mounted;				// # of times mounted locally
};
struct NFSserver
{
	struct NFSshareinfo *share[MAXDIRS];
	char ipaddress_string[16];
	int sharecount;				// # of NFSshares found
	int port;					// 
};
struct NFSserver **NFSshares;

/******************************************************************/



/* Prototypes */
/******************************************************************/
void bail(uint16_t ErrorLineNumber);
int CompareStrings(char *dest, char *src, int casesensitive);
int Copy(void *dest, void *src, int mode);
void CreateMountlistNFS();
void CreateMountlistSMB();
int EncodeName(char *src, char *dest);
uint16_t EnumShares(uint32_t IPAddress, ushort IPtid);
int FindnChar(void *dest, char find, int count);
void FindServer(uchar *rxbuffer, uint32_t ipaddress);
int32_t FindString(void *dest, void *src, int len, int casesensitive);
uint32_t GetNBStatusRequest(uchar *rxbuffer, int len);
void GetShareInfoNFS(void);
uint32_t GetUDP(void *rxbuffer, int len);
void Hexdump(void *src, int len);
void InitGlobals(void);
int OpenSocket(in_port_t portnum, int protocol);
int OpenTCPSocket(int portnum, uint32_t IPAddress, ushort IPtid);
void PrintDashes(void);
void ScanInterfaces(void);
void SendNBStatusRequest(const uint32_t IPAddress, uint32_t cmd);
int SendRecv(int socket_FD, void *txbuf, void *rxbuf, int rxlen);
void SendUDP(uint32_t IPAddress, int portnum, void *xmitdata);
int ToAscii(void *dest, void *src);
int ToUnicode(void *dest, void *src);
void *zmalloc(int size, int caller); // caller gets set to __LINE__

/******************************************************************/

/* Functions */
/******************************************************************/
/* Print error message(s) and exit program. */
void bail(uint16_t ErrorLineNumber)
{
	if(errno != 0)
	{
		fputs(strerror(errno), stdout);
		fputs(": ", stdout);
	}
    //printf("Error line %d\n", ErrorLineNumber);
	exit(1);
}
/******************************************************************/

/******************************************************************/
/* Compares src and dest strings. Returns 0 if equal, positive
 * number if dest > src, and negative number if dest < src.
 * If casesensitive is 0 the compare is not case sensitive. */
int CompareStrings(char *dest, char *src, int casesensitive)
{
	char *cdest=dest;
	char *csrc=src;

	if(casesensitive)
	{
		while(*csrc == *cdest)
		{
			if(*csrc == 0) break;
			csrc++; cdest++;
		}
		return(*cdest - *csrc);
	}
	else
	{
		while(toupper(*csrc) == toupper(*cdest))
		{
			if(*csrc == 0) break;
			csrc++; cdest++;
		}
		return(toupper(*cdest) - toupper(*csrc));
	}
}
/******************************************************************/

/******************************************************************/
int Copy(void *dest, void *src, int mode)
{
/* Copy bytes: dest, src, mode=number of bytes, returns number of bytes
 * Set bytes: dest, src=0-FF, mode=number of bytes, returns number of bytes
 * Copy string: dest, src, mode=0, returns length
 * Find char: dest=src, src, mode=char, returns offset of char or -1
 * String length: dest=src, src, mode=0, returns length
 * Length does not include zero terminator.
 * To Append s2 to s1 Copy(s1 + Copy(s1 ,s1 ,0), s2, 0);
 */
	char *csrc=src;
	char *cdest=dest;
	char find;
	int count;
	int direction;
	if(cdest == csrc)
	{// String length or Find char
		find=mode;
		mode=0;
		while(*csrc)
		{
			if(*csrc == find)
				return(mode);
			csrc++; mode++;
		}
		if(find) mode=-1; // Char not found
		return(mode);
	}
	count=mode;
	if(count == 0)
	{// convert Copy string to Copy bytes
		while(*(csrc + count) != 0)
			count++;
		mode=count;
		count++;
	}
	{// Copy bytes or Set bytes, safe for overlap copy
		if(cdest > csrc)
		{
			direction= -1;
			if(((int)csrc & -256) != 0) csrc+=(count - 1);
			cdest+=count - 1;
		}
		else direction=1;
		while(count != 0)
		{
			if(((int)csrc & -256) == 0)
			{
				*cdest=((int)csrc & 255);
			}
			else
			{
				*cdest=*csrc;
				csrc+=direction;
			}
			cdest+=direction; count--;
		}
	}
	return(mode);
}
/******************************************************************/

/******************************************************************/
void CreateMountlistNFS()
{
	char linebuffer[1024], servername[16], mountpoint[256], sharename[256];
	FILE *fp;
	ushort subnet;
	int i, j, offset;
	
	if((fp=fopen("/etc/mtab", "r")) == NULL) return;

	while(fgets(linebuffer, 1024, fp))
	{
		if(FindString(linebuffer, " nfs ", 0, 0) != -1)
		{
			offset=FindnChar(linebuffer, ':', 1); // End of server name
			Copy(servername, linebuffer, offset);
			servername[offset]=0;
			offset++; // Start of share name
			i=(FindString(linebuffer + offset, " /", 0, 0) & 0xFFFF); // End of share name
			Copy(sharename, linebuffer + offset, i);
			sharename[i]=0;
			offset=offset + i + 1; // Start of mount point
			i=FindnChar(linebuffer + offset, ' ', 1); // End of mount point
			Copy(mountpoint, linebuffer + offset, i);
			mountpoint[i]=0;
			for(subnet=1; subnet < 255; subnet++)
			{
				if(NFSshares[subnet] != NULL)
					if(CompareStrings(servername, NFSshares[subnet]->ipaddress_string, 0) == 0)
					{
						i=NFSshares[subnet]->sharecount - 1; // -1 because array starts at 0
						while(i >= 0)
						{
							if(CompareStrings(sharename, NFSshares[subnet]->share[i]->name, 0) == 0)
							{
								if(NFSshares[subnet]->share[i]->mounted < 8)
								{
									j=NFSshares[subnet]->share[i]->mounted;
									NFSshares[subnet]->share[i]->mounted++;
									Copy(NFSshares[subnet]->share[i]->mountpoint[j], mountpoint, 0);
								}
							}
							i--;
						}
					}
			}
		}
	}
	fclose(fp);
	return;
}
/******************************************************************/

/******************************************************************/
void CreateMountlistSMB()
{
	char linebuffer[1024], servername[16], mountpoint[256], sharename[256];
	FILE *fp;
	ushort subnet;
	int i, j, offset;
	
	if((fp=fopen("/etc/mtab", "r")) == NULL) return;

	while(fgets(linebuffer, 1024, fp))
	{
		if(FindString(linebuffer, " cifs ", 0, 0) != -1)
		{
			offset=FindnChar(linebuffer + 2, '/', 1); // End of server name
			Copy(servername, linebuffer + 2, offset); // Skip the leading /'s
			servername[offset]=0;
			offset+=3; // Start of share name
			i=(FindString(linebuffer + offset, " /", 0, 0) & 0xFFFF); // End of share name
			Copy(sharename, linebuffer + offset, i);
			sharename[i]=0;
			if(sharename[i - 1] == '/')
				sharename[i - 1]=0;
			offset=offset + i + 1; // Start of mount point
			i=FindnChar(linebuffer + offset, ' ', 1); // End of mount point
			Copy(mountpoint, linebuffer + offset, i);
			mountpoint[i]=0;
			for(subnet=1; subnet < 255; subnet++)
			{
				if(SMBshares[subnet] != NULL)
					if((CompareStrings(servername, SMBshares[subnet]->ipaddress_string, 0) == 0) ||
						(CompareStrings(servername, SMBshares[subnet]->servername, 0) == 0))
					{
						i=SMBshares[subnet]->sharecount - 1; // -1 because array starts at 0
						while(i >= 0)
						{
							if(CompareStrings(sharename, SMBshares[subnet]->share[i].name, 0) == 0)
							{
								if(SMBshares[subnet]->share[i].mounted < 8)
								{
									j=SMBshares[subnet]->share[i].mounted;
									SMBshares[subnet]->share[i].mounted++;
									Copy(SMBshares[subnet]->share[i].mountpoint[j], mountpoint, 0);
								}
							}
							i--;
						}
					}
			}
		}
	}
	fclose(fp);
	return;
}
/******************************************************************/

/******************************************************************/
/* Encode a 15 char '\0' terminated name into a 34 byte string ("LEVEL 2").
   Byte 1 = length (allways 32). Bytes 2-33 = Encoded Name. Byte 34 = '\0'.
   Pad with spaces if required, truncate if too long. */
int EncodeName(char *src, char *dest)
{
	int i, j=1;
	char C_or_A='C';
	uchar c;
	dest[0]=32; /* Length of string not counting terminater. */
	for(i=0; (i < 15) && (src[i] != '\0'); i++) /* Encode Name and truncate */
	{
		c=toupper(src[i]);
		dest[j]=((c >> 4) & 0x0F) + 'A';
		dest[j+1]=(c & 0x0F) + 'A';
		j=j+2;
	}
/* Pad with spaces. Unless first character='*', then pad with zeros */
	if(src[0]=='*') C_or_A='A';
	while(j < 33)
	{
		dest[j]=C_or_A;
		dest[j+1]='A';
		j=j+2;
	}
	dest[j]='\0';
	return(j + 1); // Encoded name is always 34 characters
}
/******************************************************************/

/******************************************************************/
/* Find available shares using the old ENUMSHARES SMB call. */
uint16_t EnumShares(uint32_t IPAddress, ushort IPtid)
{
	int tmp, i, j, port=445;
	uint16_t offset;
	uint16_t *poff;
	uint16_t *coff;
	char *doff;
	uint convert;
	ushort tid=IPtid;
	uchar rxbuffer[4096];

	tcp_port=OpenTCPSocket(port, IPAddress, tid);
	if(tcp_port < 0)
	{// Port 445 failed so try port 139, session request needed
		port=139;
		close(tcp_port);
		tcp_port=OpenTCPSocket(port, IPAddress, tid);
		if(tcp_port < 0) return(__LINE__);
	 //-------Session request NetBios
		EncodeName(SMBshares[tid]->servername, sessionreq.callee); // servername<20>
		EncodeName(myname, sessionreq.caller); // myname<20>
		sessionreq.caller[31]='A'; // and now it's myname<00>
		tmp=SendRecv(tcp_port, &sessionreq, rxbuffer, sizeof(rxbuffer));
		if(tmp < 0) return(__LINE__);
		if(rxbuffer[0] != 0x82) return(__LINE__);
	}
//-------Negotiate protocol request SMB
	protocol.pid=tid;
	tmp=SendRecv(tcp_port, &protocol, rxbuffer, sizeof(rxbuffer));
	if(tmp < 0) return(__LINE__);
//-------Session setup AndX request SMB Guest
	setup.pid=tid;
	setup.vcnumber=tid;
	setup.sessionkey=*(uint64_t*)&rxbuffer[52];
	setup.ansipasswordlen=1;
	setup.ansipassword=0;
	offset=Copy(&setup.strings[0], "Guest", 0) + 1;
	offset+=Copy(&setup.strings[offset], SMBshares[tid]->groupname, 0) + 1;
	offset+=Copy(&setup.strings[offset], "Unix\0Samba", 11);
	setup.bytecount=offset; // Length of the strings field
	offset=62 + offset; // SMB message length
	setup.length=((offset << 8) & 0xFF00) | ((offset >> 8) & 0x00FF);
	tmp=SendRecv(tcp_port, &setup, rxbuffer, sizeof(rxbuffer));
	if(tmp < 0)
	{
//-------Session setup AndX request SMB Anonymous
// Since ansipasswordlen==0 we have to move the strings field to start
// at the ansipassword field instead. We also add one byte less to offset.
		setup.ansipasswordlen=0;
		offset=Copy(&setup.ansipassword, 0, 2);
		offset+=Copy(&setup.ansipassword + offset, "Unix\0Samba", 11);
		setup.bytecount=offset;
		offset=61 + offset; // SMB message length
		setup.length=((offset << 8) & 0xFF00) | ((offset >> 8) & 0x00FF);
		tmp=SendRecv(tcp_port, &setup, rxbuffer, sizeof(rxbuffer));
		if(tmp < 0) return(__LINE__);
	}

//-------Tree connect AndX request SMB
	treeconn.pid=tid;
	treeconn.uid=*(uint16_t*)&rxbuffer[32];
	// strings is preloaded with a double backslash, that's why we start at 2
	offset=Copy(&treeconn.strings[2], SMBshares[tid]->servername, 0) + 2;
	// Append "\\IPC$" to server name
	offset+=Copy(treeconn.strings + offset,  "\\IPC$", 0) + 1;
	offset+=Copy(treeconn.strings + offset, "IPC", 0) + 1;
	treeconn.bytecount=offset + 1;
	offset=44 + offset; // SMB message length
	treeconn.length=((offset << 8) & 0xFF00) | ((offset >> 8) & 0x00FF);
	tmp=SendRecv(tcp_port, &treeconn, rxbuffer, sizeof(rxbuffer));
	if(tmp < 0) return(__LINE__);

	if(port == 445) // MS NetrShareEnum
	{
//-------NT create AndX request SMB

		NTcreate.tid=*(uint16_t*)&rxbuffer[28]; // Tree ID
		NTcreate.pid=tid; // PID
		NTcreate.uid=*(uint16_t*)&rxbuffer[32]; // User ID
		tmp=SendRecv(tcp_port, &NTcreate, rxbuffer, sizeof(rxbuffer));
		if(tmp < 0) return(__LINE__);

		RPCbind.tid=*(uint16_t*)&rxbuffer[28]; // Tree ID
		RPCbind.pid=tid; // PID
		RPCbind.uid=*(uint16_t*)&rxbuffer[32]; // User ID
		RPCbind.FID=*(uint16_t*)&rxbuffer[42]; // File ID
		i=RPCbind.FID; // Save FID for the next sequence
		tmp=SendRecv(tcp_port, &RPCbind, rxbuffer, sizeof(rxbuffer));
		if(tmp < 0) return(__LINE__);
		if(rxbuffer[62] != 0x0C) return(__LINE__);

		NetrShareEnum.tid=*(uint16_t*)&rxbuffer[28]; // Tree ID
		NetrShareEnum.pid=tid; // PID
		NetrShareEnum.uid=*(uint16_t*)&rxbuffer[32]; // User ID
		NetrShareEnum.FID=i; // File ID returned by RPCbind
		i=ToUnicode(NetrShareEnum.ServerUnc, SMBshares[tid]->servername) + 1;
		NetrShareEnum.MaxCount=i; // # of Unicode characters
		NetrShareEnum.ActualCount=i;
		i=i << 1; // Convert # of Unicode charaters to # of bytes
		if(i & 2)
			i=i + 2; // Unicode string must end on long boundry
		NetrShareEnum.totdatacnt=i + 72;
		NetrShareEnum.datacount=NetrShareEnum.totdatacnt;
		NetrShareEnum.FragLength=NetrShareEnum.totdatacnt;
		NetrShareEnum.bytecount=NetrShareEnum.totdatacnt + 7;
		NetrShareEnum.AllocHint=i + 48;
		Copy(&NetrShareEnum.ServerUnc[i],&NetrShareEnum.Level, 32);
		i=NetrShareEnum.dataoffset + NetrShareEnum.bytecount;
		NetrShareEnum.length=((i << 8) & 0xFF00) | ((i >> 8) & 0x00FF);
		tmp=SendRecv(tcp_port, &NetrShareEnum, rxbuffer, sizeof(rxbuffer));
		if(tmp < 0) return(__LINE__);
		if(rxbuffer[62] != 0x02) return(__LINE__);

		RXNetrShareEnum=(struct RXwordcount10 *)rxbuffer;
		if(RXNetrShareEnum->datacount < RXNetrShareEnum->totdatacnt)
		{ // Share list required multiple packets, append them to rxbuffer
			int receivedbytes=RXNetrShareEnum->datacount;
			char auxbuffer[4096];
			struct RXwordcount10 *auxbuf=(struct RXwordcount10 *)auxbuffer;
			doff=(char *)&RXNetrShareEnum->ffsmb_string +
								RXNetrShareEnum->dataoffset;
			while(receivedbytes < RXNetrShareEnum->totdatacnt)
			{
				tmp=SendRecv(tcp_port, NULL, auxbuffer, sizeof(auxbuffer));
				if(tmp < 0) return(__LINE__);
				Copy(doff + auxbuf->datadisplacement,
						(char *)&auxbuf->ffsmb_string + auxbuf->dataoffset,
														auxbuf->datacount);
				receivedbytes+=auxbuf->datacount;
			}
		}
		SMBshares[tid]->sharecount=RXNetrShareEnum->Count; // # of shares
		i=RXNetrShareEnum->Count - 1;
		doff=(char *)&RXNetrShareEnum->ShareName + (i * 12); // Location of first share name
		j=0;
		while(i >= 0)
		{
			j=ToAscii(&SMBshares[tid]->share[i].name, doff) + 1;
			j=j << 1; // Convert # of characters to # of bytes
			if(j & 2)
				j=j + 2; // Everythings aligned on long boundries
			doff=doff + j + 12; // 12 bytes between strings
			j=ToAscii(&SMBshares[tid]->share[i].comment, doff) + 1;
			j=j << 1; // Convert # of characters to # of bytes
			if(j & 2)
				j=j + 2; // Everythings aligned on long boundries
			doff=doff + j + 12; // 12 bytes between strings
			i--;
		}
	} // End MS NetrShareEnum
	else // MS NetShareEnum
	{
//-------Net share enumerate request SMB
		enumshares.tid=*(uint16_t*)&rxbuffer[28]; // Tree ID
		enumshares.pid=tid; // PID
		enumshares.uid=*(uint16_t*)&rxbuffer[32]; // User ID
		tmp=SendRecv(tcp_port, &enumshares, rxbuffer, sizeof(rxbuffer));
		if(tmp < 0) return(__LINE__);

/* Bytes 0-3 are the netbios header. Bytes 4-35 are the SMB header.
   Bytes 51-52=DataOffset word (lowbyte/highbyte), it's the distance
   from byte 4 to the start of the share list. Share records are 20 bytes
   long broken down as follows. Name+0 13, pad=0, share type 2, Description
   offset 2, unused 2. Description strings are found by subtracting Convert
   from Description offset. */
// Parameter offset=Distance from start of SMB header to Status
		poff=(uint16_t *)(rxbuffer + rxbuffer[45] + 4);
// Data offset=Distance from start of SMB header to start of share list
		doff=(char *)(rxbuffer + rxbuffer[51] + 4);
// Comment offset=Distance from start of share list to comment. It's
// a word located 16 bytes past the start of a share name. You have to
// subtract convert from it to get the actual distance.
		coff=(uint16_t *)(doff + 16);
		convert=*(poff + 1); // Used to calculate the start of a shares comment
		i=*(poff + 2); // Number of shares
		SMBshares[tid]->sharecount=i;
		j=0;
		while(i > 0)
		{
	 //printf("%-14s%s\n", doff + j, doff + (*(coff + (j / 2)) - convert));
			Copy(SMBshares[tid]->share[i - 1].name, doff + j, 0);
			Copy(SMBshares[tid]->share[i - 1].comment, doff + (*(coff + (j / 2)) - convert), 0);
			i--; j=j + 20;
		}
	} // End MS NetShareEnum

//-------Net server getinfo request SMB
// This call is not supported by Samba
	netservergetinfo.tid=*(uint16_t*)&rxbuffer[28]; // Tree ID
	netservergetinfo.pid=tid; // PID
	netservergetinfo.uid=*(uint16_t*)&rxbuffer[32]; // User ID
	tmp=SendRecv(tcp_port, &netservergetinfo, rxbuffer, sizeof(rxbuffer));
	if(tmp < 0) return(__LINE__);
// Parameter offset=Distance from start of SMB header to Status
	poff=(uint16_t *)(rxbuffer + rxbuffer[45] + 4);
	i=1; // # of entries is always 1, used by  while loop  below
	
	if(*poff != 0x0000) // Check for bad status
	{
//-------Net server enumerate2 request SMB
// We use this just to get the server comment
		enumservers2.tid=*(uint16_t*)&rxbuffer[28]; // Tree ID
		enumservers2.pid=tid; // PID
		enumservers2.uid=*(uint16_t*)&rxbuffer[32]; // User ID
		i=Copy(enumservers2.workgroup, SMBshares[tid]->groupname, 0);
		doff=enumservers2.workgroup + i + 1; // Points to end of structure + 1
		enumservers2.totparamcnt=doff - (char *)&enumservers2.functioncode;
		enumservers2.parametercount=enumservers2.totparamcnt;
		enumservers2.dataoffset=doff - (char *)&enumservers2.ffsmb_string;
		enumservers2.bytecount=doff - (char *)&enumservers2.transaction_name;
		offset=enumservers2.dataoffset;
		enumservers2.length=((offset << 8) & 0xFF00) | ((offset >> 8) & 0x00FF);
		tmp=SendRecv(tcp_port, &enumservers2, rxbuffer, sizeof(rxbuffer));
		if(tmp < 0) return(__LINE__);
// Parameter offset=Distance from start of SMB header to Status
		poff=(uint16_t *)(rxbuffer + rxbuffer[45] + 4);
		if(*poff != 0x0000) return(__LINE__);// Check for bad status
		i=*(poff + 2);
		if(i == 0x0000) return(__LINE__);// Check number of entries
	}
	convert=*(poff + 1); // Used to calculate the start of a shares comment
// Data offset=Distance from start of SMB header to start of share list
	doff=(char *)(rxbuffer + rxbuffer[51] + 4);
	poff=(uint16_t*)doff; // Save pointer to start of share list
// Comment offset=Distance from start of share list to comment. It's
// a word located 22 bytes past the start of a share name. You have to
// subtract convert from it to get the actual distance.
	while(i > 0)
	{
		j=0;
		while(*(doff + j) == SMBshares[tid]->servername[j])
		{
			if(*(doff + j) == 0)
			{
				i=0;
				break;
			}
			j++;
		}
		if(i)
			doff=doff + 26; // Advance to next name
		i--;
	}
	if(i == 0x0000) return(__LINE__);// Server name not found
	coff=(uint16_t *)(doff + 22);
	Copy(SMBshares[tid]->servercomment, (char *)poff + (*(coff) - convert), 0);
	return(0);

}
/******************************************************************/

/******************************************************************/
/* Searches dest count times for char find. If char is found count
 * times it returns offset of last find, otherwise it returns -1. */
int FindnChar(void *dest, char find, int count)
{
	char *cdest=dest;
	int offset=0;
	while(count)
	{
		if((offset=Copy(cdest, cdest, find)) == -1) return(-1);
		count--;
		cdest+=offset + 1;
	}
	cdest--;
	return(cdest - (char *)dest);
}
/******************************************************************/

/******************************************************************/
void FindServer(uchar *rxbuffer, uint32_t ipaddress)
{
	int s, e, ptr=57; // points to first name in the list.
	int flags;
	int namecounter=rxbuffer[ptr - 1] - 1; // Number of names in list - 1.
	uchar subnet=(ipaddress >> 24) & 0xFF;
	if(SMBshares[subnet] == NULL)
		SMBshares[subnet]=zmalloc(sizeof(struct SMBserver), __LINE__);
	for( ; namecounter >= 0; namecounter--)
	{
		s=ptr + (namecounter * 18); // Points to start of current name.
		e=s + 15; // Points to end of current name.
		flags=e + 1;
		if(rxbuffer[e]=='\0')
		{
			while(rxbuffer[e - 1] == ' ') // Strip trailing spaces
				e--;
			rxbuffer[e]='\0'; // Terminate string
			if((rxbuffer[flags] & 0x80) == 0)
				Copy(SMBshares[subnet]->servername, &rxbuffer[s], 0);
			else
				Copy(SMBshares[subnet]->groupname, &rxbuffer[s], 0);
		}
	}
	struct sockaddr_in tmp;
	tmp.sin_addr.s_addr=ipaddress;
	Copy(SMBshares[subnet]->ipaddress_string, inet_ntoa(tmp.sin_addr), 0);
	return;
}
/******************************************************************/

/******************************************************************/
/* Scans dest to see if contains src. If len==n then use first n
 * bytes of src, if len==0 then use all of src using \0 terminator.
 * If casesensitive is 0 the search is not case sensitive.
 * Returns ((lenght << 16) & offset) if found, -1 if not found. */
int32_t FindString(void *dest, void *src, int len, int casesensitive)
{
	char *find=src;
	char *cdest=dest;
	int findlen, destlen, i, j;
	
	findlen=len;
	if(findlen == 0)
		findlen=Copy(find, find, 0); // Get string length
	destlen=Copy(cdest, cdest, 0) - findlen;
	for(i=0; i <= destlen; i++) // This should catch destlen < findlen
	{
		j=0;
		if(casesensitive)
		{
			while(*(cdest + i + j) == *(find + j))
			{
				j++;
				if(j == findlen)
					return(i + (findlen << 16));
			}
		}
		else
		{
			while(toupper(*(cdest + i + j)) == toupper(*(find + j)))
			{
				j++;
				if(j == findlen)
					return(i + (findlen << 16));
			}
		}
	}	
	return(-1);
}
/******************************************************************/

/******************************************************************/
/* Checks to see if anyone answered a status request message. Returns
 * the ip address of the machine or 0 if no answers are pending. Fills
 * rxbuffer with answer. */

uint32_t GetNBStatusRequest(uchar *rxbuffer, int len)
{
	int tmp;
	socklen_t reply_ip_len;
	struct sockaddr_in reply_ip;
	struct timeval tv;
	tv.tv_sec=0;
	tv.tv_usec=100;
	tmp=setsockopt(udp137, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	if(tmp < 0) bail(__LINE__);
	Copy(rxbuffer, 0, len);
	Copy(&reply_ip, 0, sizeof(reply_ip));
	reply_ip_len=sizeof(reply_ip);
	tmp=recvfrom(udp137, rxbuffer, len, 0, (struct sockaddr *)&reply_ip, &reply_ip_len);
	if(tmp <= 0) return(0);
	return((uint32_t)reply_ip.sin_addr.s_addr);
}
/******************************************************************/

/******************************************************************/
/* Finds NFS servers and retrieves their shares */
void GetShareInfoNFS(void)
{
	int i, j, k, timeout=3;
	ushort subnet;
	const int len=4096;
	uint8_t rxbuffer[len];
	uint32_t *rxdata;
	struct in_addr validIP;
	struct PortreplyU *preplyU=(struct PortreplyU *)&rxbuffer;
	struct ExportreplyU *xreplyU=(struct ExportreplyU *)&rxbuffer;

	udpsocket=OpenSocket(0, SOCK_DGRAM);
	portrequestU.XID=1024;
	SendUDP(net_ipbase | (255 << 24), 111, &portrequestU); // Broadcast port request
	validIP.s_addr=1;
	while(timeout) // Check for responding computers
	{
		if(validIP.s_addr == 0) // Receive timed out
		{
			usleep(100000);
			timeout--;
		}
		validIP.s_addr=GetUDP(rxbuffer, len);
		subnet=(validIP.s_addr >> 24) & 0xFF;
		if(preplyU->XID == 1024) // Broadcast reply
		{
			if(NFSshares[subnet] == NULL)
				NFSshares[subnet]=zmalloc(sizeof(struct NFSserver), __LINE__);
			Copy(NFSshares[subnet]->ipaddress_string, inet_ntoa(validIP), 0);
			NFSshares[subnet]->port=htonl(preplyU->Port);
			exportrequestU.XID=subnet | ((NFSshares[subnet]->port << 16) & 0xFFFF0000);
			if(NFSshares[subnet]->port != 0)
				SendUDP(validIP.s_addr, NFSshares[subnet]->port, &exportrequestU);
		}
		else if(validIP.s_addr != 0) // Export reply
		{
			if(xreplyU->ListReturned != 0)
			{
				j=0;
				rxdata=&xreplyU->FirstEntry; // Length of first directory string
				while(j < MAXDIRS)
				{
					NFSshares[subnet]->share[j]=zmalloc(sizeof(struct NFSshareinfo), __LINE__);
					i=ntohl(*rxdata); // Length of string
					i=Copy(NFSshares[subnet]->share[j]->name, rxdata + 1, i);
					NFSshares[subnet]->sharecount++;
					i+=((4 - (i & 3)) & 3); // Round up to 4 byte boundry
					rxdata=rxdata + (i >> 2) + 1;
					k=0;
					while(*rxdata)
					{
						rxdata++;
						i=ntohl(*rxdata); // Length of string
						if(k < MAXUSERS) // Copy user to structure if there's room
						{
							Copy(NFSshares[subnet]->share[j]->user[k], rxdata + 1, i);
							NFSshares[subnet]->share[j]->usercount++;
							k++;
						} // But keep parsing users to get to next directory
						i+=((4 - (i & 3)) & 3); // Round up to 4 byte boundry
						rxdata=rxdata + (i >> 2) + 1; // End of user list flag
					}
					j++;
					rxdata++; // End of directory list flag
					if(*rxdata == 0) break;
					rxdata++; // Length of next directory string
				}
			}
			timeout=3;
		}
	}
	close(udpsocket);
}
/******************************************************************/

/******************************************************************/
/* Finds Samba/Windows servers and retrieves their shares */
void GetShareInfoSMB(void)
{
	int tmp, timeout=3;
	uint32_t validIP;
	ushort subnet;
	const int len=512;
	uchar rxbuffer[len];

	udp137=OpenSocket(0, SOCK_DGRAM);
	SendNBStatusRequest(net_ipbase | 0xFF000000, 0x20); //Broadcast status request
	validIP=1;
	while(timeout) // Check for responding computers
	{
		if(validIP == 0)
		{
			usleep(100000);
			timeout--;
		}
		validIP=GetNBStatusRequest(rxbuffer, len); // Returns zero if no reply pending
		if(rxbuffer[1] == 0xFF) // Reply from broadcast
		{
			SendNBStatusRequest(validIP, 0x21); // Send name query to specific PC
		}
		else if(validIP) // Reply from name query
		{// validIP contains ip address of response in network order
			FindServer(rxbuffer, validIP); // Parse rxbuffer for server and group name
			timeout=3; // Reset timeout and see if more computers respond
		}
	}
	for(subnet=1; subnet < 255; subnet++)
	{
		if(SMBshares[subnet] != NULL)
		{
			tmp=EnumShares(net_ipbase, subnet);
            //if(tmp != 0) printf("Error line %d\n", tmp);
			close(tcp_port); // EnumShares opened it and we close it
		}
	}
}
/******************************************************************/

/******************************************************************/
/* Checks to see if anyone responded to a request message. Returns
 * the ip address of the machine or 0 if no answers are pending. Fills
 * rxbuffer with reply. */

uint32_t GetUDP(void *rxbuffer, int len)
{
	int tmp;
	dummy=rxbuffer;
	socklen_t reply_ip_len;
	struct sockaddr_in reply_ip;
	struct timeval tv;
	tv.tv_sec=0;
	tv.tv_usec=10000;
	tmp=setsockopt(udpsocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	if(tmp < 0) bail(__LINE__);
	Copy(rxbuffer, 0, len);
	Copy(&reply_ip, 0, sizeof(reply_ip));
	reply_ip_len=sizeof(reply_ip);
	tmp=recvfrom(udpsocket, &dummy->XID, len, 0, (struct sockaddr *)&reply_ip, &reply_ip_len);
	if(tmp <= 0) return(0);
	dummy->Size=tmp;
	return(reply_ip.sin_addr.s_addr);
}
/******************************************************************/

/******************************************************************/
/* Print a hex and ASCII dump of the src to the screen. The outer
   loop counts 16 character lines, the inner one counts characters.
   Either loop will bail once end of buffer is reached. Len does
   not have to be a multiple of 16. */
void Hexdump(void *src, int len)
{
	const char xlat[]="0123456789ABCDEF"; // Nibble to ASCII translation table.
	int remainder=len;
	int i, j, x;
	uchar buffer[80], tmp;
	char *csrc=src;

	for(j=0; remainder > 0; j=j + 16)
	{
		Copy(buffer, (char *)' ', sizeof(buffer)); // Fill buffer with spaces
		for(i=0; i < 16; i=i + 1)
		{
			if(remainder <= 0) break; // Test for a short line
			x=3 * i;
			tmp=csrc[i+j];
			buffer[x]=xlat[((tmp >> 4) & 0x0F)];
			buffer[x + 1]=xlat[((tmp) & 0x0F)];
			if((tmp < 20)||(tmp > 127)) tmp='.'; // Replace unprintable chars.
			buffer[i + 50]=tmp;
			remainder--;
		}
		buffer[i + 55]='\0'; // Add terminator for puts.
		puts((char *)buffer);
	}
	fputc('\n', stdout);
	return;
}
/******************************************************************/

/******************************************************************/
void InitGlobals(void)
{
	if(SMBshares == NULL)
		SMBshares=zmalloc(sizeof(struct SMBserver *) * 256, __LINE__);
	if(NFSshares == NULL)
		NFSshares=zmalloc(sizeof(struct server *) * 256, __LINE__);
	if(NICinfo == NULL)
		NICinfo=zmalloc(sizeof(struct NICstrings) * 10, __LINE__);
}
/******************************************************************/

/******************************************************************/
/* Open a socket, bind to a port and return a handle. If portnum==0
   then Linux will assign a free port automatically. */
int OpenSocket(in_port_t portnum, int protocol)
{
	const int Yes=1;
	int sock_fd, tmp;
	struct sockaddr_in myipaddr;
	Copy(&myipaddr, 0, sizeof(myipaddr));
	myipaddr.sin_family=AF_INET;
	myipaddr.sin_addr.s_addr=net_ipaddress;
	myipaddr.sin_port=htons(portnum);
	sock_fd=socket(AF_INET, protocol, 0);
	if(sock_fd < 0) bail(__LINE__);
	tmp=setsockopt(sock_fd, SOL_SOCKET, SO_BROADCAST, &Yes, sizeof(Yes));
	tmp=setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &Yes, sizeof(Yes));
	tmp=setsockopt(sock_fd, SOL_SOCKET, SO_DONTROUTE, &Yes, sizeof(Yes));
/* fcntl, sysctl? SO_BROADCAST SO_KEEPALIVE?,SO_REUSEADDR,TCP_NODELAY */
	tmp=bind(sock_fd, (struct sockaddr *) &myipaddr, sizeof(myipaddr));
	if(tmp < 0) bail(__LINE__);
	return(sock_fd);
}
/******************************************************************/

/******************************************************************/
/* Open a socket and return a handle. */
int OpenTCPSocket(int portnum, uint32_t IPAddress, ushort IPtid)
{
	int socket_fd, tmp;
	struct sockaddr_in nameservice;
	ushort tid=IPtid;
	struct timeval tv;
	tv.tv_sec=0;
	tv.tv_usec=100000;

	socket_fd=socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
	setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
/* Setup some network parameters */
	Copy(&nameservice, 0, sizeof(struct sockaddr_in));
	nameservice.sin_family = AF_INET;
	nameservice.sin_addr.s_addr=IPAddress | (tid << 24);  /* Convert sub address.*/
	nameservice.sin_port= htons(portnum);
	tmp=connect(socket_fd, (struct sockaddr *)&nameservice, sizeof(nameservice));
	if(tmp < 0) socket_fd=-1;
	return(socket_fd);
}
/******************************************************************/

/******************************************************************/
void PrintDashes(void)
{
	int i=80;
	while(i)
	{
		fputc('-', stdout);
		i--;
	}
	fputc('\n', stdout);
}
/******************************************************************/

/******************************************************************/
/* Scans NIC(s) to retrieve machines IP address(es). */

void ScanInterfaces(void)
{
	struct ifaddrs *ifaddr, *ifa;
	int s, ptr=0;

	if (getifaddrs(&ifaddr) == -1) bail(__LINE__);

// Walk through linked list, save pointer for freeifaddrs()

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
	{
		if((ifa->ifa_addr == NULL) || (ptr >= 10)) break;
		if((ifa->ifa_addr->sa_family == AF_INET) && !(ifa->ifa_flags & IFF_LOOPBACK))
		{ /* INET yes, LOOPBACK no. */
			s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
				NICinfo[ptr].ipaddress, 16, NULL, 0, NI_NUMERICHOST);
			Copy(&NICinfo[ptr].NICname, ifa->ifa_name, 15);
			NICinfo[ptr].NICname[15]='\0'; // Clip name if more than 15 characters
			ptr++;
		}
	}
	freeifaddrs(ifaddr);
}
/******************************************************************/

/******************************************************************/
/* Sends a status request to IPAddress so we can get the machines
   BIOS name, group name, and if it is a fileserver. IPAddress is
   in the form of 192.168.1.0 with 0 being replaced with tid. The
   structure statreq is preloaded and we only change tid. */

void SendNBStatusRequest(const uint32_t IPAddress, uint32_t cmd)
{
	struct sockaddr_in nameservice;
	int tmp;
	statreq.tid=(IPAddress >> 16) & 0xFF00;
	statreq.nbstat=(cmd << 8) & 0xFF00;
/* Setup some network parameters */
	nameservice.sin_family = AF_INET;
	nameservice.sin_addr.s_addr=IPAddress;
	nameservice.sin_port= htons( 137 );
	tmp=sendto(udp137, &statreq, 50, 0, (struct sockaddr *)&nameservice, sizeof(struct sockaddr));
	if(tmp < 0) bail(__LINE__);
//Hexdump(&statreq, 50);
//if(tmp < 50) printf("Bytes sent=%d  tid=%d\n", tmp, tid);
}
/******************************************************************/

/******************************************************************/
int SendRecv(int socket_FD, void *txbuf, void *rxbuf, int rxlen)
{
	uchar *txbuffer=txbuf;
	uchar *rxbuffer=rxbuf;
	int tmp, bytecount=-1, length, retry=15;

	if(txbuffer)
	{
		bytecount=0;
		length=txbuffer[3] + ((txbuffer[2] << 8) & 0xFF00) + 4;
		while(bytecount < length)
		{
			tmp=send(socket_FD, txbuffer + bytecount, length - bytecount, 0);
			if((tmp < 0) && (errno != EWOULDBLOCK))
				return(-2); // Any error except Timeout
			if(tmp > 0)
				bytecount=bytecount + tmp;
		}
	}
	if(rxbuffer)
	{
		Copy(rxbuffer, 0, rxlen);
		bytecount=0;
		length=4;
		while(1)
		{
			tmp=recv(socket_FD, rxbuffer + bytecount, length - bytecount, 0);
			if((tmp < 0) && (errno != EWOULDBLOCK))
				return(-3); // Any error except Timeout
			if(tmp == 0)
				return(-4); // Other end closed the socket
			if(tmp > 0)
				bytecount=bytecount + tmp;
			if(bytecount == 4) // Get packet length from netBIOS header
				length=rxbuffer[3] + ((rxbuffer[2] << 8) & 0xFF00) + 4;
			if(bytecount == length) break;
			retry--;
			if(retry == 0)
				return(-5); // Timed out
		}
		if(*(uint32_t*)&rxbuffer[9] != 0x0000)
			return(-6); // DOS or WinNT error
	}
	return(bytecount);
}
/******************************************************************/

/******************************************************************/
/* Sends a UDP packet to IPAddress at portnum. We prepended a Size
 * field to the data structures. The actual data to be sent starts
 * at the XID field. The dummy pointer is just a convenient way
 * to access those two fields */

void SendUDP(uint32_t IPAddress, int portnum, void *xmitdata)
{
	struct sockaddr_in nameservice;
	int tmp;
	dummy=xmitdata;
/* Setup some network parameters */
	nameservice.sin_family = AF_INET;
	nameservice.sin_addr.s_addr=IPAddress;
	nameservice.sin_port= htons(portnum);
	tmp=sendto(udpsocket, &dummy->XID, dummy->Size, 0, (struct sockaddr *)&nameservice, sizeof(nameservice));
	if(tmp < 0) bail(__LINE__);
//Hexdump(&statreq, 50);
//if(tmp < 50) printf("Bytes sent=%d  tid=%d\n", tmp, tid);
}
/******************************************************************/

/******************************************************************/
int ToAscii(void *dest, void *src)
{
	char *csrc=src;
	char *cdest=dest;
	
	while(*csrc)
	{
		*cdest=*csrc;
		cdest++; csrc+=2;
	}
	*cdest='\0';
	return(cdest - (char *)dest);
}
/******************************************************************/

/******************************************************************/
int ToUnicode(void *dest, void *src)
{
	char *csrc=src;
	char *cdest=dest;
	
	while(*csrc)
	{
		*cdest=*csrc;
		cdest++; csrc++;
		*cdest='\0';
		cdest++;
	}
	*cdest='\0';
	cdest++;
	*cdest='\0';
	return(csrc - (char *)src);
}
/******************************************************************/

/******************************************************************/
/* Mallocs a block of memory and zeros it out. If the malloc request
 * fails, the program aborts and prints the line number the zmalloc
 * call originated from. Otherwise a pointer to valid memory is
 * returned. */
void *zmalloc(int size, int caller)
{
	void *request;

	if((request=malloc(size)) == NULL) bail(caller);

	Copy(request, 0, size);
	return(request);
}
/******************************************************************/

/******************************************************************/
int main(int argc, char *argv[])
{
	int tmp, i, j, k, header;
	int	NIC=0;			// Network card we are currently scanning from
	int found=0;		// Flag to indicate shares were found
	int summary, findSMB=1, findNFS=1;
	ushort subnet;

	InitGlobals();
	gethostname(myname, sizeof(myname));
	myname[15]='\0'; // If myname is more than 15 chars we truncate it.
    printf("Local Host Name  = %s\n", myname);
	ScanInterfaces();
	for(i=0; i < 10; i++)
	{
		if(NICinfo[i].ipaddress[0] == 0)
			break;
		NIC=i;
		printf("Local IP Address = %s on %s\n",NICinfo[i].ipaddress,
												NICinfo[i].NICname);
	}
//	fputc('\n', stdout);
	if(argc > 1)
	{
		if(CompareStrings(argv[1], "-nn", 0) == 0)
			findNFS=0;
		else if(CompareStrings(argv[1], "--no-nfs", 0) == 0)
			findNFS=0;
		else if(CompareStrings(argv[1], "-ns", 0) == 0)
			findSMB=0;
		else if(CompareStrings(argv[1], "--no-smb", 0) == 0)
			findSMB=0;
		else
		{
            printf("\nfindshares version %s\nCopyright Richard A. Rost April"
                    " 23, 2011\n\n",VERSION);
			printf("This program attempts to locate remote shares on Windows,\n"
				"Samba, and NFS servers in the same subnet as the local IP\n"
				"address. Any remote shares found that are mounted in the\n"
				"local file system are indicated.\n\n"
				"Usage:  findshares [-nn] [--no-nfs] [-ns] [--no-smb]\n\n"
				"Options:\n"
				"\t-nn [--no-nfs]\tShow only Windows/Samba shares\n"
				"\t-ns [--no-smb]\tShow only NFS shares\n\n");
			return(0);
		}
	}
	
	while(NIC >= 0)
	{
		InitGlobals();
		tmp=inet_aton((char *) &NICinfo[NIC].ipaddress, (struct in_addr *) &net_ipaddress);
		if(tmp < 0) bail(__LINE__);
		net_ipbase=net_ipaddress & 0x00FFFFFF; // Base address in network order

		printf("\nScanning from %s on %s\n",NICinfo[NIC].ipaddress,
												NICinfo[NIC].NICname);

		if(findSMB)
		{
			GetShareInfoSMB();
			CreateMountlistSMB();
		}
		if(findNFS)
		{
			GetShareInfoNFS();
			CreateMountlistNFS();
		}

		summary=0;
		header=0;
		found=0;
		for(subnet=1; subnet < 255; subnet++)
		{
			if(SMBshares[subnet] != NULL)
			{
				if(header == 0)
				{
					header=printf("Samba/Windows Shares\n");
					PrintDashes();
					found=1;
				}
				printf("%s\n    %-16s %-16s  %s\n", SMBshares[subnet]->groupname,
												SMBshares[subnet]->servername,
												SMBshares[subnet]->ipaddress_string,
												SMBshares[subnet]->servercomment);
				i=SMBshares[subnet]->sharecount - 1; // -1 because array starts at 0
				while(i >= 0)
				{
					if(SMBshares[subnet]->share[i].mounted)
						summary=1;
					printf("\t%-30s %s\n", SMBshares[subnet]->share[i].name,
					SMBshares[subnet]->share[i].comment);
					i--;
				}
				fputc('\n', stdout);
			}
		}
		header=0;
		for(subnet=1; subnet < 255; subnet++)
		{
			if(NFSshares[subnet] != NULL)
				if(NFSshares[subnet]->port != 0)
				{
					if(header == 0)
					{
						header=printf("NFS Exports\n");
						PrintDashes();
						found=1;
					}
					printf("Server IP Address = %s\n   Exported Location"
										"      Allowable Clients\n",
										 NFSshares[subnet]->ipaddress_string);
					j=NFSshares[subnet]->sharecount - 1; // -1 because array starts at 0
					while(j >= 0)
					{
						if(NFSshares[subnet]->share[j]->mounted)
							summary=1;
						printf("   %s\n", NFSshares[subnet]->share[j]->name);
						k=NFSshares[subnet]->share[j]->usercount - 1; // -1 because array starts at 0
						while(k >= 0)
						{
							printf("                          %s\n", NFSshares[subnet]->share[j]->user[k]);
							k--;
						}
						j--;
						fputc('\n', stdout);
					}
				}
		}
		if(summary)
		{
			printf("\n%-16s%-22s %s\n", "Server", "Remote Resource", "Local Mount Point");
			PrintDashes();
			for(subnet=1; subnet < 255; subnet++)
			{
				if(SMBshares[subnet] != NULL)
				{
					i=SMBshares[subnet]->sharecount - 1; // -1 because array starts at 0
					while(i >= 0)
					{
						j=SMBshares[subnet]->share[i].mounted - 1; // -1 because array starts at 0
						while(j >= 0)
						{
							printf("%-16s%-22s %s\n", SMBshares[subnet]->ipaddress_string,
															SMBshares[subnet]->share[i].name,
															SMBshares[subnet]->share[i].mountpoint[j]);
							j--;
						}
						i--;
					}
				}
				if(NFSshares[subnet] != NULL)
				{
					i=NFSshares[subnet]->sharecount - 1; // -1 because array starts at 0
					while(i >= 0)
					{
						j=NFSshares[subnet]->share[i]->mounted - 1; // -1 because array starts at 0
						while(j >= 0)
						{
							printf("%-16s%-22s %s\n", NFSshares[subnet]->ipaddress_string,
															NFSshares[subnet]->share[i]->name,
															NFSshares[subnet]->share[i]->mountpoint[j]);
							j--;
						}
						i--;
					}
				}
			}
			fputc('\n', stdout);
		}
		for(subnet=1; subnet < 255; subnet++)
		{
			if(SMBshares[subnet] != NULL)
				free(SMBshares[subnet]);
		}
		free(SMBshares);
		SMBshares=NULL;
		for(subnet=1; subnet < 255; subnet++)
		{
			if(NFSshares[subnet] != NULL)
			{
				for(j=0; j < MAXDIRS; j++)
				{
					if(NFSshares[subnet]->share[j] != NULL)
						free(NFSshares[subnet]->share[j]);
				}
				free(NFSshares[subnet]);
			}
		}
		free(NFSshares);
		NFSshares=NULL;
		NIC--;
		if(found == 0)
			printf("No shares found\n");
	}
	
	free(NICinfo);
	close(udp137);
	return(0);
}


