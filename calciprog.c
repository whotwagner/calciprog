/*
 * Copyright (C) 2015 Wolfgang Hotwagner <code@feedyourhead.at>       
 *                                                                
 * This file is part of calciprog                                            
 *
 * calciprog is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License 
 * as published by the Free Software Foundation; either version 2 
 * of the License, or (at your option) any later version.
 *
 * concut is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License          
 * along with concut; if not, write to the 
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA 
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>


/* this union splits 32bit-adresses into 4 bytes 
   it's very important to use unsigned here. otherwise
   we would not be able to store values up to 255.
 */
typedef union {
	uint32_t val;
	unsigned char byte[4];
} uIp32;



/* this struct is used by calcNet() */
typedef struct ipcalcsubnet{
	unsigned int hosts; /* max. possible hosts */
	struct in_addr netmask;		/* subnetmask */
	struct in_addr netid;   	/* network id */
	struct in_addr hostmin;		/* first possible ip of this network */
	struct in_addr hostmax;		/* maximal possible ip of this network */
	struct in_addr broadcast;	/* broadcast-ip of this network */
	struct in_addr wildcard;	/* wildcard => inverse subnetmask */
} ipnet;


/*
   this function calculates our network-stuff
   it needs the struct ipcalcsubnet. we have to fill
   this struct first with a netid and a subnetmask
 */  
int calcNet(ipnet* netdata)
{
	/*
	   let's say, that we filled in our netid the address 192.168.10.3
	   and a subnetmask 255.255.255.0. then 192.168.10.3 is not the real network-id.
	   it's just a ip of the network 192.168.10.0/24.
	   We can easily calculate the netid: 
	   			just do a bitwise AND on the netmask and ip
	 */  
	netdata->netid.s_addr = netdata->netid.s_addr & netdata->netmask.s_addr;
	/*
	   the wildcard-address is the inverse of our subnetmask.
	   so if our subnetmask is 255.255.255.0, our wildcard would be 0.0.0.255
	   			Just do a bitwise NOT on the netmask
	 */  
	netdata->wildcard.s_addr = ~netdata->netmask.s_addr;
	/*
	   the broadcast-address is the highest possible ip-address of our network.
	   it's easy to calculate too:
	   			Just do a bitwise OR on net netid and the wildcard-address
	 */  
	netdata->broadcast.s_addr = netdata->netid.s_addr | netdata->wildcard.s_addr; 
	/*
	   the first useable hostip in our network is calculated by netid + 1
	   if our netid is 192.168.10.0 and our subnetmask is 255.255.255.0
	   then our first host would be 192.168.10.1
	   But at this point we do have a problem. We have to take care about little-endian
	   and big-endian.
	   We do have a 32bit-Adress. Lets assume our netid is 192.168.10.0. 
	   so netdata->netid.s_addr would have the decimal value 698560. 
	   that's in binary: 00000000000010101010100011000000
	   0 is in binary:   00000000                           <-first
	   10 is in binary:          00001010
	   168 is in binary:                 10101000
	   192 is in binary:                         11000000   <- last
	   therefore if we just increase our netid.s_addr by one we would get:
	   		193.168.10.0
	   instead of 192.168.10.1
	   That's why we have to change the byte-order, increase the 32bit by 1
	   and then change the byte-order back. our address is stored in host-byte-order,
	   so we have to use the function htonl() to change it into network-byte-order. if it's in 
	   network-byte-order we can increase it and then we use ntohl() convert it back 
	   to host-byte-order:
	 */  
	netdata->hostmin.s_addr = ntohl(htonl(netdata->netid.s_addr) +1);
	/* 
	  the last ip of each network is the broadcast-address. so the
	  last useable ip of our network is broadcast-address-1. like above 
	  we have to change our address to network-byte-order, decrease it and
	  change back to host-byte-order again:
	 */
	netdata->hostmax.s_addr = ntohl(htonl(netdata->broadcast.s_addr) -1);
	/*
	   hosts will store the maximal number of useable ips in our network.
	   it's calculated by wildcard -1. here we do not store a network-address! 
	   this is just an integer-value which stores the number of hosts. that's why
	   we convert our wildcard-address to network-byteorder, decrease it and store
	   this value into hosts.
	   in our example the wildcard is 0.0.0.255 which is in decimal 255. 
	   255 decreased by one would be 254. so we can use 254 ips in our network
	 */  
	netdata->hosts = htonl(netdata->wildcard.s_addr)-1;
			
	return 0;
}

/*
   getCidr() calculates the subnetmask and returns the
   subnetmask in cidr-notation.
   For example: 255.255.252.0 returns 22
   		255.255.255.0 returns 24
 */  
int getCidr(struct in_addr *nm)
{
	int cidr = 0;
	struct in_addr netmask;
	netmask.s_addr = nm->s_addr;

	/*
	   in CIDR-notation our  address looks like:
	   	192.168.10.0/24
	   So our subnetmask is just a short integer.
	   this integer is easily calculated:
	   
	   	just count all the 1-bit's of our
		subnetmask:

	   	00000000111111111111111111111111
			<here we have 24 1-bits>


	   But how can we count all the 1-bits?
	   with this tricky function:
	   let's assume a netmask of 255.255.255.0
	   stored in host-byte-order it would look like:
	   00000000111111111111111111111111

	   now let's do a bitwise AND with 0x01

	   00000000111111111111111111111111
	   00000000000000000000000000000001
	   our result would be:
	   00000000000000000000000000000001

	   then right-shift all the bits by one digit:

	   00000000011111111111111111111111

	   We do this until our bitmask looks like:

	   00000000000000000000000000000000
	   
	    perfect, because our loop stopps
	    when netmask.s_addr is 0.

	    so if we count all the loops of bitshifting
	    we are able to count all the 1-bits in our
	    netmask:

	 */  
	while ( netmask.s_addr )
	{
    		cidr += ( netmask.s_addr & 0x01 );
    		netmask.s_addr >>= 1;
	}

	return cidr;
}


int main(int argc, char *argv[])
{
	/* this is our network-structure
	   it's gonna filled by calcNet
	 */
	ipnet netdata;
	if(argc != 3)
	{
		fprintf(stderr,"usage: %s <ip> <subnetmask>\n",argv[0]);
		return EXIT_FAILURE;
	}
	/* inet_pton converts our ip-string into a 32bit-address structure */
	if(inet_pton(AF_INET,argv[1],&netdata.netid) == 1)
	{
		/* inet_pton converts our subnetmask-string into a 32bit-address structure */
		if(inet_pton(AF_INET,argv[2],&netdata.netmask) == 1)
		{
			/* now calculate our network */
			calcNet(&netdata);

			/* print our all members of our structure */
			printf("Network:\t%s/%d\n",inet_ntoa(netdata.netid),getCidr(&netdata.netmask));
			printf("Wildcard:\t%s\n",inet_ntoa(netdata.wildcard));
			printf("Netmask:\t%s\n",inet_ntoa(netdata.netmask));
			printf("Hostmin:\t%s\n",inet_ntoa(netdata.hostmin));
			printf("Hostmax:\t%s\n",inet_ntoa(netdata.hostmax));
			printf("Broadcast:\t%s\n",inet_ntoa(netdata.broadcast));
			printf("Hosts:\t\t%u\n",netdata.hosts);

			/* here is an example how to split a 32bit-integer
			   into 4 byte.
			 */  
			uIp32 b;
			b.val = netdata.netid.s_addr;
			printf("NetID-Bytewise: %u %u %u %u -> %u \n",b.byte[3],b.byte[2],b.byte[1],b.byte[0],b.val);
		}

	}

	return EXIT_SUCCESS;
}

/*                   ~~THE END~                   */
