/*
 *	msxGameReader library  
 *	Copyright (C) 2019 Antonio Jose Ramos Marquez (aka bigboss) @psxdev on twitter
 *  Repository https://github.com/psxdev/msxGameReader
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.

 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 */
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <libusb.h>
#include <debugnet.h>
#include <msxGameReader.h>

libusb_device * dev;

libusb_context * context=NULL;

#define MsxGameReaderMaxSlot 16
#define MsxGameReaderVendorId 0x1125
#define MsxGameReaderProductId 0xAC01
int msxGameReaderCount=0;
typedef struct MsxGameReader
{
	bool inserted;
	bool enabled;
	int handleId;
	int slot;
	int romType;
	int romSize;
	int sramSize;
	int sramBank;
	unsigned char *rom;
	unsigned char *sram;
}MsxGameReader;

libusb_device **devs;

libusb_device_handle *handle[MsxGameReaderMaxSlot];

MsxGameReader *mgr[16];
void usbCheckDevices(libusb_device *dev)
{
	int ret;
	struct libusb_device_descriptor desc;
	ret=libusb_get_device_descriptor(dev,&desc);
	if(ret<0)
	{
		debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s error with descriptor 0x%8x %s\n",__func__,ret,libusb_error_name(ret));
		return;
	}
	if(desc.idVendor==MsxGameReaderVendorId && desc.idProduct==MsxGameReaderProductId)
	{
		handle[msxGameReaderCount]=NULL;
		debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s found new Msx GameReader Device number %d\n",__func__,msxGameReaderCount);
		ret=libusb_open(dev, &handle[msxGameReaderCount]);
		if(ret==0)
		{


			debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s new handle for Msx GameReader Device number %d\n",__func__,msxGameReaderCount);
			mgr[msxGameReaderCount]=(MsxGameReader *)malloc(sizeof(MsxGameReader));
			if(mgr[msxGameReaderCount]!=NULL)
			{
				mgr[msxGameReaderCount]->enabled=false;
				mgr[msxGameReaderCount]->inserted=false;
				mgr[msxGameReaderCount]->slot=-1;
				mgr[msxGameReaderCount]->romType=-1;
				mgr[msxGameReaderCount]->romSize=0;
				mgr[msxGameReaderCount]->sramSize=0;
				mgr[msxGameReaderCount]->handleId=msxGameReaderCount;
				mgr[msxGameReaderCount]->rom=NULL;
				mgr[msxGameReaderCount]->sram=NULL;
				msxGameReaderCount++;
			}
			else
			{
				debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s  memory error\n",__func__);
			}
		}
		else
		{
			debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s error opening handle 0x%8x %s\n",__func__,ret,libusb_error_name(ret));
		}

		
		
	}
}
void usbInit(int logLevel)
{
	int ret;
	int i;

	debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s in\n",__func__);

	ret=libusb_init(&context);
	if(ret<0)
	{
		debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s error initializing usb stack 0x%8x\n",__func__,ret);
		return;
	}
	debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s usb stack initialized\n",__func__,ret);
	libusb_set_debug(context,logLevel);
	ret=libusb_get_device_list(NULL,&devs);
	if(ret<0)
	{
		debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s error retrieving usb devices 0x%8x %s\n",__func__,ret,libusb_error_name(ret));
		return;
	}
	debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s %d devices found\n",__func__,ret);

	for (i=0;devs[i];++i)
	{
		usbCheckDevices(devs[i]);
	}
	
	//libusb_free_device_list(devs, 1);
	debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s out\n",__func__);

}
void usbFinish()
{
	int i;
	for(i=0;i<msxGameReaderCount;i++)
	{
		libusb_close(handle[i]);
	}
	libusb_exit(context);
	return;
}
int msxGameReaderSendCommand(libusb_device_handle *handler,uint8_t nCmd,uint16_t wValue,uint16_t wIndex)
{
	int ret;
	switch(nCmd)
	{
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			ret=libusb_control_transfer(handler,0xC0,nCmd,wValue,wIndex,0,0,0);
			break;
		default:
			debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s Unsupported command. (nCmd=%d)\n",__func__,nCmd);
			return -1;
	}
	if(ret!=0)
	{
		debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s error sending command 0x%8x %s\n",__func__,ret,libusb_error_name(ret));
	}
	return ret;
}


int msxGameReaderReadFrom(libusb_device_handle *handler,unsigned char *buffer,uint16_t address,uint16_t length,uint8_t nCmd)
{
	int ret;
	int numBytesTransfered;
	switch(nCmd)
	{
		case 1:
		case 2:
		case 4:
			ret=msxGameReaderSendCommand(handler,nCmd,address,length);
			break;
		default:
			debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s Unsupported command. (nCmd=%d)\n",__func__,nCmd);
			return -1;
	}
	while(ret==0 && length!=0)
	{
		ret=libusb_bulk_transfer(handler,0x82,buffer,length,&numBytesTransfered,0);
		debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s bulk transfer length=%d bytesTransfered=%d return=%d\n",__func__,length,numBytesTransfered,ret);
		length=length-numBytesTransfered;	
	}
	if(ret!=0)
	{
		debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s error in transfer 0x%8x %s\n",__func__,ret,libusb_error_name(ret));
		ret=-1;
	}
	return ret;
}
int msxGameReaderReadMemory(int handleId,unsigned char *buffer,uint16_t address,uint16_t length)
{
	int ret;
	if(handle[handleId]==NULL || buffer==NULL)
	{
		return -1;
	}
	ret=msxGameReaderReadFrom(handle[handleId],buffer,address,length,2);
	debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s   address=%4x length=%d ret=%d\n\n",__func__,address,length,ret);

	return ret;
}
int msxGameReaderReadIo(int handleId,unsigned char *buffer,uint16_t address,uint16_t length)
{
	int ret;
	if(handle[handleId]==NULL || buffer==NULL)
	{
		return -1;
	}
	ret=msxGameReaderReadFrom(handle[handleId],buffer,address,length,4);
	return ret;
}


int msxGameReaderGetSlotStatus(int handleId,unsigned char *buffer)
{
	int ret;
	if(handle[handleId]==NULL || buffer==NULL)
	{
		return -1;
	}
	ret=msxGameReaderReadFrom(handle[handleId],buffer,0,3,1);
	return ret;
}
int msxGameReaderIsSlotEnabled(int handleId)
{
	int ret;
	if(handle[handleId]==NULL)
	{
		return -1;
	}
	ret=mgr[handleId]->enabled;
	return ret;
}
int msxGameReaderIsCartridgeInserted(int handleId)
{
	int ret;
	if(handle[handleId]==NULL)
	{
		return -1;
	}
	unsigned char status[3];
	ret=msxGameReaderGetSlotStatus(handleId,status);

	return !ret&&status[1];
}
int msxGameReaderWriteTo(libusb_device_handle *handler,unsigned char *buffer,uint16_t address,uint16_t length,uint8_t nCmd)
{
	int ret;
	int numBytesTransfered;
	switch(nCmd)
	{
		case 3:
		case 5:
			ret=msxGameReaderSendCommand(handler,nCmd,address,length);
			break;
		default:
			debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s Unsupported command. (nCmd=%d)\n",__func__,nCmd);
			return -1;
	}
	while(ret==0 && length!=0)
	{
		ret=libusb_bulk_transfer(handler,0x2,buffer,length,&numBytesTransfered,0);
		debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s bulk transfer length=%d bytesTransfered=%d return=%d\n",__func__,length,numBytesTransfered,ret);
		length=length-numBytesTransfered;	
	}
	if(ret!=0)
	{
		debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s error in transfer 0x%8x %s\n",__func__,ret,libusb_error_name(ret));
		ret=-1;
	}
	return ret;
}

int msxGameReaderWriteMemory(int handleId,unsigned char *buffer,uint16_t address,uint16_t length)
{
	int ret;
	if(handle[handleId]==NULL || buffer==NULL)
	{
		return -1;
	}
	ret=msxGameReaderWriteTo(handle[handleId],buffer,address,length,3);
	debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s byteWrite=%2x  address=%4x length=%d ret=%d\n\n",__func__,*buffer,address,length,ret);

	return ret;
}
int msxGameReaderWriteIo(int handleId,unsigned char *buffer,uint16_t address,uint16_t length)
{
	int ret;
	if(handle[handleId]==NULL || buffer==NULL)
	{
		return -1;
	}
	ret=msxGameReaderWriteTo(handle[handleId],buffer,address,length,5);
	return ret;
}
int msxGameReaderInit()
{
	int i;
	int ret;
	unsigned char buf[3];
	if(msxGameReaderCount==0)
	{
		debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s error no Game Reader available\n",__func__);
		return -1;
	}
	for(i=0;i<msxGameReaderCount;i++)
	{
		if(handle[i]!=NULL)
		{
			ret=libusb_set_configuration(handle[i],1);
			if(ret!=0)
			{
				debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s error cannot set configuration 1 0x%8x %s\n",__func__,ret,libusb_error_name(ret));

			}
			debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s Msx Game Reader number: %d configuration set\n",__func__,i);

			ret=libusb_claim_interface(handle[i], 0);
				debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s libusb_claim_interface 0x%8x %s\n",__func__,ret,libusb_error_name(ret));


			ret=libusb_set_interface_alt_setting(handle[i], 0, 0);

				debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s libusb_claim_interface 0x%8x %s\n",__func__,ret,libusb_error_name(ret));



			ret=msxGameReaderGetSlotStatus(i,buf);
			if(ret==0)
			{
				mgr[i]->slot=i;
				mgr[i]->inserted=buf[1]?true:false;
				mgr[i]->enabled=buf[0]?true:false;
				debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s Msx Game Reader number: %d status[0]=%2x status[1]=%2x status[2]=%2x\n",__func__,i,buf[0],buf[1],buf[2]);

			}
		}
	}
	return 0;
	
}
int msxGameReaderAnalyzeRomType(int handleId)
{
	int ret,ret1;
	unsigned char status[3];
	unsigned char byteWrite;
	unsigned char byte16k[16384];
	unsigned char byte16k2[16384];
	unsigned char *piVar2;
	unsigned char *piVar3;
	unsigned char *piVar4;
	bool bVar5;

	if(handleId>-1 && handleId<16 && handle[handleId]!=NULL)
	{
		if(msxGameReaderCount)
		{
			mgr[handleId]->romType=-1;
			ret=msxGameReaderIsSlotEnabled(handleId);
			ret1=msxGameReaderGetSlotStatus(handleId,status);
			if(!ret1 && ret && status[1])
			{
				byteWrite=0;
				ret=msxGameReaderWriteMemory(handleId,&byteWrite,0xbffe,1);
				if(ret==0)
				{
					byteWrite=0;
					ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7ffe,1);
					if(ret==0)
					{
						byteWrite=0;
						ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7ff7,1);
						if(ret==0)
						{
							byteWrite=0;
							ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x5ffe,1);
							if(ret==0)
							{
								byteWrite=0;
								ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x5fff,1);
								if(ret==0)
								{
									byteWrite=0;
									ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
									if(ret==0)
									{
										byteWrite=0;
										ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x4000,1);
										if(ret==0)
										{
											byteWrite=0;
											ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x5000,1);
											if(ret==0)
											{
												byteWrite=0;
												ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x6000,1);
												if(ret==0)
												{
													byteWrite=0;
													ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x6800,1);
													if(ret==0)
													{
														byteWrite=0;
														ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7000,1);
														if(ret==0)
														{
															byteWrite=0;
															ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7800,1);
															if(ret==0)
															{
																byteWrite=0;
																ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x8000,1);
																if(ret==0)
																{
																	byteWrite=0;
																	ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x9000,1);
																	if(ret==0)
																	{
																		byteWrite=0;
																		ret=msxGameReaderWriteMemory(handleId,&byteWrite,0xa000,1);
																		if(ret==0)
																		{
																			byteWrite=0;
																			ret=msxGameReaderWriteMemory(handleId,&byteWrite,0xb000,1);
																			if(((ret==0) &&
                                          										(ret=msxGameReaderReadMemory(handleId,byte16k,0x4000,0x4000),ret==0)) &&
                                         										(ret=msxGameReaderReadMemory(handleId,byte16k2,0xa000,0x2000),ret==0))

                                      										{
                                      											ret=0x800;
                                        										bVar5=true;
                                        										piVar2=byte16k;
                                        										piVar4=byte16k2;
                                        										
                                        										do 
                                        										{
                                          											if(ret==0) break;
                                          											ret=ret-1;
                                          											bVar5=*piVar2==*piVar4;
                                          											piVar2=piVar2+1;
                                          											piVar4=piVar4+1;
                                        										}while(bVar5);
                                        										if(bVar5)
                                        										{
                                        											byteWrite=0xff;
                                          											ret=msxGameReaderWriteMemory(handleId,&byteWrite,0xa000,1);
                                          											if(ret!=0) goto LAB_10002307;
                                          											byteWrite=0;
                                          											ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
                                          											if((ret!=0) || (ret=msxGameReaderReadMemory(handleId,byte16k2,0xa000,0x2000),ret!=0)) goto LAB_10002307;
                                          											byteWrite=0;
                                          											ret=msxGameReaderWriteMemory(handleId,&byteWrite,0xa000,1);
                                          											if(ret!=0) goto LAB_10002307;
                                          											ret=0x800;
                                          											bVar5=true;
                                          											piVar2=byte16k;
                                          											piVar4=byte16k2;

                                          											do 
                                          											{
                                            											if(ret==0) break;
                                            											ret=ret-1;
                                            											bVar5=*piVar2==*piVar4;
                                            											piVar2=piVar2+1;
                                            											piVar4=piVar4+1;
                                          											} while(bVar5);
                                          											byteWrite=0xff;
                                          											if(!bVar5) //!bVar5
                                          											{
                                          												//loc_10001E05 
                                            											ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x4000,1);
                                            											if(ret==0) 
                                            											{
                                              												byteWrite=0;
                                              												ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
                                              												if((ret==0) && (ret=msxGameReaderReadMemory(handleId,byte16k2,0x4000,0x2000), ret==0)) 
                                              												{
                                                												byteWrite=0;
                                                												ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x4000,1);
                                                												if (ret==0) 
                                                												{
                                                  													ret=0x800;
                                                  													bVar5=true;
                                                  													piVar2=byte16k;
                                                  													piVar4=byte16k2;
                                                  													do 
                                                  													{
                                                    													if (ret==0) break;
                                                    													ret=ret-1;
                                                    													bVar5=*piVar2==*piVar4;
                                                    													piVar2=piVar2+1;
                                                    													piVar4=piVar4+1;
                                                  													}while(bVar5);
                                                  													if(bVar5) 
                                                  													{
                                                    													mgr[handleId]->romType=4;
                                                    													byteWrite=1;
                                                    													ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x6000,1);
                                                    													if(ret==0) 
                                                    													{
                                                      														byteWrite=2;
                                                      														ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x8000,1);
                                                      														if(ret==0)
                                                      														{
                                                        														byteWrite=3;
                                                        														ret=msxGameReaderWriteMemory(handleId,&byteWrite,0xa000,1);
                                                        														if(ret==0)
                                                        														{
                                                        															debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s Msx Game Reader number: %d romType=%d\n",__func__,handleId,mgr[handleId]->romType);

                                                        															return 1;
                                                        															/* neg   eax     ; ZF=1/0 for zero/non-zero, CF=not(ZF)
    																												sbb   eax,eax  ; eax = 0/-1 for CF=0/1
    																												inc   eax      ; 1 when eax was 0 at start, 0 otherwise*/
                                                        														}

                                                      														}
                                                    													}
                                                  													}
                                                  													else 
                                                  													{
                                                  														//loc_10001F22
                                                    													mgr[handleId]->romType=1;
                                                    													debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s Msx Game Reader number: %d romType=%d\n",__func__,handleId,mgr[handleId]->romType);
                                                    													return 1;
                                                  													}
                                                												}
                                              												}
                                            											}
                                            											goto LAB_10002307;
                                            										}

                                            										ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x9000,1);
                                          											if (ret!=0) goto LAB_10002307;
                                          											byteWrite=0;
                                          											ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
                                          											if ((ret!=0) ||(ret=msxGameReaderReadMemory(handleId,byte16k2,0x8000,0x2000),ret!=0)) goto LAB_10002307;
                                          											byteWrite=0;
                                          											ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x9000,1);
                                          											if(ret!=0) goto LAB_10002307;
                                          											ret=0x800;
                                          											bVar5=true;
                                          											piVar2=byte16k;
                                          											piVar4=byte16k2;
                                          											do 
                                          											{
                                            											if (ret==0) break;
                                            											ret=ret-1;
                                            											bVar5=*piVar2==*piVar4;
                                            											piVar2=piVar2+1;
                                            											piVar4=piVar4+1;
                                          											}while(bVar5);
                                          											if (!bVar5) 
                                          											{
                                          												//loc_10001D6B
                                            											mgr[handleId]->romType=3;
                                            											byteWrite=0;
                                            											ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x5000,1);
                                            											if (ret==0) 
                                            											{
                                              												byteWrite=1;
                                              												ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7000,1);
                                              												if(ret==0) 
                                              												{
                                                												byteWrite=2;
                                                												ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x9000,1);
                                                												if (ret==0) 
                                                												{
                                                  													byteWrite=3;
                                                  													msxGameReaderWriteMemory(handleId,&byteWrite,0xb000,1);
                                                  													if(ret==0)
                                                  													{
                                                  														debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s Msx Game Reader number: %d romType=%d\n",__func__,handleId,mgr[handleId]->romType);
                                                  														return 1;
                                                  													}
                                                												}
                                              												}
                                            											}
                                            											goto LAB_10002307;
                                          											}
                                          											byteWrite=0xff;
                                          											ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x6800,1);
                                          											if(ret!=0) goto LAB_10002307;
                                          											byteWrite=0;
                                          											ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
                                          											if ((ret!=0) || (ret=msxGameReaderReadMemory(handleId,byte16k2,0x6000,0x2000), ret!=0)) goto LAB_10002307;
                                          											byteWrite=0;
                                          											ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x6800,1);
                                          											if (ret!=0) goto LAB_10002307;
                                          											ret=0x800;
                                          											bVar5=true;
                                          											piVar2=byte16k;
                                          											piVar4=byte16k2;
                                          											do
                                          											{
                                            											if(ret==0) break;
                                            											ret=ret-1;
                                            											bVar5=*piVar2==*piVar4;
                                            											piVar2=piVar2+1;
                                            											piVar4=piVar4+1;
                                          											}while(bVar5);
                                          											if(!bVar5) 
                                          											{
                                          												//loc_10001D57
                                            											mgr[handleId]->romType=5;
                                            											debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s Msx Game Reader number: %d romType=%d\n",__func__,handleId,mgr[handleId]->romType);
                                            											return 1;
                                          											}
                                          											mgr[handleId]->romType=-1;
                                          										}
                                          										//loc_10001929
                                        										byteWrite=0;
                                        										ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
                                        										if((ret==0) &&
                                           											(ret=msxGameReaderReadMemory(handleId,byte16k2,0x8000,0x4000),ret==0))
                                                              					{
                                          											ret=0x1000;
                                          											bVar5=true;
                                          											piVar2=byte16k;
                                          											piVar4=byte16k2;
                                          											do 
                                          											{
                                            											if(ret==0) break;
                                            											ret=ret-1;
                                            											bVar5=*piVar2==*piVar4;
                                            											piVar2=piVar2+1;
                                            											piVar4=piVar4+1;
                                          											}while(bVar5);
                                          											if(bVar5) 
                                          											{

                                            											byteWrite=0xff;
                                            											ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x8000,1);
                                            											if (ret!=0) goto LAB_10002307;
                                            											byteWrite=0;
                                            											ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
                                            											if ((ret!=0) || (ret=msxGameReaderReadMemory(handleId,byte16k2,0x8000,0x4000), ret!=0)) goto LAB_10002307;
                                            											byteWrite = 0;
                                            											ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x8000,1);
                                            											if (ret!=0) goto LAB_10002307;
                                            											ret=0x1000;
                                            											bVar5=true;
                                            											piVar2=byte16k;
                                            											piVar4=byte16k2;
                                            											do 
                                            											{
                                              												if(ret==0) break;
                                              												ret=ret-1;
                                              												bVar5=*piVar2==*piVar4;
                                              												piVar2=piVar2+1;
                                              												piVar4=piVar4+1;
                                            											}while(bVar5);
                                            											if(!bVar5) 
                                            											{
                                            												//loc_10001F4A
                                              												mgr[handleId]->romType=2;
                                              												debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s Msx Game Reader number: %d romType=%d\n",__func__,handleId,mgr[handleId]->romType);
                                              												return 1;
                                            											}
                                            											byteWrite=0xff;
                                            											ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7000,1);
                                            											if(ret!=0) goto LAB_10002307;
                                            											byteWrite=0;
                                            											ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
                                            											if ((ret!=0) || (ret=msxGameReaderReadMemory(handleId,byte16k2,0x8000,0x4000), ret!= 0)) goto LAB_10002307;
                                            											byteWrite=0;
                                            											ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7000,1);
                                            											if (ret!=0) goto LAB_10002307;
                                            											ret=0x1000;
                                            											bVar5=true;
                                            											piVar2=byte16k;
                                            											piVar4=byte16k2;
                                            											do 
                                            											{
                                              												if(ret==0) break;
                                              												ret=ret-1;
                                              												bVar5=*piVar2==*piVar4;
                                              												piVar2=piVar2+1;
                                              												piVar4=piVar4+1;
                                            											}while(bVar5);
                                            											if(!bVar5) 
                                            											{
                                              												mgr[handleId]->romType=6;
                                              												debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s Msx Game Reader number: %d romType=%d\n",__func__,handleId,mgr[handleId]->romType);
                                              												return 1;
                                            											}
                                            											mgr[handleId]->romType=-1;
                                          											}
                                          											//loc_10001AE6
                                          											byteWrite=0;
                                          											ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
                                          											if ((ret==0) && (ret=msxGameReaderReadMemory(handleId,byte16k,0x4000,2),ret==0)) 
                                          											{
                                            											if ((*(short *)byte16k)==0x4241) //AB
                                            											{
                                              												byteWrite=0xff;
                                              												ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7000,1);
                                              												if (ret!=0) goto LAB_10002307;
                                              												byteWrite=0;
                                              												ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
                                              												if((ret!=0) || (ret=msxGameReaderReadMemory(handleId,byte16k,0x8000,0x4000), ret !=0)) goto LAB_10002307;
                                              												byteWrite=0;
                                              												ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7000,1);
                                              												if ((ret!=0) || (ret=msxGameReaderReadMemory(handleId,byte16k2,0x8000,0x4000), ret!= 0))goto LAB_10002307;
                                              												ret=0x1000;
                                              												bVar5=true;
                                              												piVar2=byte16k;
                                              												piVar4=byte16k2;
                                              												do 
                                              												{
                                                												if(ret==0) break;
                                                												ret=ret-1;
                                                												bVar5=*piVar2==*piVar4;
                                                												piVar2=piVar2+1;
                                                												piVar4= piVar4+1;
                                              												}while(bVar5);
                                              												if(bVar5) 
                                              												{
                                              													//loc_10001F5E
                                                												byteWrite=0;
                                                												ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x5fff,1);
                                                												if (ret!=0) goto LAB_10002307;
                                                												byteWrite=0;
                                                												ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7ff7,1);
                                                												if ((ret!=0) || (ret=msxGameReaderReadMemory(handleId,byte16k,0x4000,0x4000), ret!=0))goto LAB_10002307;
                                                												byteWrite=3;
                                                												ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7ff7,1);
                                                												if (ret!=0) goto LAB_10002307;
                                                												byteWrite=0;
                                                												ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
                                                												if ((ret!=0) || (ret=msxGameReaderReadMemory(handleId,byte16k2,0x4000,0x4000), ret!=0)) goto LAB_10002307;
                                                												ret =0x1000;
                                                												bVar5=true;
                                                												piVar2=byte16k;
                                                												piVar4=byte16k2;
                                                												do 
                                                												{
                                                  													if(ret==0) break;
                                                  													ret=ret-1;
                                                  													bVar5=*piVar2==*piVar4;
                                                  													piVar2=piVar2+1;
                                                  													piVar4=piVar4+1;
                                                												}while(bVar5);
                                                												if(!bVar5) 
                                                												{
                                                  													byteWrite=0x10;
                                                  													ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7ff6,1);
                                                  													if (ret!=0) goto LAB_10002307;
                                                  													byteWrite=0;
                                                  													ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7ff6,1);
                                                  													if ((ret!=0) || (ret=msxGameReaderReadMemory(handleId,byte16k2,0x4000,0x2000), ret!=0)) goto LAB_10002307;
                                                  													byteWrite=0;
                                                  													ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7ff7,1);
                                                  													if (ret!=0) goto LAB_10002307;
                                                  													ret=0x800;
                                                  													bVar5=true;
                                                  													piVar2=byte16k;
                                                  													piVar4=byte16k2;
                                                  													do 
                                                  													{
                                                    													if(ret==0) break;
                                                    													ret=ret-1;
                                                    													bVar5=*piVar2==*piVar4;
                                                    													piVar2=piVar2+1;
                                                    													piVar4=piVar4+1;
                                                  													}while(bVar5);
                                                  													if(!bVar5) 
                                                  													{
                                                    													byteWrite = 0x4d;
                                                    													ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x5ffe,1);
                                                    													if(ret!=0) goto LAB_10002307;
                                                    													byteWrite=0x69;
                                                    													ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x5fff,1);
                                                    													if(ret!=0) goto LAB_10002307;
                                                    													byteWrite=0;
                                                    													ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
                                                    													if((ret!=0) || (ret=msxGameReaderReadMemory(handleId,byte16k2,0x4000,0x2000),ret!=0)) goto LAB_10002307;
                                                    													byteWrite=0;
                                                    													ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x5fff,1);
                                                    													if(ret!=0) goto LAB_10002307;
                                                    													ret=0x800;
                                                   														bVar5=true;
                                                    													piVar3=byte16k;
                                                    													piVar2=byte16k2;
                                                    													do 
                                                    													{
                                                      														if(ret==0) break;
                                                      														ret=ret-1;
                                                      														bVar5=*piVar3==*piVar2;
                                                      														piVar3=piVar3+1;
                                                      														piVar2=piVar2+1;
                                                    													}while(bVar5);
                                                    													if(!bVar5) 
                                                    													{
                                                      														mgr[handleId]->romType=7;
                                                      														debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s Msx Game Reader number: %d romType=%d\n",__func__,handleId,mgr[handleId]->romType);
                                                      														return 1;
                                                    													}
                                                  													}
                                                												}

                                              												}
                                              												else 
                                              												{
                                              													
                                                												byteWrite=0xf;
                                                												ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7000,1);
                                                												if(ret!=0) goto LAB_10002307;
                                                												byteWrite=0;
                                                												ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
                                                												if((ret!=0) || (ret=msxGameReaderReadMemory(handleId,byte16k2,0x8000,0x4000), ret!=0)) goto LAB_10002307;
                                                												ret=0x1000;
                                                												bVar5=true;
                                                												piVar2=byte16k;
                                                												piVar4=byte16k2;
                                                												do 
                                                												{
                                                  													if(ret==0) break;
                                                  													ret=ret-1;
                                                  													bVar5=*piVar2==*piVar4;
                                                  													piVar2=piVar2+1;
                                                  													piVar4=piVar4+1;
                                                												}while(bVar5);
                                                												if(bVar5) 
                                                												{
                                                  													byteWrite=0x17;
                                                  													ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7000,1);
                                                  													if (ret!=0) goto LAB_10002307;
                                                  													byteWrite=0;
                                                  													ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
                                                  													if ((ret!=0) || (ret=msxGameReaderReadMemory(handleId,byte16k2,0x8000,0x4000), ret!=0)) goto LAB_10002307;
                                                  													byteWrite=0;
                                                  													ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7000,1);
                                                  													if (ret!=0) goto LAB_10002307;
                                                  													ret=0x1000;
                                                  													bVar5=true;
                                                  													piVar3=byte16k;
                                                  													piVar2=byte16k2;
                                                  													do 
                                                  													{
                                                    													if(ret==0) break;
                                                    													ret=ret-1;
                                                    													bVar5=*piVar3==*piVar2;
                                                    													piVar3=piVar3+1;
                                                    													piVar2=piVar2+1;
                                                  													}while(bVar5);
                                                  													if(bVar5) 
                                                  													{
                                                    													mgr[handleId]->romType=6;
                                                    													debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s Msx Game Reader number: %d romType=%d\n",__func__,handleId,mgr[handleId]->romType);
                                                    													return 1;
                                                  													}
                                                												}
                                              												}
                                              												
                                            											}
                                            											else 
                                            											{
                                              												byteWrite=0;
                                              												ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x5fff,1);
                                              												if ((ret!=0) || (ret=msxGameReaderReadMemory(handleId,byte16k,0x4000,0x2000), ret!=0)) goto LAB_10002307;
                                              												byteWrite=0x4d;
                                              												ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x5ffe,1);
                                              												if (ret!=0) goto LAB_10002307;
                                              												byteWrite=0x69;
                                              												ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x5fff,1);
                                              												if(ret!=0) goto LAB_10002307;
                                              												byteWrite=0;
                                              												ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
                                              												if ((ret!=0) || (ret=msxGameReaderReadMemory(handleId,byte16k2,0x4000,0x2000), ret!=0)) goto LAB_10002307;
                                              												byteWrite=0;
                                              												ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x5fff,1);
                                              												if(ret!=0) goto LAB_10002307;
                                              												ret=0x800;
                                              												bVar5=true;
                                              												piVar3=byte16k;
                                              												piVar2=byte16k2;
                                              												do 
                                              												{
                                                												if(ret==0) break;
                                                												ret=ret-1;
                                                												bVar5=*piVar3==*piVar2;
                                                												piVar3=piVar3+1;
                                                												piVar2=piVar2+1;
                                              												}while(bVar5);
                                              												if(!bVar5)
                                              												{
                                                												mgr[handleId]->romType=7;
                                                												debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s Msx Game Reader number: %d romType=%d\n",__func__,handleId,mgr[handleId]->romType);
                                                												return 1;
                                              												}
                                            											}
                                            											//loc_100022F8
                                                										mgr[handleId]->romType=0;
                                                										debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s Msx Game Reader number: %d romType=%d\n",__func__,handleId,mgr[handleId]->romType);
                                                										return 1;
                                          											}
                                        
                                                              					}
                                      										}
																		}
																	}
																}
															}
														}
													}
												}
											}
										}
									}
								}
							}						
						}					
					}
				}

			}

		}
	}
	else
	{
        LAB_10002307:
        debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s Msx Game Reader number: %d romType=%d\n",__func__,handleId,mgr[handleId]->romType);
		return -1;
	}
	return -1;
}
int msxGameReaderAnalyzeRomSize(int handleId)
{
	int ret;	
	unsigned char status[3];
	if(handleId>-1 && handleId<16 && handle[handleId]!=NULL)
	{
		mgr[handleId]->romSize=-1;

		if((ret=msxGameReaderIsSlotEnabled(handleId),ret==1) && (ret=msxGameReaderGetSlotStatus(handleId,status),ret==0) && status[1] && mgr[handleId]->romType>-1)
		{
			
			switch(mgr[handleId]->romType)
			{
				case 0:
				case 7:
					mgr[handleId]->romSize=8;
					return 1;
				case 2:
				case 6:
					mgr[handleId]->romSize=256;
					return 1;
				case 3:
					mgr[handleId]->romSize=64;
					return 1;
				default:
					mgr[handleId]->romSize=128;
					return 1;

			}
		}
	}
	return 0;

}

int msxGameReaderAnalyzeSram(int handleId)
{
	int ret;
	unsigned char kk;
	unsigned char byteWrite;
	unsigned char byte16k[16384];
	unsigned char byte16k2[16384];
	unsigned char byte16k3[16384];
	unsigned char status[3];
	bool bVar8;
	unsigned char puVar4;
	unsigned char *piVar4;
	unsigned char *piVar5;
	unsigned char *piVar6;
	unsigned char *piVar7;


	if(handleId>-1 && handleId<16 && handle[handleId]!=NULL)
	{
		if((ret=msxGameReaderIsSlotEnabled(handleId),ret==1) && (ret=msxGameReaderGetSlotStatus(handleId,status),ret==0) && status[1] && mgr[handleId]->romType>-1)
		{
			switch(mgr[handleId]->romType)
			{
				case 4:
					mgr[handleId]->sramSize=0;
					mgr[handleId]->sramBank=0;
					byteWrite=0x10;
					ret=msxGameReaderWriteMemory(handleId,&byteWrite,0xa000,1);
                    if(ret==0)
                    {
                    	byteWrite=0;
						ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x0,1);
						if(ret==0 && (ret=msxGameReaderReadMemory(handleId,byte16k,0xb000,0x200),ret==0))
                    	{
                    		do
                    		{
          						byte16k2[ret]=~byte16k[ret];
         						ret=ret+1;
                    		}while(ret<0x200);
                    		ret=msxGameReaderWriteMemory(handleId,byte16k2,0xb000,0x200);
                    		byteWrite=0;
                    		ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x0,1);
                    		if(ret==0 && (ret=msxGameReaderReadMemory(handleId,byte16k2,0xb000,0x200),ret==0) && (ret=msxGameReaderWriteMemory(handleId,byte16k,0xb000,0x200),ret==0))
                    		{
                    			byteWrite=0;
                    			ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x0,1);
                    			if(ret==0)
                    			{
                    				do
                    				{
                    					byte16k2[ret]=~byte16k2[ret];
         								ret=ret+1;
                    				}while(ret<0x200);
                    				ret=0x80;
                    				bVar8=true;
                    				piVar6=byte16k;
                    				piVar5=byte16k2;
                    				do
                    				{
                    					if(ret==0) break;
                    					ret=ret-1;
                    					bVar8=*piVar5==*piVar6;
                    					piVar6=piVar6+1;
                    					piVar5=piVar5+1;
                    				}while(bVar8);
                    				if(bVar8 && (ret=msxGameReaderReadMemory(handleId,byte16k2,0xb000,0x200),ret==0))
                    				{
                    					ret=0x80;
                    					bVar8=true;
                    					piVar6=byte16k;
                    					piVar5=byte16k2;
                    					do
                    					{
                    						if(ret==0) break;
                    						ret=ret-1;
                    						bVar8=*piVar5==*piVar6;
                    						piVar6=piVar6+1;
                    						piVar5=piVar5+1;
                    					}while(bVar8);
                    					if(bVar8)
                    					{
                    						mgr[handleId]->sramSize=2;
											mgr[handleId]->sramBank=16;
											if(mgr[handleId]->romSize>16)
											{
												mgr[handleId]->romSize=16;

											}
                    					}       					
                    				}
                    				return 1;
                    			}
                    		}
                    	}
                    }
                    return 0;
                case 5:
                	puVar4=0x80;
                	//jander=0x80;
                	do
                	{
                		byteWrite=puVar4;
                		ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7800,1);
                		if(ret!=0) return 0;
                		byteWrite=0;
                		ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
                		if(ret!=0 || (ret=msxGameReaderReadMemory(handleId,byte16k,0xa000,0x200),ret!=0)) return 0;
                		do
                    	{
          					byte16k2[ret]=~byte16k[ret];
         					ret=ret+1;
                    	}while(ret<0x200);
                    	ret=msxGameReaderWriteMemory(handleId,byte16k2,0xa000,0x200);
                    	if(ret!=0) return 0;
                    	byteWrite=0;
                		ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
                		if(ret!=0 || (ret=msxGameReaderReadMemory(handleId,byte16k2,0xa000,0x200),ret!=0) || (ret=msxGameReaderWriteMemory(handleId,byte16k,0xa000,0x200),ret!=0)) return 0;
                		ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
                		if(ret!=0) return 0;
                		do
                    	{
          					byte16k2[ret]=~byte16k2[ret];
         					ret=ret+1;
                    	}while(ret<0x200);
                    	ret=0x80;
                    	bVar8=true;
                    	piVar6=byte16k;
                    	piVar5=byte16k2;
                    	do
                    	{
                    		if(ret==0) break;
                    		ret=ret-1;
                    		bVar8=*piVar5==*piVar6;
                    		piVar6=piVar6+1;
                    		piVar5=piVar5+1;
                    	}while(bVar8);
                    	if(bVar8)
                    	{
                    		ret=msxGameReaderReadMemory(handleId,byte16k2,0xa000,0x200);
                    		if(ret!=0) return 0;
                    		ret=0x80;
                    		bVar8=true;
                    		piVar6=byte16k;
                    		piVar5=byte16k2;
                    		do
                    		{
                    			if(ret==0) break;
                    			ret=ret-1;
                    			bVar8=*piVar5==*piVar6;
                    			piVar6=piVar6+1;
                    			piVar5=piVar5+1;
                    		}while(bVar8);
                    		if(bVar8) break;
                    	}
                    	puVar4=puVar4>>1;
                	}while(puVar4!=0);
                	mgr[handleId]->sramSize=0;
					mgr[handleId]->sramBank=puVar4;
					if(puVar4==0) return 1;
					byteWrite=puVar4;
					ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7000,1);
					if(ret!=0) return 0;
					if(mgr[handleId]->sramBank>1)
					{
						kk=1;
						do
						{
							byteWrite=mgr[handleId]->sramBank+kk;
							ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7800,1);
							if(ret!=0) return 0;
							byteWrite=0;
							ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
							if(ret!=0 || (ret=msxGameReaderReadMemory(handleId,byte16k,0xa000,0x200),ret!=0)) return 0;
							do
                    		{
          						byte16k2[ret]=~byte16k[ret];
         						ret=ret+1;
                    		}while(ret<0x200);
                    		ret=msxGameReaderWriteMemory(handleId,byte16k2,0xa000,0x200);
                    		if(ret!=0) return 0;
                    		byteWrite=0;
                    		ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
                    		if(ret!=0 || (ret=msxGameReaderReadMemory(handleId,byte16k2,0xa000,0x200),ret!=0) || (ret=msxGameReaderReadMemory(handleId,byte16k3,0x8000,0x200),ret!=0) || (ret=msxGameReaderWriteMemory(handleId,byte16k,0xa000,0x200),ret!=0)) return 0;
                			ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
                			if(ret!=0) return 0;
                			ret=0x80;
                    		bVar8=true;
                    		piVar7=byte16k3;
                    		piVar5=byte16k2;
                    		do
                    		{
                    			if(ret==0) break;
                    			ret=ret-1;
                    			bVar8=*piVar5==*piVar7;
                    			piVar7=piVar7+1;
                    			piVar5=piVar5+1;
                    		}while(bVar8);
                    		if(bVar8) break;
                    		ret=0;
                    		do
                    		{
          						byte16k2[ret]=~byte16k2[ret];
         						ret=ret+1;
                    		}while(ret<0x200);
                    		ret=0x80;
                    		bVar8=true;
                    		piVar6=byte16k;
                    		piVar5=byte16k2;
                    		do
                    		{
                    			if(ret==0) break;
                    			ret=ret-1;
                    			bVar8=*piVar5==*piVar6;
                    			piVar6=piVar6+1;
                    			piVar5=piVar5+1;
                    		}while(bVar8);
                    		kk=kk*2;
							if(bVar8!=0 || mgr[handleId]->sramBank>=kk ) break;
						}while(true);
					}
					mgr[handleId]->sramSize=kk;
					if(mgr[handleId]->romSize>mgr[handleId]->sramBank)
					{
						mgr[handleId]->romSize=mgr[handleId]->sramBank;
					}
					byteWrite=0;
					ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7000,1);
					if(ret!=0) return 0;
					ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7800,1);
					if(ret==0) return 1;
					return 0;
				case 6:
					puVar4=0x80;
					kk=0x80;
					do
					{
						byteWrite=puVar4;
						ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7000,1);
						if(ret!=0) return 0;
						byteWrite=0;
						ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
						if(ret!=0 || (ret=msxGameReaderReadMemory(handleId,byte16k,0x8000,0x200),ret!=0)) return 0;
						do
                    	{
          					byte16k2[ret]=~byte16k[ret];
         					ret=ret+1;
                    	}while(ret<0x200);
                    	ret=msxGameReaderWriteMemory(handleId,byte16k2,0x8000,0x200);
                    	ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
                    	if(ret!=0 || (ret=msxGameReaderReadMemory(handleId,byte16k2,0x8000,0x200),ret!=0) || (msxGameReaderWriteMemory(handleId,byte16k,0x8000,0x200),ret!=0)) return 0;
                    	ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
                    	if(ret!=0) return 0;
                    	do
                    	{
          					byte16k2[ret]=~byte16k2[ret];
         					ret=ret+1;
                    	}while(ret<0x200);
                    	ret=0x80;
                    	bVar8=true;
                    	piVar6=byte16k;
                    	piVar5=byte16k2;
                    	do
                    	{
                    		if(ret==0) break;
                    		ret=ret-1;
                    		bVar8=*piVar5==*piVar6;
                    		piVar6=piVar6+1;
                    		piVar5=piVar5+1;
                    	}while(bVar8);
                    	if(bVar8)
                    	{
                    		ret=msxGameReaderReadMemory(handleId,byte16k2,0x8000,0x200);
                    		if(ret!=0) return 0;
                    		ret=0x80;
                    		bVar8=true;
                    		piVar6=byte16k;
                    		piVar5=byte16k2;
                    		do
                    		{
                    			if(ret==0) break;
                    			ret=ret-1;
                    			bVar8=*piVar5==*piVar6;
                    			piVar6=piVar6+1;
                    			piVar5=piVar5+1;
                    		}while(bVar8);
                    		if(bVar8) break;
                    	}
                    	puVar4=puVar4>>1;

					}while(puVar4!=0);
					mgr[handleId]->sramSize=0;
					mgr[handleId]->sramBank=puVar4;
					if(puVar4==0) return 1;
					byteWrite=puVar4;
					ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x6000,1);
					if(ret!=0) return 0;
					kk=1;
					if(mgr[handleId]->sramBank>1)
					{
						do
						{
							byteWrite=mgr[handleId]->sramBank+kk;
							ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7000,1);
							if(ret!=0) return 0;
							byteWrite=0;
							ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
							if(ret!=0 || (ret=msxGameReaderReadMemory(handleId,byte16k,0x8000,0x200),ret!=0)) return 0;
							do
                    		{
          						byte16k2[ret]=~byte16k[ret];
         						ret=ret+1;
                    		}while(ret<0x200);
                    		ret=msxGameReaderWriteMemory(handleId,byte16k2,0x8000,0x200);
                    		if(ret!=0) return 0;
                    		byteWrite=0;
                    		ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
                    		if(ret!=0 || (ret=msxGameReaderReadMemory(handleId,byte16k2,0x8000,0x200),ret!=0) || (ret=msxGameReaderReadMemory(handleId,byte16k3,0x4000,0x200),ret!=0) || (ret=msxGameReaderWriteMemory(handleId,byte16k,0x8000,0x200),ret!=0)) return 0;
                    		ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
                    		if(ret!=0) return 0;
                    		ret=0x80;
                    		bVar8=true;
                    		piVar7=byte16k3;
                    		piVar5=byte16k2;
                    		do
                    		{
                    			if(ret==0) break;
                    			ret=ret-1;
                    			bVar8=*piVar5==*piVar7;
                    			piVar7=piVar7+1;
                    			piVar5=piVar5+1;
                    		}while(bVar8);
                    		if(bVar8) break;
                    		ret=0;
                    		do
                    		{
          						byte16k2[ret]=~byte16k2[ret];
         						ret=ret+1;
                    		}while(ret<0x200);
                    		ret=0x80;
                    		bVar8=true;
                    		piVar6=byte16k;
                    		piVar5=byte16k2;
                    		do
                    		{
                    			if(ret==0) break;
                    			ret=ret-1;
                    			bVar8=*piVar5==*piVar6;
                    			piVar6=piVar6+1;
                    			piVar5=piVar5+1;
                    		}while(bVar8);
                    		kk=kk*2;
							if(bVar8!=0 || mgr[handleId]->sramBank<=kk ) break;
						}while(true);
					}
					mgr[handleId]->sramSize=kk*2;
					if(mgr[handleId]->romSize>mgr[handleId]->sramBank)
					{
						mgr[handleId]->romSize=mgr[handleId]->sramBank*2;
					}
					byteWrite=0;
					ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x6000,1);
					if(ret!=0) return 0;
					ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7000,1);
					if(ret==0) return 1;
					return 0;
				case 7:
					mgr[handleId]->sramSize=0;
					mgr[handleId]->sramBank=0;
					byteWrite=0;
					ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x5fff,1);
					if(ret==0)
					{
						ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7ff7,1);
						if(ret==0 && (ret=msxGameReaderReadMemory(handleId,byte16k2,0x4000,0x200),ret==0))
						{
							byteWrite=0x4d;
							ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x5ffe,1);
							if(ret==0)
							{
								byteWrite=0x69;
								ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x5fff,1);
								if(ret==0)
								{
									byteWrite=0;
									ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
									if(ret==0 && (ret=msxGameReaderReadMemory(handleId,byte16k,0x4000,0x200),ret==0))
									{
										ret=0x80;
                    					bVar8=true;
                    					piVar6=byte16k;
                    					piVar5=byte16k2;
                    					do
                    					{
                    						if(ret==0) break;
                    						ret=ret-1;
                    						bVar8=*piVar5==*piVar6;
                    						piVar6=piVar6+1;
                    						piVar5=piVar5+1;
                    					}while(bVar8);
                    					if(!bVar8)
                    					{
                    						ret=0;
                    						do
                    						{
          										byte16k2[ret]=~byte16k[ret];
         										ret=ret+1;
                    						}while(ret<0x200);
                    						msxGameReaderWriteMemory(handleId,byte16k2,0x4000,0x200);
                    						byteWrite=0;
                    						ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
                    						if(ret==0 && (ret=msxGameReaderReadMemory(handleId,byte16k2,0x4000,0x200),ret==0) && (ret=msxGameReaderWriteMemory(handleId,byte16k,0x4000,0x200),ret==0))
                    						{
                    							byteWrite=0;
                    							ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x5fff,1);
                    							if(ret==0)
                    							{
                    								do
                    								{
          												byte16k2[ret]=~byte16k2[ret];
         												ret=ret+1;
                    								}while(ret<0x200);
                    								ret=0x80;
                    								bVar8=true;
                    								piVar6=byte16k;
                    								piVar5=byte16k2;
                    								do
                    								{
                    									if(ret==0) break;
                    									ret=ret-1;
                    									bVar8=*piVar5==*piVar6;
                    									piVar6=piVar6+1;
                    									piVar5=piVar5+1;
                    								}while(bVar8);
                    								if(bVar8)
                    								{
                    									mgr[handleId]->sramSize=1;
                    									mgr[handleId]->sramBank=0xff;
                    									return 1;

                    								}
                    					
                    							}
                    						}
                    					}
									}
								}
							}
						}
					}
					return 0;
				default:
					mgr[handleId]->sramSize=0;
                    mgr[handleId]->sramBank=0;
                    return 1;
			}
		}
	}
	return 0;
}


int msxGameReaderReadRomAll(int handleId,unsigned char *buffer)
{
	int ret;
	unsigned char status[3];
	unsigned char byteWrite;
	unsigned char index;
	unsigned char *buf;
	if(handleId>-1 && handleId<16 && handle[handleId]!=NULL)
	{
		if((ret=msxGameReaderIsSlotEnabled(handleId),ret==1) && (ret=msxGameReaderGetSlotStatus(handleId,status),ret==0) && status[1])
		{
			if(mgr[handleId]->romType>-1)
			{
				switch(mgr[handleId]->romType)
				{
					case 0:
						ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
						if(ret!=0)
						{
							return 0;
						}
						ret=msxGameReaderReadMemory(handleId,buffer,0,0x2000);
						if(ret!=0)
						{
							return 0;
						}
						ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
						if(ret!=0)
						{
							return 0;
						}
						ret=msxGameReaderReadMemory(handleId,buffer+0x2000,0x2000,0x2000);
						if(ret!=0)
						{
							return 0;
						}
						ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
						if(ret!=0)
						{
							return 0;
						}
						ret=msxGameReaderReadMemory(handleId,buffer+0x4000,0x4000,0x2000);
						if(ret!=0)
						{
							return 0;
						}
						ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
						if(ret!=0)
						{
							return 0;
						}
						ret=msxGameReaderReadMemory(handleId,buffer+0x6000,0x6000,0x2000);
						if(ret!=0)
						{
							return 0;
						}
						ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
						if(ret!=0)
						{
							return 0;
						}
						ret=msxGameReaderReadMemory(handleId,buffer+0x8000,0x8000,0x2000);
						if(ret!=0)
						{
							return 0;
						}
						ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
						if(ret!=0)
						{
							return 0;
						}
						ret=msxGameReaderReadMemory(handleId,buffer+0xa000,0xa000,0x2000);
						if(ret!=0)
						{
							return 0;
						}
						ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
						if(ret!=0)
						{
							return 0;
						}
						ret=msxGameReaderReadMemory(handleId,buffer+0xc000,0xc000,0x2000);
						if(ret!=0)
						{
							return 0;
						}
						ret=msxGameReaderWriteMemory(handleId,&byteWrite,0,1);
						if(ret!=0)
						{
							return 0;
						}
						ret=msxGameReaderReadMemory(handleId,buffer+0xe000,0xe000,0x2000);
						if(ret==0)
						{
							return 1;
						}
						return 1;
					case 1:
						index=0;
						buf=buffer;
						if(mgr[handleId]->romSize>0)
						{
							do
							{
								byteWrite=index;
								ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x8000,1);
								if(ret!=0)
								{
									return 0;
								}
								byteWrite=index+1;
								ret=msxGameReaderWriteMemory(handleId,&byteWrite,0xa000,1);
								if(ret!=0)
								{
									return 0;
								}
								ret=msxGameReaderReadMemory(handleId,buf,0x8000,0x4000);
								if(ret!=0)
								{
									return 0;
								}
								index=index+2;
								buf=buf+0x4000;

							}while(index<mgr[handleId]->romSize);
						}
						byteWrite=0;
						ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x8000,1);
						if(ret!=0)
						{
							return 0;
						}
						ret=msxGameReaderWriteMemory(handleId,&byteWrite,0xa000,1);
						if(ret==0)
						{
							return 1;
						}
						return 0;
					case 2:
						index=0;
						buf=buffer;
						if(mgr[handleId]->romSize/2>0)
						{
							do
							{
								byteWrite=index;
								ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x8000,1);
								if(ret!=0)
								{
									return 0;
								}
								ret=msxGameReaderReadMemory(handleId,buf,0x8000,0x4000);
								if(ret!=0)
								{
									return 0;
								}
								index=index+1;
								buf=buf+0x4000;

							}while(index<mgr[handleId]->romSize/2);
						}
						byteWrite=0;
						ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x8000,1);
						if(ret==0)
						{
							return 1;
						}
						return 0;
					case 3:
						index=0;
						buf=buffer;
						if(mgr[handleId]->romSize>0)
						{
							do
							{
								byteWrite=index;
								ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x5000,1);
								if(ret!=0)
								{
									return 0;
								}
								byteWrite=index+1;
								ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7000,1);
								if(ret!=0)
								{
									return 0;
								}
								ret=msxGameReaderReadMemory(handleId,buf,0x4000,0x4000);
								if(ret!=0)
								{
									return 0;
								}
								index=index+2;
								buf=buf+0x4000;

							}while(index<mgr[handleId]->romSize);
						}
						byteWrite=0;
						ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x5000,1);
						if(ret!=0)
						{
							return 0;
						}
						byteWrite=1;
						ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7000,1);
						if(ret==0)
						{
							return 1;
						}
						return 0;
					case 4:
						index=0;
						buf=buffer;
						if(mgr[handleId]->romSize>0)
						{
							do
							{
								byteWrite=index;
								ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x8000,1);
								if(ret!=0)
								{
									return 0;
								}
								byteWrite=index+1;
								ret=msxGameReaderWriteMemory(handleId,&byteWrite,0xa000,1);
								if(ret!=0)
								{
									return 0;
								}
								ret=msxGameReaderReadMemory(handleId,buf,0x8000,0x4000);
								if(ret!=0)
								{
									return 0;
								}
								index=index+2;
								debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s index: %d %d\n",__func__,index,index*0x4000);

								buf=buf+0x4000;
								//debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s jur jur\n",__func__);

							}while(index<mgr[handleId]->romSize);
						}
						byteWrite=2;
						ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x8000,1);
						if(ret!=0)
						{
							return 0;
						}
						byteWrite=3;
						ret=msxGameReaderWriteMemory(handleId,&byteWrite,0xa000,1);
						if(ret==0)
						{
							return 1;
						}
						return 0;
					case 5:
						index=0;
						buf=buffer;
						if(mgr[handleId]->romSize>0)
						{
							do
							{
								byteWrite=index;
								ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7000,1);
								if(ret!=0)
								{
									return 0;
								}
								byteWrite=index+1;
								ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7800,1);
								if(ret!=0)
								{
									return 0;
								}
								ret=msxGameReaderReadMemory(handleId,buf,0x8000,0x4000);
								if(ret!=0)
								{
									return 0;
								}
								index=index+2;
								buf=buf+0x4000;

							}while(index<mgr[handleId]->romSize);
						}
						byteWrite=0;
						ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x6000,1);
						if(ret!=0)
						{
							return 0;
						}
						byteWrite=0;
						ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x6800,1);
						if(ret==0)
						{
							return 1;
						}
						return 0;	
					case 6:
						index=0;
						buf=buffer;
						if(mgr[handleId]->romSize/2>0)
						{
							do
							{
								byteWrite=index;
								ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7000,1);
								if(ret!=0)
								{
									return 0;
								}
								ret=msxGameReaderReadMemory(handleId,buf,0x8000,0x4000);
								if(ret!=0)
								{
									return 0;
								}
								index=index+1;
								buf=buf+0x4000;

							}while(index<mgr[handleId]->romSize/2);
						}
						byteWrite=0;
						ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7000,1);
						if(ret==0)
						{
							return 1;
						}
						return 0;
					case 7:
						byteWrite=0x10;
						ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7ff6,1);
						if(ret!=0)
						{
							return 0;
						}
						byteWrite=0;
						ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7ff6,1);
						if(ret!=0)
						{
							return 0;
						}
						index=0;
						buf=buffer;
						if(mgr[handleId]->romSize/2>0)
						{
							do
							{
								byteWrite=index;
								ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7ff7,1);
								if(ret!=0)
								{
									return 0;
								}
								ret=msxGameReaderReadMemory(handleId,buf,0x4000,0x4000);
								if(ret!=0)
								{
									return 0;
								}
								index=index+1;
								buf=buf+0x4000;

							}while(index<mgr[handleId]->romSize/2);
						}
						byteWrite=0;
						ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7ff7,1);
						if(ret==0)
						{
							return 1;
						}
						return 0;
					default:
						return 0;
				}
			}
		}
	}
	return 0;
}



int msxGameReaderReadSramAll(int handleId,unsigned char *buffer)
{
	int ret;
	unsigned char status[3];
	unsigned char byteWrite;
	unsigned char index;
	unsigned char *buf;
	if(handleId>-1 && handleId<16 && handle[handleId]!=NULL)
	{
		if((ret=msxGameReaderIsSlotEnabled(handleId),ret==1) && (ret=msxGameReaderGetSlotStatus(handleId,status),ret==0) && status[1])
		{
			if(mgr[handleId]->sramSize<1 || mgr[handleId]->sramBank<1)
			{
				return 0;
			}
			switch(mgr[handleId]->romType)
			{
				case 4:
					byteWrite=0x10;
					ret=msxGameReaderWriteMemory(handleId,&byteWrite,0xa000,1);
					if(ret!=0)
					{
						return 0;
					}
					ret=msxGameReaderReadMemory(handleId,buffer,0xa000,0x2000);
					if(ret!=0)
					{
						return 0;
					}
					byteWrite=0;
					ret=msxGameReaderWriteMemory(handleId,&byteWrite,0xa000,1);
					if(ret!=0)
					{
						return 0;
					}
					ret=msxGameReaderReadMemory(handleId,buffer+0x2000,0xa000,0x2000);
					if(ret!=0)
					{
						return 0;
					}
					byteWrite=0;
					ret=msxGameReaderWriteMemory(handleId,&byteWrite,0xa000,1);
					if(ret==0)
					{
						return 1;
					}
					return 0;
				case 5:
					if(mgr[handleId]->sramSize>0)
					{
						index=0;
						buf=buffer;
						do
						{	
							byteWrite=mgr[handleId]->sramBank+index;
							ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7000,1);
							if(ret!=0)
							{
								return 0;
							}
							ret=msxGameReaderReadMemory(handleId,buf,0x8000,0x2000);
							if(ret!=0)
							{
								return 0;
							}
							index=index+1;
							buf=buf+0x2000;
						}while(index<mgr[handleId]->sramSize);	
					}
					byteWrite=0;
					ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7000,1);
					if(ret==0)
					{
						return 1;
					}
					return 0;
				case 6:
					if(mgr[handleId]->sramSize/2>0)
					{
						index=0;
						buf=buffer;
						do
						{	
							byteWrite=mgr[handleId]->sramBank+index;
							ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7000,1);
							if(ret!=0)
							{
								return 0;
							}
							ret=msxGameReaderReadMemory(handleId,buf,0x8000,0x4000);
							if(ret!=0)
							{
								return 0;
							}
							index=index+1;
							buf=buf+0x4000;
						}while(index<mgr[handleId]->sramSize/2);
						
					}
					byteWrite=0;
					ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x7000,1);
					if(ret==0)
					{
						return 1;
					}
					return 0;
				case 7:
					byteWrite=0x4d;
					ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x5ffe,1);
					if(ret!=0)
					{
						return 0;
					}
					byteWrite=0x69;
					ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x5fff,1);
					if(ret!=0)
					{
						return 0;
					}
					ret=msxGameReaderReadMemory(handleId,buffer,0x4000,0x2000);
					if(ret!=0)
					{
						return 0;
					}
					byteWrite=0;
					ret=msxGameReaderWriteMemory(handleId,&byteWrite,0x5fff,1);
					if(ret!=0)
					{
						return 0;
					}
					return 1;
				default:
					return 0;
			}
		}
	}
	return 0;
	
}

int main()
{

	int ret;
	int i;
	int counter=0;
	usbInit(3);
	ret=msxGameReaderInit();
	if(ret!=0)
	{
		usbFinish();
		return 0;
	}
	debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s Msx Game Reader number: %d enabled=%d inserted=%d slot=%d\n",__func__,0,mgr[0]->enabled,mgr[0]->inserted,mgr[0]->slot);

	msxGameReaderAnalyzeRomType(0);
	msxGameReaderAnalyzeRomSize(0);
	debugNetPrintf(DEBUG,"[MSXGAMEREADER] %s Msx Game Reader rom size: %d\n",__func__,mgr[0]->romSize);
	//ugly test with metalgear RC-750
	mgr[0]->rom=(unsigned char *)malloc(mgr[0]->romSize*16384);
	msxGameReaderReadRomAll(0,mgr[0]->rom);
	int fd;
	fd=open("mgear.rom",O_WRONLY | O_APPEND | O_CREAT, 0644);
	if(fd>0)
	{
		write(fd,mgr[0]->rom,mgr[0]->romSize*1024);
	}
	close(fd);

	

	usbFinish();
	return 0;
}