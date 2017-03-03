/*
             LUFA Library
     Copyright (C) Dean Camera, 2012.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2012  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

#include "TestAndMeasurement.h"
#include <stdlib.h>

/** Contains the (usually static) capabilities of the TMC device. This table is requested by the
 *  host upon enumeration to give it information on what features of the Test and Measurement USB
 *  Class the device supports.
 */
TMC_Capabilities_t Capabilities =
	{
		.Status     = TMC_STATUS_SUCCESS,
		.TMCVersion = VERSION_BCD(1.00),

		.Interface  =
			{
				.ListenOnly             = false,
				.TalkOnly               = false,
				.PulseIndicateSupported = false,
			},

		.Device     =
			{
				.SupportsAbortINOnMatch = false,
			},
	};

/** Current TMC control request that is being processed */
static uint8_t RequestInProgress = 0;

/** Stream callback abort flag for bulk IN data */
static bool IsTMCBulkINReset = false;

/** Stream callback abort flag for bulk OUT data */
static bool IsTMCBulkOUTReset = false;

/** Last used tag value for data transfers */
static uint8_t CurrentTransferTag = 0;

/** Length of last data transfer, for reporting to the host in case an in-progress transfer is aborted */
static uint16_t LastTransferLength;

/** Buffer to hold the next message to sent to the TMC host */
static uint8_t NextResponseBuffer[255];

/** Indicates the length of the next response to send */
static uint8_t NextResponseLen, readCMD;

/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */

int main(void) {
	SetupHardware();
	sei();

	while(1) {
		TMC_Task();
		USB_USBTask();
        readData();
	}
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void) {
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	/* Hardware Initialization */
	USB_Init();
    
    /* Initialize ADC */
    initADC();
}

/** Event handler for the USB_Connect event. This indicates that the device is enumerating via the status LEDs and
 *  starts the library USB task to begin the enumeration and USB management process.
 */
void EVENT_USB_Device_Connect(void) {
}

/** Event handler for the USB_Disconnect event. This indicates that the device is no longer connected to a host via
 *  the status LEDs and stops the USB management and CDC management tasks.
 */
void EVENT_USB_Device_Disconnect(void) {
}

/** Event handler for the USB_ConfigurationChanged event. This is fired when the host set the current configuration
 *  of the USB device after enumeration - the device endpoints are configured and the CDC management task started.
 */
void EVENT_USB_Device_ConfigurationChanged(void) {
	bool ConfigSuccess = true;

	/* Setup TMC In, Out and Notification Endpoints */
	ConfigSuccess &= Endpoint_ConfigureEndpoint(TMC_NOTIFICATION_EPADDR, EP_TYPE_INTERRUPT, TMC_IO_EPSIZE, 1);
	ConfigSuccess &= Endpoint_ConfigureEndpoint(TMC_IN_EPADDR,  EP_TYPE_BULK, TMC_IO_EPSIZE, 1);
	ConfigSuccess &= Endpoint_ConfigureEndpoint(TMC_OUT_EPADDR, EP_TYPE_BULK, TMC_IO_EPSIZE, 1);
}

/** Event handler for the USB_ControlRequest event. This is used to catch and process control requests sent to
 *  the device from the USB host before passing along unhandled control requests to the library for processing
 *  internally.
 */
void EVENT_USB_Device_ControlRequest(void) {
	uint8_t TMCRequestStatus = TMC_STATUS_SUCCESS;

	/* Process TMC specific control requests */
	switch (USB_ControlRequest.bRequest) {
		case Req_InitiateAbortBulkOut:
			if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_ENDPOINT)) {
				/* Check that no split transaction is already in progress and the data transfer tag is valid */
				if (RequestInProgress != 0) TMCRequestStatus = TMC_STATUS_SPLIT_IN_PROGRESS;
				else if (USB_ControlRequest.wValue != CurrentTransferTag) TMCRequestStatus = TMC_STATUS_TRANSFER_NOT_IN_PROGRESS;
				else {
					/* Indicate that all in-progress/pending data OUT requests should be aborted */
					IsTMCBulkOUTReset = true;

					/* Save the split request for later checking when a new request is received */
					RequestInProgress = Req_InitiateAbortBulkOut;
				}

				Endpoint_ClearSETUP();

				/* Write the request response byte */
				Endpoint_Write_8(TMCRequestStatus);

				Endpoint_ClearIN();
				Endpoint_ClearStatusStage();
			}

			break;
		case Req_CheckAbortBulkOutStatus:
			if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_ENDPOINT)) {
				/* Check that an ABORT BULK OUT transaction has been requested and that the request has completed */
				if (RequestInProgress != Req_InitiateAbortBulkOut) TMCRequestStatus = TMC_STATUS_SPLIT_NOT_IN_PROGRESS;
				else if (IsTMCBulkOUTReset) TMCRequestStatus = TMC_STATUS_PENDING;
				else RequestInProgress = 0;

				Endpoint_ClearSETUP();

				/* Write the request response bytes */
				Endpoint_Write_8(TMCRequestStatus);
				Endpoint_Write_16_LE(0);
				Endpoint_Write_32_LE(LastTransferLength);

				Endpoint_ClearIN();
				Endpoint_ClearStatusStage();
			}

			break;
		case Req_InitiateAbortBulkIn:
			if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_ENDPOINT)) {
				/* Check that no split transaction is already in progress and the data transfer tag is valid */
				if (RequestInProgress != 0) {
					TMCRequestStatus = TMC_STATUS_SPLIT_IN_PROGRESS;
				}
				else if (USB_ControlRequest.wValue != CurrentTransferTag) {
					TMCRequestStatus = TMC_STATUS_TRANSFER_NOT_IN_PROGRESS;
				}
				else {
					/* Indicate that all in-progress/pending data IN requests should be aborted */
					IsTMCBulkINReset = true;

					/* Save the split request for later checking when a new request is received */
					RequestInProgress = Req_InitiateAbortBulkIn;
				}

				Endpoint_ClearSETUP();

				/* Write the request response bytes */
				Endpoint_Write_8(TMCRequestStatus);
				Endpoint_Write_8(CurrentTransferTag);

				Endpoint_ClearIN();
				Endpoint_ClearStatusStage();
			}

			break;
		case Req_CheckAbortBulkInStatus:
			if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_ENDPOINT)) {
				/* Check that an ABORT BULK IN transaction has been requested and that the request has completed */
				if (RequestInProgress != Req_InitiateAbortBulkIn) TMCRequestStatus = TMC_STATUS_SPLIT_NOT_IN_PROGRESS;
				else if (IsTMCBulkINReset) TMCRequestStatus = TMC_STATUS_PENDING;
				else RequestInProgress = 0;

				Endpoint_ClearSETUP();

				/* Write the request response bytes */
				Endpoint_Write_8(TMCRequestStatus);
				Endpoint_Write_16_LE(0);
				Endpoint_Write_32_LE(LastTransferLength);

				Endpoint_ClearIN();
				Endpoint_ClearStatusStage();
			}

			break;
		case Req_InitiateClear:
			if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE)) {
				/* Check that no split transaction is already in progress */
				if (RequestInProgress != 0) Endpoint_Write_8(TMC_STATUS_SPLIT_IN_PROGRESS);
				else {
					/* Indicate that all in-progress/pending data IN and OUT requests should be aborted */
					IsTMCBulkINReset  = true;
					IsTMCBulkOUTReset = true;

					/* Save the split request for later checking when a new request is received */
					RequestInProgress = Req_InitiateClear;
				}

				Endpoint_ClearSETUP();

				/* Write the request response byte */
				Endpoint_Write_8(TMCRequestStatus);

				Endpoint_ClearIN();
				Endpoint_ClearStatusStage();
			}

			break;
		case Req_CheckClearStatus:
			if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE)) {
				/* Check that a CLEAR transaction has been requested and that the request has completed */
				if (RequestInProgress != Req_InitiateClear) TMCRequestStatus = TMC_STATUS_SPLIT_NOT_IN_PROGRESS;
				else if (IsTMCBulkINReset || IsTMCBulkOUTReset) TMCRequestStatus = TMC_STATUS_PENDING;
				else RequestInProgress = 0;

				Endpoint_ClearSETUP();

				/* Write the request response bytes */
				Endpoint_Write_8(TMCRequestStatus);
				Endpoint_Write_8(0);

				Endpoint_ClearIN();
				Endpoint_ClearStatusStage();
			}

			break;
		case Req_GetCapabilities:
			if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE)) {
				Endpoint_ClearSETUP();

				/* Write the device capabilities to the control endpoint */
				Endpoint_Write_Control_Stream_LE(&Capabilities, sizeof(TMC_Capabilities_t));
				Endpoint_ClearOUT();
			}

			break;
	}
}

void ProcessSentMessage(uint8_t* const Data, uint16_t Length) {
    uint8_t val, valid, ss[5], i, j, *cur;
    startS ssval;
    unsigned int value, temp;
    volatile float rate;
    cmd command = CMD_ERR;
    uint8_t *args[10] = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
    
    //Find Command Arguements
    for(i = 0, j = 0; Data[i] != NULL && j < 10; i++) {
        if(isWhiteSpace(Data[i])) {
            Data[i] = NULL;
            val = 1;
        }
        else if (val) {
            args[j++] = &(Data[i]);
            val = 0;
        }
    }
    
    switch (command = findCommand(args[0])) {
        case CMD_RREG:
            //Make sure it is a number
            if (!isNumeric(args[1],0)) {
                command = CMD_ERR;
                break;
            }
            ss[0] = myatoi(args[1], &valid);
            
            if (ss[0] > 26 && valid == NUM_VALID) command = CMD_ERR;
            else {
                NextResponseBuffer[0] = readReg(ss[0]);
                NextResponseBuffer[1] = NULL;
                NextResponseLen = 2;
            }
            break;
        case CMD_ADD:
            //args[1] is number of samples
            //args[2] is channels
            //args[3] is sampling rate
            
            //Preliminary error checking
            if (args[4] != NULL || !isNumeric(args[1],0) || !isNumeric(args[2],0) || !isNumeric(args[3],1)) {
                command = CMD_ERR;
                break;
            }
            
            value = myatoi(args[1],&valid);
            ss[0] = myatoi(args[2],&valid);
            rate = atof(args[3]);
            
            //TODO: error checking for rate vs #channels to ensure max rate is not exceeded
            //Number error checking
            if (value == 0 || ss[0] == 0 || rate == 0) {
                command = CMD_ERR;
                break;
            }
            
            ss[1] = determinePrescale(rate);
            temp = determineCounter(ss[1],rate);
            
            if (add(value,determineADCRate(rate),ss[1], (uint8_t) (temp >> 8), (uint8_t) temp, ss[0])) {
                sprintf(NextResponseBuffer, "Number of Samples: %u\t Channels: %u\t Sample Rate: %f\nPrescale Value: %u\tCounter Value: %u\n",value,ss[0],rate,ss[1],temp);
                NextResponseLen = strlen(NextResponseBuffer)+1;
            }
            else command = CMD_ERR;
            break;
        case CMD_RM:
            if (rm()) strcpy((char*)NextResponseBuffer,(char*)"REMOVED\n");
            else strcpy((char*)NextResponseBuffer,(char*)"NO SAMPLE SETS\n");
            
            NextResponseLen = strlen((char*) NextResponseBuffer)+1;
            break;
        case CMD_STOP:
            if (stop() == STOP) strcpy((char*)NextResponseBuffer,(char*)"STOPPED\n");
            else strcpy((char*)NextResponseBuffer,(char*)"STOPPING\n");
            
            NextResponseLen = strlen((char*) NextResponseBuffer)+1;
            break;
        case CMD_START:
            if ((ssval = start()) == START) strcpy((char*)NextResponseBuffer,(char*)"STARTED\n");
            else if (ssval == STOP) strcpy((char*)NextResponseBuffer,(char*)"NO SAMPLE SETS\n");
            else if (ssval == YIELD) strcpy((char*)NextResponseBuffer,(char*)"STOPPING\n");
            else strcpy((char*)NextResponseBuffer,(char*)"ALREADY GOING\n");
            
            NextResponseLen = strlen((char*) NextResponseBuffer)+1;
            break;
        case CMD_QRY:
            ss[0] = (uint8_t) myatoi(args[1], &valid);
            if (valid != NUM_VALID) {
                command = CMD_ERR;
                NextResponseLen = 4;
            }
            else if (qryDSet(&value, &(ss[0]), &(ss[1]), &(ss[2]), &(ss[3]), &(ss[4])) != NULL) {
                temp = (unsigned int) ((ss[2] << 8) + ss[3]);
                sprintf(NextResponseBuffer, "Number of Samples: %u\tADC Rate: %u\tPrescale: %u\tCompare Value: %u\t Channels: %u\n", value, ss[0], ss[1], temp, ss[4]);
            }
            else if (queue == NULL) strcpy((char*)NextResponseBuffer,(char*)"NO SAMPLE SETS\n");
            else strcpy((char*)NextResponseBuffer,(char*)"DOES NOT EXIST\n");
            
            NextResponseLen = strlen((char*) NextResponseBuffer)+1;
            break;
        case CMD_READ:
            //TODO: switch buffers
            readCMD = 1;
            break;
        default:
            command = CMD_ERR;
            break;
    }
    if (command == CMD_ERR) {
        //strcpy((char*)NextResponseBuffer, (char*) Data);
        strcpy((char*)NextResponseBuffer,(char*)"ERROR");
        NextResponseLen = strlen((char*) NextResponseBuffer)+1;
    }
}

uint8_t GetNextMessage(uint8_t* const Data) {
    uint8_t DataLen,i;

    if (readCMD) {
        DataLen = buf1_end;
        for (i = 0; i < DataLen; i++) Data[i] = buf1[i];
        buf1_end = 0;
    }
    else {
        DataLen = NextResponseLen;
        for (i = 0; i < DataLen; i++) Data[i] = NextResponseBuffer[i];
    }
    readCMD = 0;

	return DataLen;
}

/** Function to manage TMC data transmission and reception to and from the host. */
void TMC_Task(void) {
	/* Device must be connected and configured for the task to run */
	if (USB_DeviceState != DEVICE_STATE_Configured) return;

	TMC_MessageHeader_t MessageHeader;
	uint8_t             rw[255];

	/* Try to read in a TMC message from the interface, process if one is available */
	if (ReadTMCHeader(&MessageHeader)) {
		switch (MessageHeader.MessageID) {
			case TMC_MESSAGEID_DEV_DEP_MSG_OUT:
				LastTransferLength = 0;
				while (Endpoint_Read_Stream_LE(rw, MIN(MessageHeader.TransferSize, sizeof(rw)), &LastTransferLength) ==
				       ENDPOINT_RWSTREAM_IncompleteTransfer) {
					if (IsTMCBulkOUTReset) break;
				}
                
                rw[MessageHeader.TransferSize] = NULL;

				Endpoint_ClearOUT();

				ProcessSentMessage(rw, MessageHeader.TransferSize);
				break;
			case TMC_MESSAGEID_DEV_DEP_MSG_IN:
				Endpoint_ClearOUT();

				MessageHeader.TransferSize = GetNextMessage(rw);
				MessageHeader.MessageIDSpecific.DeviceOUT.LastMessageTransaction = true;
				WriteTMCHeader(&MessageHeader);

				LastTransferLength = 0;
				while (Endpoint_Write_Stream_LE(rw, MessageHeader.TransferSize, &LastTransferLength) ==
				       ENDPOINT_RWSTREAM_IncompleteTransfer) {
					if (IsTMCBulkINReset) break;
				}

				Endpoint_ClearIN();
				break;
			default:
				Endpoint_StallTransaction();
				break;
		}
	}

	/* All pending data has been processed - reset the data abort flags */
	IsTMCBulkINReset  = false;
	IsTMCBulkOUTReset = false;
}

/** Attempts to read in the TMC message header from the TMC interface.
 *
 *  \param[out] MessageHeader  Pointer to a location where the read header (if any) should be stored
 *
 *  \return Boolean true if a header was read, false otherwise
 */
bool ReadTMCHeader(TMC_MessageHeader_t* const MessageHeader) {
	uint16_t BytesTransferred;
	uint8_t  ErrorCode;

	/* Select the Data Out endpoint */
	Endpoint_SelectEndpoint(TMC_OUT_EPADDR);

	/* Abort if no command has been sent from the host */
	if (!(Endpoint_IsOUTReceived())) return false;

	/* Read in the header of the command from the host */
	BytesTransferred = 0;
	while ((ErrorCode = Endpoint_Read_Stream_LE(MessageHeader, sizeof(TMC_MessageHeader_t), &BytesTransferred)) ==
	       ENDPOINT_RWSTREAM_IncompleteTransfer) {
		if (IsTMCBulkOUTReset) break;
	}

	/* Store the new command tag value for later use */
	CurrentTransferTag = MessageHeader->Tag;

	/* Indicate if the command has been aborted or not */
	return (!(IsTMCBulkOUTReset) && (ErrorCode == ENDPOINT_RWSTREAM_NoError));
}

bool WriteTMCHeader(TMC_MessageHeader_t* const MessageHeader) {
	uint16_t BytesTransferred;
	uint8_t  ErrorCode;

	/* Set the message tag of the command header */
	MessageHeader->Tag        =  CurrentTransferTag;
	MessageHeader->InverseTag = ~CurrentTransferTag;

	/* Select the Data In endpoint */
	Endpoint_SelectEndpoint(TMC_IN_EPADDR);

	/* Send the command header to the host */
	BytesTransferred = 0;
	while ((ErrorCode = Endpoint_Write_Stream_LE(MessageHeader, sizeof(TMC_MessageHeader_t), &BytesTransferred)) ==
	       ENDPOINT_RWSTREAM_IncompleteTransfer) {
		if (IsTMCBulkINReset) break;
	}

	/* Indicate if the command has been aborted or not */
	return (!(IsTMCBulkINReset) && (ErrorCode == ENDPOINT_RWSTREAM_NoError));
}
