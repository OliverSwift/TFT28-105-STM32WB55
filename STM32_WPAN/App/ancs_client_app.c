/**
 ******************************************************************************
 * @file    ancs_client_app.c
 * @author  MCD Application Team
 * @brief   ANCS client Application
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
 *
 ******************************************************************************
 */


/* Includes ------------------------------------------------------------------*/
#include "app_common.h"
#include "dbg_trace.h"

#include "ble.h"
#include "app_ble.h"    
#include "ancs_client_app.h"

#include "stm32_seq.h"
#include "app.h"
#include "main.h"

/* Private defines -----------------------------------------------------------*/

/* MAX Number of entry in the context lists */
#define MAX_NMB_NOTIFY         128
#define MAX_DISPLAY_NAME_LEN   128
#define MAX_DATA_LIST_LEN      0xFF
#define MAX_DATA_LEN           0x0128

static ANCS_Notification lastNotification;

/**
 * The default GATT command timeout is set to 30s
 */
#define GATT_DEFAULT_TIMEOUT (30000)


/* Invalid Notify Entry */
#define INVALID_NOTIFY_ENTRY   0xFFFF

#define UNPACK_2_BYTE_PARAMETER(ptr)  \
		(uint16_t)((uint16_t)(*((uint8_t *)ptr))) |   \
		(uint16_t)((((uint16_t)(*((uint8_t *)ptr + 1))) << 8))

/* copy UUID to tab */
#define COPY_UUID_128(uuid_struct, uuid_15, uuid_14, uuid_13, uuid_12, uuid_11, uuid_10, uuid_9, uuid_8, uuid_7, uuid_6, uuid_5, uuid_4, uuid_3, uuid_2, uuid_1, uuid_0) \
		do {\
			uuid_struct[0] = uuid_0; uuid_struct[1] = uuid_1; uuid_struct[2] = uuid_2; uuid_struct[3] = uuid_3; \
			uuid_struct[4] = uuid_4; uuid_struct[5] = uuid_5; uuid_struct[6] = uuid_6; uuid_struct[7] = uuid_7; \
			uuid_struct[8] = uuid_8; uuid_struct[9] = uuid_9; uuid_struct[10] = uuid_10; uuid_struct[11] = uuid_11; \
			uuid_struct[12] = uuid_12; uuid_struct[13] = uuid_13; uuid_struct[14] = uuid_14; uuid_struct[15] = uuid_15; \
		}while(0)

/* UUIDs list for ANCS service and characteristics*/
#define COPY_ANCS_SERVICE_UUID(uuid_struct)                      COPY_UUID_128(uuid_struct, 0x79, 0x05, 0xF4, 0x31, 0xB5, 0xCE, 0x4E, 0x99, 0xA4, 0x0F, 0x4B, 0x1E, 0x12, 0x2D, 0x00, 0xD0)
#define COPY_ANCS_NOTIFICATION_SOURCE_UUID(uuid_struct)          COPY_UUID_128(uuid_struct, 0x9F, 0xBF, 0x12, 0x0D, 0x63, 0x01, 0x42, 0xD9, 0x8C, 0x58, 0x25, 0xE6, 0x99, 0xA2, 0x1D, 0xBD)
#define COPY_ANCS_DATA_SOURCE_UUID(uuid_struct)                  COPY_UUID_128(uuid_struct, 0x22, 0xEA, 0xC6, 0xE9, 0x24, 0xD6, 0x4B, 0xB5, 0xBE, 0x44, 0xB3, 0x6A, 0xCE, 0x7C, 0x7B, 0xFB)
#define COPY_ANCS_CONTROL_POINT_UUID(uuid_struct)                COPY_UUID_128(uuid_struct, 0x69, 0xD1, 0xD8, 0xF3, 0x45, 0xE1, 0x49, 0xA8, 0x98, 0x21, 0x9B, 0xBD, 0xFD, 0xAA, 0xD9, 0xD9)

/* Private typedef -----------------------------------------------------------*/
typedef enum
{
	GATT_PROC_MTU_UPDATE,
	GATT_PROC_DISC_ANCS_SERVICE,
	GATT_PROC_DISC_ALL_PRIMARY_SERVICES,
	GATT_PROC_DISC_ALL_CHARS,
	GATT_PROC_DISC_ALL_DESCS,
	GATT_PROC_ENABLE_ALL_NOTIFICATIONS,
} GattProcId_t;

typedef enum
{
	ANCS_PROC_GET_NOTIFICATION_ATTRIBUTE,
	ANCS_PROC_GET_APP_ATTRIBUTE,
	ANCS_PROC_PERFORM_NOTIFICATION_ACTION,
} AncsProcId_t;

/**
 * structure of fields for GATT notication source characteristic
 */
typedef struct notifyListS {
	uint8_t    used;           /*flag to indicate notification processed or not*/
	EventID    evID;
	EventFlags evFlag;
	CategoryID catID;
	uint8_t    catCount;
	uint32_t   notifUID;
} notifyList_type;

/**
 * structure of ANCS context (containing all needed settings: events state, handlers, inputs...)
 */
typedef struct ancs_contextS {

	ANCS_ProfileState state;
	uint16_t connection_handle;

	uint8_t appDisplayName_len;
	uint8_t appDisplayName[MAX_DISPLAY_NAME_LEN];
	uint8_t AppIdentifierLength;
	uint8_t AppIdentifier[MAX_DISPLAY_NAME_LEN];
	uint8_t list[MAX_DATA_LIST_LEN];

	notifyList_type notifyList[MAX_NMB_NOTIFY];

	uint16_t notifyEntry; /* index of input notification */
	uint8_t notifyShowed; /* flag to indicate notification has already been displayed */
	uint32_t startTime;
	uint8_t genericFlag;  /* action done after notification */

	uint16_t ALLServiceStartHandle;
	uint16_t ALLServiceEndHandle;

	uint16_t GAPServiceStartHandle;
	uint16_t GAPServiceEndHandle;

	uint16_t GAPCentralAddressResolutionCharStartHdle;
	uint16_t GAPCentralAddressResolutionCharValueHdle;
	uint16_t GAPCentralAddressResolutionCharDescHdle;
	uint16_t GAPCentralAddressResolutionCharEndHdle;

	uint16_t GATTServiceStartHandle;
	uint16_t GATTServiceEndHandle;

	uint16_t ServiceChangedCharStartHdle;
	uint16_t ServiceChangedCharValueHdle;
	uint16_t ServiceChangedCharDescHdle;
	uint16_t ServiceChangedCharEndHdle;

	uint16_t ANCSServiceStartHandle;
	uint16_t ANCSServiceEndHandle;

	uint16_t ANCSNotificationSourceCharStartHdle;
	uint16_t ANCSControlPointCharStartHdle;
	uint16_t ANCSDataSourceCharStartHdle;

	uint16_t ANCSNotificationSourceCharValueHdle;
	uint16_t ANCSControlPointCharValueHdle;
	uint16_t ANCSDataSourceCharValueHdle;

	uint16_t ANCSNotificationSourceCharDescHdle;
	uint16_t ANCSControlPointCharDescHdle;
	uint16_t ANCSDataSourceCharDescHdle;

} ancs_context_type;

/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
PLACE_IN_SECTION("BLE_APP_CONTEXT") static ancs_context_type ancs_context;

/* Global variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void gatt_cmd_resp_wait(uint32_t timeout);
static void gatt_cmd_resp_release(uint32_t flag);
static void GattProcReq(GattProcId_t GattProcId);
static void GattParseServicesByUUID(aci_att_find_by_type_value_resp_event_rp0 *pr);
static void GattParseServices(aci_att_read_by_group_type_resp_event_rp0 *pr);
static void GattParseChars(aci_att_read_by_type_resp_event_rp0 *pr);
static void GattParseDescs(aci_att_find_info_resp_event_rp0 *pr);
static void GattParseNotification(aci_gatt_notification_event_rp0 *pr);

static void ANCS_App_Update_Service( void );
static SVCCTL_EvtAckStatus_t ANCS_Client_Event_Handler( void *Event );

void ANCS_Notification_Check(EventFlags EventFlagMask);
void ANCS_Client_Reset( void );
static void AncsProcReq(AncsProcId_t AncsProcId);
static void Ancs_Mgr( void );

/* Functions Definition ------------------------------------------------------*/
/* Private functions ----------------------------------------------------------*/
/**
 * Display Event ID notified in ANCS notification source characteristic
 */
static void ANCS_Show_EventID(EventID evID)
{
	switch(evID) {
	case EventIDNotificationAdded:
		printf("** EventID: Added\r\n");
		break;
	case EventIDNotificationModified:
		printf("** EventID: Modified\r\n");
		break;
	case EventIDNotificationRemoved:
		printf("** EventID: Removed\r\n");
		break;
	}
}

/**
 * Display Event Flag notified in ANCS notification source characteristic
 */
static void ANCS_Show_EventFlag(EventFlags evFlag)
{
	printf("** EventFlags:");
	if (evFlag & EventFlagSilent) 			printf(" Silent");
	if (evFlag & EventFlagImportant) 		printf(" Important");
	if (evFlag & EventFlagPreExisting) 		printf(" PreExisting");
	if (evFlag & EventFlagPositiveAction) 	printf(" PositiveAction");
	if (evFlag & EventFlagNegativeAction) 	printf(" NegativeAction");
	printf("\n");
}

/**
 * Display CategoryID notified in ANCS notification source characteristic
 */
static void ANCS_Show_CategoryID(CategoryID catID)
{
	switch (catID) {
	case CategoryIDOther:
		printf("** CategoryID: Other\r\n");
		break;
	case CategoryIDIncomingCall:
		printf("** CategoryID: Incoming Call\r\n");
		break;
	case CategoryIDMissedCall:
		printf("** CategoryID: Missed Call\r\n");
		break;
	case CategoryIDVoicemail:
		printf("** CategoryID: Voice Mail\r\n");
		break;
	case CategoryIDSocial:
		printf("** CategoryID: Social\r\n");
		break;
	case CategoryIDSchedule:
		printf("** CategoryID: Schedule\r\n");
		break;
	case CategoryIDEmail:
		printf("** CategoryID: Email\r\n");
		break;
	case CategoryIDNews:
		printf("** CategoryID: News\r\n");
		break;
	case CategoryIDHealthAndFitness:
		printf("** CategoryID: Healt and Fitness\r\n");
		break;
	case CategoryIDBusinessAndFinance:
		printf("** CategoryID: Business and Finance\r\n");
		break;
	case CategoryIDLocation:
		printf("** CategoryID: Location\r\n");
		break;
	case CategoryIDEntertainment:
		printf("** CategoryID: Entertainment\r\n");
		break;
	default:
		printf("** CategoryID: Reserved(%d)\r\n", catID);
		break;
	}
}

/**
 * Display Notification AttributeID for Get Notification Attributes command from NC
 */
static void ANCS_Show_Attr(NotificationAttributeID attrID)
{
	switch (attrID) {
	case NotificationAttributeIDAppIdentifier:
		printf("** AppIdentifier: ");
		break;
	case NotificationAttributeIDTitle:
		printf("** Title: ");
		break;
	case NotificationAttributeIDSubtitle:
		printf("** SubTitle: ");
		break;
	case NotificationAttributeIDMessage:
		printf("** Message: ");
		break;
	case NotificationAttributeIDMessageSize:
		printf("** MessageSize: ");
		break;
	case NotificationAttributeIDDate:
		printf("** Date: ");
		break;
	case NotificationAttributeIDPositiveActionLabel:
		printf("** PositiveActionLabel: ");
		break;
	case NotificationAttributeIDNegativeActionLabel:
		printf("** NegativeActionLabel: ");
		break;
	}
}

/**
 * Display App AttributeID used in Get App Attributes command from NC
 */
static void ANCS_Show_AppAttr(AppAttributeID appAttrID)
{
	printf("\r\n*************** App Attribute Infomation Received **********************\r\n");
	switch (appAttrID) {
	case AppAttributeIDDisplayName:
		printf("** DisplayName: ");
		break;
	}
}

/**
 * Display different fields for GATT notication source characteristic received
 */
static void ANCS_Show_Notification(uint16_t index)
{
	printf("\r\n*************** Data Source Detailed Information Received **********************\r\n");
	ANCS_Show_EventID((EventID)ancs_context.notifyList[index].evID);
	ANCS_Show_EventFlag((EventFlags)ancs_context.notifyList[index].evFlag);
	ANCS_Show_CategoryID((CategoryID)ancs_context.notifyList[index].catID);
	printf("** CategoryCount: %d\r\n", ancs_context.notifyList[index].catCount);
	printf("** NotificationUID: 0x%08x\r\n", (int)ancs_context.notifyList[index].notifUID);
}

/**
 * create Get Notification Attributes command to NP from UID notification and action flags inputs
 */
static void ANCS_Cmd_GetNotificationAttr(notificationAttr_type attr)
{
	tBleStatus result = BLE_STATUS_SUCCESS;
	uint8_t CmdSize = 5;
	uint8_t CmdGetNotificationAtt[19]={CommandIDGetNotificationAttributes,
			(uint8_t)(attr.UID >> 0),
			(uint8_t)(attr.UID >> 8),
			(uint8_t)(attr.UID >> 16),
			(uint8_t)(attr.UID >> 24),
	};

	if (attr.appID_flag == TRUE)
	{
		CmdGetNotificationAtt[CmdSize++] = NotificationAttributeIDAppIdentifier;
	}

	if ( (attr.title_flag == TRUE) && (attr.title_max_size != 0x00) )
	{
		CmdGetNotificationAtt[CmdSize++] = NotificationAttributeIDTitle;
		CmdGetNotificationAtt[CmdSize++] = (uint8_t)((attr.title_max_size >> 0) & 0xFF);
		CmdGetNotificationAtt[CmdSize++] = (uint8_t)((attr.title_max_size >> 8) & 0xFF);
	}

	if ( (attr.subtitle_flag == TRUE) && (attr.subtitle_max_size!= 0x00) )
	{
		CmdGetNotificationAtt[CmdSize++] = NotificationAttributeIDSubtitle;
		CmdGetNotificationAtt[CmdSize++] = (uint8_t)((attr.subtitle_max_size >> 0) & 0xFF);
		CmdGetNotificationAtt[CmdSize++] = (uint8_t)((attr.subtitle_max_size >> 8) & 0xFF);
	}

	if ( (attr.message_flag == TRUE) && (attr.message_max_size != 0x00) )
	{
		CmdGetNotificationAtt[CmdSize++] = NotificationAttributeIDMessage;
		CmdGetNotificationAtt[CmdSize++] = (uint8_t)((attr.message_max_size >> 0) & 0xFF);
		CmdGetNotificationAtt[CmdSize++] = (uint8_t)((attr.message_max_size >> 8) & 0xFF);
	}

	if (attr.messageSize_flag == TRUE)
	{
		CmdGetNotificationAtt[CmdSize++] = NotificationAttributeIDMessageSize;
	}

	if (attr.date_flag == TRUE)
	{
		CmdGetNotificationAtt[CmdSize++] = NotificationAttributeIDDate;
	}

	if (attr.positiveAction_flag == TRUE)
	{
		CmdGetNotificationAtt[CmdSize++] = NotificationAttributeIDPositiveActionLabel;
	}

	if (attr.negativeAction_flag == TRUE)
	{
		CmdGetNotificationAtt[CmdSize++] = NotificationAttributeIDNegativeActionLabel;
	}

	result = aci_gatt_write_char_value( ancs_context.connection_handle,
			ancs_context.ANCSControlPointCharValueHdle,
			CmdSize,
			(uint8_t *)CmdGetNotificationAtt);
	gatt_cmd_resp_wait(GATT_DEFAULT_TIMEOUT);
	if (result != BLE_STATUS_SUCCESS)
	{
		APP_DBG_MSG(" aci_gatt_write_char_value cmd NOK status =0x%02X \n",result);
	}

}


/**
 * create Perform Notification Action command from UID notification received and desired action ID
 */
static void ANCS_Cmd_PerformNotificationAction(uint32_t notificationUID, ActionID actID)
{
	tBleStatus result = BLE_STATUS_SUCCESS;
	uint8_t CmdPerformNotificationAction[]={CommandIDPerformNotificationAction,
			(uint8_t)(notificationUID >> 0),
			(uint8_t)(notificationUID >> 8),
			(uint8_t)(notificationUID >> 16),
			(uint8_t)(notificationUID >> 24),
			actID };

	result = aci_gatt_write_char_value(ancs_context.connection_handle,
			ancs_context.ANCSControlPointCharValueHdle,
			sizeof(CmdPerformNotificationAction),
			(uint8_t *)CmdPerformNotificationAction);
	gatt_cmd_resp_wait(GATT_DEFAULT_TIMEOUT);
	if (result != BLE_STATUS_SUCCESS)
	{
		APP_DBG_MSG(" aci_gatt_write_char_value cmd NOK status =0x%02X \n",result);
	}
}

/**
 * create Get App Attributes command to NP for number of wanted AppIdentifiers input
 */
static void ANCS_Cmd_GetAppAttr(uint8_t AppIdentifierLength, char *AppIdentifier/* NULL Terminated*/)
{
	tBleStatus result = BLE_STATUS_SUCCESS;
	uint8_t CmdSize = 0;
	uint8_t CmdGetAppAtt[128]={0};

	CmdGetAppAtt[CmdSize++] = CommandIDGetAppAttributes;

	// Append null terminated app identifier string
	while(*AppIdentifier) {
		CmdGetAppAtt[CmdSize++] = *AppIdentifier++;
	}
	CmdGetAppAtt[CmdSize++] = 0;

	// Attributes we want (only DisplayName since it's the only existing one ATM)
	CmdGetAppAtt[CmdSize++] = AppAttributeIDDisplayName;

	result = aci_gatt_write_char_value( ancs_context.connection_handle,
			ancs_context.ANCSControlPointCharValueHdle,
			CmdSize,
			(uint8_t *)CmdGetAppAtt);
	gatt_cmd_resp_wait(GATT_DEFAULT_TIMEOUT);
	if (result != BLE_STATUS_SUCCESS)
	{
		APP_DBG_MSG(" aci_gatt_write_char_value cmd NOK status =0x%02X \n",result);
	}
}

/**
 * Called by application when user interacts on corresponding notification
 */
static void notificationYesNoCallback(uint32_t notifUID, ActionID actID) {
	lastNotification.notifUID = notifUID;
	lastNotification.actID = actID;
	ancs_context.state = ANCS_PERFORM_NOTIFICATION_ACTION;
	Ancs_Mgr();
}

/**
 * Parser of response for Get Notification Attributes command
 * number of received attributes and attributes list input
 */
static void ANCS_Parse_GetNotificationAttr_Resp(uint32_t  notifUID, uint16_t attrLen, uint8_t  *attrList)
{
	uint16_t len, index;

	// Clear last Notification information
	memset(&lastNotification, 0, sizeof(lastNotification));
	lastNotification.notifUID = notifUID;
	lastNotification.categoryId = ancs_context.notifyList[ancs_context.notifyEntry].catID;
	lastNotification.cb = notificationYesNoCallback;

	/* loop on number of attribute IDs */
	index = 0;

	while (index < attrLen)
	{
		/* attrList[index+0] corresponding to AttributeID x*/
		Osal_MemCpy(&len, &attrList[index+1], 2); /* AttributeID x Value Length */
		if (len == 0)  /* Empty */
		{
			index += 3;
		}
		else
		{
			/* AttributeID x */
			NotificationAttributeID attrID = (NotificationAttributeID)attrList[index];

			ANCS_Show_Attr(attrID); /* Notification AttributeID x*/

			/* AttributeID x Value */
			index++;
			if (len <= sizeof(ancs_context.list))
			{
				Osal_MemCpy(ancs_context.list, &attrList[index+2], len);
				ancs_context.list[len] = '\0';
			}
			else
			{
				APP_DBG_MSG("cut the AttributeID x Value len=%d <= sizeof(ancs_context.list)=%d \n\r",len,sizeof(ancs_context.list));
				Osal_MemCpy(ancs_context.list, &attrList[index+2], sizeof(ancs_context.list)-1);
				ancs_context.list[sizeof(ancs_context.list)-1] = '\0';
			}

			if (len > 0) {
				printf(" %s\r\n", ancs_context.list);
				switch(attrID) {
				case NotificationAttributeIDMessage:
					strncpy(lastNotification.message, (char*)ancs_context.list, MESSAGE_SIZE-1);
					break;
				case NotificationAttributeIDTitle:
					strncpy(lastNotification.title, (char*)ancs_context.list, TITLE_SIZE-1);
					break;
				default:
					break;
				}
			}

			index += 2 + len; /* 2 Bytes Length of AttributeID x Value + Value Length*/
		}
	}// while

	if (!ancs_context.notifyShowed) {
		ANCS_Show_Notification(ancs_context.notifyEntry);
		ancs_context.notifyShowed = TRUE;
		ancs_context.state = ANCS_NOTIFY_USER; // Tell the application that a notification is ready
		ancs_context.notifyList[ancs_context.notifyEntry].used = FALSE;
	} else {
		ancs_context.state = ANCS_IDLE;
	}
}

/**
 * Parser of response for Get App Attributes command
 * number of received attributes and attributes list input
 */
static void ANCS_Parse_GetAppAttr_Resp(uint8_t  commandID, uint16_t attrLen, uint8_t  *attrList)
{
	uint8_t appId;
	uint16_t len, index;

	index = 0;
	if(commandID == CommandIDGetAppAttributes)
	{
		/* loop on number of attribute IDs */
		while (index < attrLen)
		{
			/* attrList[index+0] corresponding to AttributeID x*/
			Osal_MemCpy(&len, &attrList[index+1], 2); /* AttributeID x Value Length */
			if (len == 0)  /* Empty */
			{
				index += 3;
			}
			else
			{
				/* AttributeID x */
				ANCS_Show_AppAttr((AppAttributeID)attrList[index]); /* App AttributeID x */

				/* AttributeID for Application identifier */
				if (attrList[index] == AppAttributeIDDisplayName)
				{
					appId = TRUE;
				} else {
					appId = FALSE;
				}

				/* AttributeID x Value */
				index++;
				if (len <= sizeof(ancs_context.list))
				{
					Osal_MemCpy(ancs_context.list, &attrList[index+2], len);
					ancs_context.list[len] = '\0';
				}
				else
				{
					APP_DBG_MSG("cut the AttributeID x Value len=%d <= sizeof(ancs_context.list)=%d \n\r",len,sizeof(ancs_context.list));
					Osal_MemCpy(ancs_context.list, &attrList[index+2], sizeof(ancs_context.list)-1);
					ancs_context.list[sizeof(ancs_context.list)-1] = '\0';
				}

				index += 2 + len; /* 2 Bytes Length of AttributeID x Value + Value Length*/
				if (appId)
				{
					if ((len+1) <= MAX_DISPLAY_NAME_LEN)
					{
						Osal_MemCpy(ancs_context.appDisplayName, ancs_context.list, len+1);
						ancs_context.appDisplayName_len = len + 1;
					} else {
						Osal_MemCpy(ancs_context.appDisplayName, ancs_context.list, MAX_DISPLAY_NAME_LEN);
						ancs_context.appDisplayName_len = MAX_DISPLAY_NAME_LEN;
					}
				} // appId

				if (len > 0)
					printf(" %s\r\n", ancs_context.list);
			}
		}// while
	}// commandID
}
/**
 * Parser of ANCS Notification Source characteristic received
 * to fill ancs_context.notifyList elements
 */
static void ANCS_Notification_Source_Received_event(
		EventID evID,
		EventFlags evFlag,
		CategoryID catID,
		uint8_t catCount,
		uint32_t notifUID)
{
	uint16_t i;

	printf("*************** Notification Received **********************\r\n");
	ANCS_Show_EventID(evID);
	ANCS_Show_EventFlag(evFlag);
	ANCS_Show_CategoryID(catID);
	printf("** CategoryCount: %d\n", catCount);
	printf("** NotificationUID: 0x%08lx\n", notifUID);

	if (evID == EventIDNotificationRemoved)
	{
		for (i=0; i < MAX_NMB_NOTIFY; i++)
		{
			if (ancs_context.notifyList[i].notifUID == notifUID)
			{
				ancs_context.notifyList[i].used = FALSE;
				printf("** notifyEntry: %d Removed\r\n", i);
				break;
			}
		}
	}
	else
	{
		for (i=0; i < MAX_NMB_NOTIFY; i++)
		{
			if (!ancs_context.notifyList[i].used)
			{
				ancs_context.notifyList[i].used = TRUE;
				ancs_context.notifyList[i].evID = evID;
				ancs_context.notifyList[i].evFlag = evFlag;
				ancs_context.notifyList[i].catID = catID;
				ancs_context.notifyList[i].catCount = catCount;
				ancs_context.notifyList[i].notifUID = notifUID;
				lastNotification.evtId = evID;
				lastNotification.categoryId = catID;
				lastNotification.notifUID = notifUID;
				printf("** notifyEntry: %d Added\r\n", i);
				break;
			}
		}
	}
}

/**
 * check event flag from ANCS Notification Source characteristic received
 * to fill ancs_context and filter silent event flags
 */
void ANCS_Notification_Check(EventFlags EventFlagMask)
{
	ancs_context.notifyEntry = INVALID_NOTIFY_ENTRY;
	for (uint8_t idx=0; idx<MAX_NMB_NOTIFY; idx++)
	{
		if (ancs_context.notifyList[idx].used)
		{
			if((ancs_context.notifyList[idx].evFlag & EventFlagMask))
			{
				ancs_context.notifyEntry = idx;
				ancs_context.notifyShowed = FALSE;
				ancs_context.state = ANCS_GET_NOTIFICATION_ATTRIBUTE;
				APP_DBG_MSG("2. Get More Detailed Information  notifyEntry=%d ==> ANCS_GET_NOTIFICATION_ATTRIBUTE \n\r",ancs_context.notifyEntry);
				break;
			}
		}
	}
	if (ancs_context.notifyEntry == INVALID_NOTIFY_ENTRY)
	{
		APP_DBG_MSG(" ancs_context.notifyEntry == INVALID_NOTIFY_ENTRY => ANCS_IDLE \n\r");
		ancs_context.state = ANCS_IDLE;
	}
}

/* Public functions ----------------------------------------------------------*/

/****************************************************************/
/*                                                              */
/*                      GATT CLIENT PART                           */
/****************************************************************/
/**
 * Reset all ancs_context fields to default values
 */
void ANCS_Client_Reset( void )
{
	memset(&ancs_context, 0x00, sizeof(ancs_context));

	for (uint8_t i=0; i < MAX_NMB_NOTIFY; i++)
	{
		ancs_context.notifyList[i].used = 0x00;
	}

	ancs_context.state = ANCS_IDLE;
	ancs_context.connection_handle = 0xFFFF;
	ancs_context.notifyEntry = INVALID_NOTIFY_ENTRY;

	APP_DBG_MSG("ANCS CLIENT RESET \n\r");

	return;
}
/**
 * function used to set task ANCS_App_Update_Service when processed ANCS_Client_Event_Handler
 */
static void Ancs_Mgr( void )
{
	UTIL_SEQ_SetTask(1 << CFG_TASK_APP_ANCS_UPDATE_SERVICE_ID, CFG_SCH_PRIO_0);
	return;
}

/**
 * function of ANCS service and characteristics init
 */
void ANCS_Client_App_Init( void )
{
	/* register ANCS_Client_Event_Handler to BLE Controller initialization*/
	SVCCTL_RegisterCltHandler(ANCS_Client_Event_Handler);

	UTIL_SEQ_RegTask(1<<CFG_TASK_APP_ANCS_UPDATE_SERVICE_ID, UTIL_SEQ_RFU, ANCS_App_Update_Service);

	/* reset ANCS context */
	ANCS_Client_Reset();

	APP_DBG_MSG("-- ANCS CLIENT INITIALIZED \n\r");

	return;
}

static void gatt_cmd_resp_release(uint32_t flag)
{
	UTIL_SEQ_SetEvt(1 << CFG_IDLEEVT_GATT_PROC_COMPLETE);
	return;
}

static void gatt_cmd_resp_wait(uint32_t timeout)
{
	UTIL_SEQ_WaitEvt(1 << CFG_IDLEEVT_GATT_PROC_COMPLETE);
	return;
}

/**
 * function of GATT service parse by UUID
 */
static void GattParseServicesByUUID(aci_att_find_by_type_value_resp_event_rp0 *pr)
{
	/*
	APP_DBG_MSG("ACI_ATT_FIND_BY_TYPE_VALUE_RESP_VSEVT_CODE - Connection_Handle=0x%04X,Num_of_Handle_Pair=%d\n\r",
			pr->Connection_Handle,
			pr->Num_of_Handle_Pair);
	for(uint8_t NumPair=0;NumPair<pr->Num_of_Handle_Pair;NumPair++)
	{
		APP_DBG_MSG("ACI_ATT_FIND_BY_TYPE_VALUE_RESP_VSEVT_CODE - NumPair=%d Found_Attribute_Handle=0x%04X,Group_End_Handle=0x%04X\n\r",
				NumPair,
				pr->Attribute_Group_Handle_Pair[NumPair].Found_Attribute_Handle,
				pr->Attribute_Group_Handle_Pair[NumPair].Group_End_Handle);
	}
	*/
	/* complete ancs_context fields */
	if(ancs_context.state == ANCS_DISCOVER_ANCS_SERVICE)
	{
		ancs_context.ANCSServiceStartHandle = pr->Attribute_Group_Handle_Pair[0].Found_Attribute_Handle;
		ancs_context.ANCSServiceEndHandle = pr->Attribute_Group_Handle_Pair[0].Group_End_Handle;
		APP_DBG_MSG("ACI_ATT_FIND_BY_TYPE_VALUE_RESP_VSEVT_CODE - Found ANCSServiceStartHandle=0x%04X,ANCSServiceEndHandle=0x%04X\n\r",
				ancs_context.ANCSServiceStartHandle,
				ancs_context.ANCSServiceEndHandle);
	}
}


/**
 * function of GATT service parse
 */
static void GattParseServices(aci_att_read_by_group_type_resp_event_rp0 *pr)
{
	uint16_t uuid,ServiceStartHandle,ServiceEndHandle;
	uint8_t uuid_offset,uuid_size,uuid_short_offset;
	uint8_t i,idx,numServ;

	/*
	APP_DBG_MSG("ACI_ATT_READ_BY_GROUP_TYPE_RESP_VSEVT_CODE - Connection_Handle=0x%04X,Attribute_Data_Length=%d,Data_Length=%d !\n",
			pr->Connection_Handle,
			pr->Attribute_Data_Length,
			pr->Data_Length);
			*/

	/* check connection handle related to response before processing */
	if (ancs_context.connection_handle == pr->Connection_Handle)
	{
		/* Number of attribute value tuples */
		numServ = (pr->Data_Length) / pr->Attribute_Data_Length;

		/* event data in Attribute_Data_List contains:
		 * 2 bytes for start handle
		 * 2 bytes for end handle
		 * 2 or 16 bytes data for UUID
		 */
		if (pr->Attribute_Data_Length == 20) /* we are interested in the UUID is 128 bit.*/
		{
			idx = 16;                /*UUID index of 2 bytes read part in Attribute_Data_List */
			uuid_offset = 4;         /*UUID offset in bytes in Attribute_Data_List */
			uuid_size = 16;          /*UUID size in bytes */
			uuid_short_offset = 12;  /*UUID offset of 2 bytes read part in UUID field*/
		}
		if (pr->Attribute_Data_Length == 6) /* we are interested in the UUID is 16 bit.*/
		{
			idx = 4;
			uuid_offset = 4;
			uuid_size = 2;
			uuid_short_offset = 0;
		}
		UNUSED(idx);
		UNUSED(uuid_size);

		/* Loop on number of attribute value tuples */
		for (i = 0; i < numServ; i++)
		{
			ServiceStartHandle =  UNPACK_2_BYTE_PARAMETER(&pr->Attribute_Data_List[uuid_offset - 4]);
			ServiceEndHandle = UNPACK_2_BYTE_PARAMETER(&pr->Attribute_Data_List[uuid_offset - 2]);
			uuid = UNPACK_2_BYTE_PARAMETER(&pr->Attribute_Data_List[uuid_offset + uuid_short_offset]);

			//APP_DBG_MSG("numServ=%d/%d,short UUID=0x%04X ServiceHandle [0x%04X - 0x%04X] \n\r",	i, numServ, uuid, ServiceStartHandle,ServiceEndHandle);

			/* complete ancs_context fields */
			if ( (ancs_context.ALLServiceStartHandle == 0x0000) || (ServiceStartHandle < ancs_context.ALLServiceStartHandle) )
			{
				ancs_context.ALLServiceStartHandle = ServiceStartHandle;
			}
			if ( (ancs_context.ALLServiceEndHandle== 0x0000) || (ServiceEndHandle > ancs_context.ALLServiceEndHandle) )
			{
				ancs_context.ALLServiceEndHandle = ServiceEndHandle;
			}


			if(uuid == GAP_SERVICE_UUID) /* 0x1800 */ {

				ancs_context.GAPServiceStartHandle = ServiceStartHandle;
				ancs_context.GAPServiceEndHandle = ServiceEndHandle;

				//APP_DBG_MSG("GAP_SERVICE_UUID=0x%04X found [%04X %04X]!\n",uuid,ServiceStartHandle,ServiceEndHandle);
			} else if (uuid == GENERIC_ATTRIBUTE_SERVICE_UUID) /* 0x1801 */ {

				ancs_context.GATTServiceStartHandle = ServiceStartHandle;
				ancs_context.GATTServiceEndHandle = ServiceEndHandle;

				//APP_DBG_MSG("GENERIC_ATTRIBUTE_SERVICE_UUID=0x%04X found [%04X %04X]!\n",uuid,ServiceStartHandle,ServiceEndHandle);
			} else if (uuid == ANCS_SERVICE_UUID) {

				ancs_context.ANCSServiceStartHandle = ServiceStartHandle;
				ancs_context.ANCSServiceEndHandle = ServiceEndHandle;

				APP_DBG_MSG("ANCS_SERVICE_UUID=0x%04X found [%04X %04X]!\n",uuid,ServiceStartHandle,ServiceEndHandle);
			}

			uuid_offset += pr->Attribute_Data_Length;
		}//numServ
	}
	else
	{
		APP_DBG_MSG("ACI_ATT_READ_BY_GROUP_TYPE_RESP_VSEVT_CODE, failed no free index in connection table !\n\r");
	}
}

/**
 * function of GATT characteristics parse
 */
static void GattParseChars(aci_att_read_by_type_resp_event_rp0 *pr)
{
	uint16_t uuid,CharStartHandle,CharValueHandle;
	uint8_t uuid_offset,uuid_size,uuid_short_offset;
	uint8_t i,idx,numHdleValuePair;
	uint8_t CharProperties;

	APP_DBG_MSG("ACI_ATT_READ_BY_TYPE_RESP_VSEVT_CODE - Connection_Handle=0x%x,Handle_Value_Pair_Length=%d,Data_Length=%d\n\r",
			pr->Connection_Handle,
			pr->Handle_Value_Pair_Length,
			pr->Data_Length);

	if (ancs_context.connection_handle == pr->Connection_Handle)
	{
		/* event data in Attribute_Data_List contains:
		 * 2 bytes for start handle
		 * 1 byte char properties
		 * 2 bytes handle
		 * 2 or 16 bytes data for UUID
		 */

		/* Number of attribute value tuples */
		numHdleValuePair = pr->Data_Length / pr->Handle_Value_Pair_Length;

		if (pr->Handle_Value_Pair_Length == 21) /* we are interested in  128 bit UUIDs */
		{
			idx = 17;                /*UUID index of 2 bytes read part in Attribute_Data_List */
			uuid_offset = 5;         /*UUID offset in bytes in Attribute_Data_List */
			uuid_size = 16;          /*UUID size in bytes */
			uuid_short_offset = 12;  /*UUID offset of 2 bytes read part in UUID field*/
		}
		if (pr->Handle_Value_Pair_Length == 7) /* we are interested in  16 bit UUIDs */
		{
			idx = 5;
			uuid_offset = 5;
			uuid_size = 2;
			uuid_short_offset = 0;
		}
		UNUSED(idx);
		UNUSED(uuid_size);

		pr->Data_Length -= 1;

		/* Loop on number of attribute value tuples */
		for (i = 0; i < numHdleValuePair; i++)
		{
			CharStartHandle = UNPACK_2_BYTE_PARAMETER(&pr->Handle_Value_Pair_Data[uuid_offset-5]);
			CharProperties = pr->Handle_Value_Pair_Data[uuid_offset-3];
			CharValueHandle = UNPACK_2_BYTE_PARAMETER(&pr->Handle_Value_Pair_Data[uuid_offset-2]);
			uuid = UNPACK_2_BYTE_PARAMETER(&pr->Handle_Value_Pair_Data[uuid_offset+uuid_short_offset]);

			if ( (uuid != 0x0) && (CharProperties != 0x0) && (CharStartHandle != 0x0) && (CharValueHandle !=0) )
			{
				//APP_DBG_MSG("-- GATT : numHdleValuePair=%d,short UUID=0x%04X FOUND CharProperties=0x%04X CharHandle [0x%04X - 0x%04X]\n", i, uuid, CharProperties, CharStartHandle,CharValueHandle);

				/* complete ancs_context fields */
				if (uuid == SERVICE_CHANGED_CHARACTERISTIC_UUID) /* 0x2A05 */ {
					ancs_context.ServiceChangedCharStartHdle = CharStartHandle;
					ancs_context.ServiceChangedCharValueHdle = CharValueHandle;
					//APP_DBG_MSG("GATT SERVICE_CHANGED_CHARACTERISTIC_UUID=0x%04X found [%04X %04X]!\n",uuid,CharStartHandle,CharValueHandle);
				} else if (uuid == ANCS_NOTIFICATION_SOURCE_CHAR_UUID) {

					ancs_context.ANCSNotificationSourceCharStartHdle = CharStartHandle;
					ancs_context.ANCSNotificationSourceCharValueHdle = CharValueHandle;

					APP_DBG_MSG("ANCS_NOTIFICATION_SOURCE_CHAR_UUID=0x%04X found [%04X %04X]!\n",uuid,CharStartHandle,CharValueHandle);
				} else if (uuid == ANCS_CONTROL_POINT_CHAR_UUID) {

					ancs_context.ANCSControlPointCharStartHdle = CharStartHandle;
					ancs_context.ANCSControlPointCharValueHdle = CharValueHandle;

					APP_DBG_MSG("ANCS_CONTROL_POINT_CHAR_UUID=0x%04X found [%04X %04X]!\n",uuid,CharStartHandle,CharValueHandle);
				} else if (uuid == ANCS_DATA_SOURCE_CHAR_UUID) {

					ancs_context.ANCSDataSourceCharStartHdle = CharStartHandle;
					ancs_context.ANCSDataSourceCharValueHdle = CharValueHandle;

					APP_DBG_MSG("ANCS_DATA_SOURCE_CHAR_UUID=0x%04X found [%04X %04X]!\n",uuid,CharStartHandle,CharValueHandle);

				} else if (uuid == CENTRAL_ADDRESS_RESOLUTION_UUID) {

					ancs_context.GAPCentralAddressResolutionCharStartHdle = CharStartHandle;
					ancs_context.GAPCentralAddressResolutionCharValueHdle = CharValueHandle;

					//APP_DBG_MSG("CENTRAL_ADDRESS_RESOLUTION_UUID=0x%04X found [%04X %04X]!\n",uuid,CharStartHandle,CharValueHandle);
				} else {
					//APP_DBG_MSG("Unknown_char UUID=0x%04X found [%04X %04X]!\n",uuid,CharStartHandle,CharValueHandle);
				}

			}
			uuid_offset += pr->Handle_Value_Pair_Length;
		}// numHdleValuePair
	}
	else
		APP_DBG_MSG("ACI_ATT_READ_BY_TYPE_RESP_VSEVT_CODE, failed handle not found in connection table !\n\r");
}

/**
 * function of GATT descriptor parse
 */
static void GattParseDescs(aci_att_find_info_resp_event_rp0 *pr)
{
	uint16_t uuid,handle;
	uint8_t uuid_offset,uuid_size,uuid_short_offset;
	uint8_t i,numDesc,handle_uuid_pair_size;
	static uint16_t gCharUUID=0,gCharStartHandle=0,gCharValueHandle=0,gCharDescriptorHandle=0;
	UNUSED(gCharStartHandle);
	UNUSED(gCharDescriptorHandle);
	UNUSED(gCharUUID);

	APP_DBG_MSG("ACI_ATT_FIND_INFO_RESP_VSEVT_CODE - Connection_Handle=0x%x,Format=%d,Event_Data_Length=%d\n\r",
			pr->Connection_Handle,
			pr->Format,
			pr->Event_Data_Length);

	if (ancs_context.connection_handle == pr->Connection_Handle)
	{
		/* event data in Attribute_Data_List contains:
		 * 2 bytes handle
		 * 2 or 16 bytes data for UUID
		 */
		if (pr->Format == UUID_TYPE_16)
		{
			uuid_size = 2;
			uuid_offset = 2;
			uuid_short_offset = 0;
			handle_uuid_pair_size = 4;
		}
		if (pr->Format == UUID_TYPE_128)
		{
			uuid_size = 16;
			uuid_offset = 2;
			uuid_short_offset = 12;
			handle_uuid_pair_size = 18;
		}
		UNUSED(uuid_size);

		/* Number of handle uuid pairs */
		numDesc = (pr->Event_Data_Length) / handle_uuid_pair_size;

		for (i = 0; i < numDesc; i++)
		{
			handle = UNPACK_2_BYTE_PARAMETER(&pr->Handle_UUID_Pair[uuid_offset-2]);
			uuid = UNPACK_2_BYTE_PARAMETER(&pr->Handle_UUID_Pair[uuid_offset+uuid_short_offset]);

			/* 1. primary serice handle + primary serice UUID */
			if(uuid == PRIMARY_SERVICE_UUID)
			{
				APP_DBG_MSG("PRIMARY_SERVICE_UUID=0x%04X handle=0x%04X\n\r",uuid,handle);
			}/* 1. primary serice handle + primary serice UUID */

			/* 2. char handle + char UUID */
			else if(uuid == CHARACTERISTIC_UUID)
			{
				/* reset UUID & handle */
				gCharUUID = 0;
				gCharStartHandle = 0;
				gCharValueHandle = 0;
				gCharDescriptorHandle = 0;

				gCharStartHandle = handle;
				//APP_DBG_MSG("reset CHARACTERISTIC_UUID=0x%04X CharStartHandle=0x%04X\n\r",uuid,handle);
			}/* 2. char handle + char UUID */

			/* 3. char desc handle + char desc UUID */
			else if( (uuid == CHAR_EXTENDED_PROPERTIES_DESCRIPTOR_UUID) /* 0x2900 */
					|| (uuid == CLIENT_CHAR_CONFIG_DESCRIPTOR_UUID) )/* 0x2902 */
			{
				gCharDescriptorHandle = handle;
				if	(gCharValueHandle == ancs_context.ServiceChangedCharValueHdle){

					ancs_context.ServiceChangedCharDescHdle = handle;
					//APP_DBG_MSG("uuid=0x%04X handle=0x%04X-0x%04X-0x%04X\n\r\n\r", uuid,gCharStartHandle,gCharValueHandle,handle);
				} else if	(gCharValueHandle == ancs_context.ANCSNotificationSourceCharValueHdle){

					ancs_context.ANCSNotificationSourceCharDescHdle = handle;
					//APP_DBG_MSG("uuid=0x%04X handle=0x%04X-0x%04X-0x%04X\n\r\n\r", uuid,gCharStartHandle,gCharValueHandle,handle);
				} else if	(gCharValueHandle == ancs_context.ANCSDataSourceCharValueHdle){

					ancs_context.ANCSDataSourceCharDescHdle = handle;
					//APP_DBG_MSG("uuid=0x%04X handle=0x%04X-0x%04X-0x%04X\n\r\n\r", uuid,gCharStartHandle,gCharValueHandle,handle);
				} else if  (gCharValueHandle == ancs_context.ANCSControlPointCharValueHdle){

					ancs_context.ANCSControlPointCharDescHdle = handle;
					//APP_DBG_MSG("uuid=0x%04X handle=0x%04X-0x%04X-0x%04X\n\r\n\r", uuid,gCharStartHandle,gCharValueHandle,handle);
				} else {
					//APP_DBG_MSG(" unkown_char_desc UUID=0x%04X handle=0x%04X-0x%04X-0x%04X \n\r",uuid,gCharStartHandle,gCharValueHandle,handle);
				}
			}// 3. char desc handle + char desc UUID
			/* 4. char value handle + char UUID */
			else
			{
				gCharUUID = uuid;
				gCharValueHandle = handle;
			}

			uuid_offset += handle_uuid_pair_size;
		}
	}
	else
		APP_DBG_MSG("ACI_ATT_FIND_INFO_RESP_VSEVT_CODE, failed handle not found in connection table !\n\r");
}

static void GattParseNotification(aci_gatt_notification_event_rp0 *pr)
{
	APP_DBG_MSG("ACI_GATT_NOTIFICATION_VSEVT_CODE - Connection_Handle=0x%x,Attribute_Handle=0x%04X,Attribute_Value_Length=%d\n\r",
			pr->Connection_Handle,
			pr->Attribute_Handle,
			pr->Attribute_Value_Length);

	if (ancs_context.connection_handle == pr->Connection_Handle)
	{
		// 1. Incoming Notification
		if (pr->Attribute_Handle == ancs_context.ANCSNotificationSourceCharValueHdle)
		{
			//APP_DBG_MSG("1. Incoming Notification Received BASIC information : \n\r");
			EventID evID       = (EventID)pr->Attribute_Value[0];
			EventFlags evFlag = (EventFlags)pr->Attribute_Value[1];
			CategoryID catID  = (CategoryID)pr->Attribute_Value[2];
			uint8_t catCount   = pr->Attribute_Value[3];
			uint32_t notifUID   = (uint32_t)(*((uint32_t *)&pr->Attribute_Value[4]));
			ANCS_Notification_Source_Received_event(evID, evFlag, catID, catCount, notifUID);

			// 2. Get More Detailed Information
			if( (evID == EventIDNotificationAdded) )
			{
				for (uint8_t idx=0; idx<MAX_NMB_NOTIFY; idx++)
				{
					if (ancs_context.notifyList[idx].used && ancs_context.notifyList[idx].notifUID == notifUID)
					{
							ancs_context.notifyEntry = idx;
							ancs_context.state = ANCS_GET_NOTIFICATION_ATTRIBUTE;
							ancs_context.notifyShowed = FALSE;
							Ancs_Mgr();
							APP_DBG_MSG("2. Get More Detailed Information  notifyEntry=%d ==> ANCS_GET_NOTIFICATION_ATTRIBUTE \n\r",ancs_context.notifyEntry);
							break;
					}
				}
			} else if (evID == EventIDNotificationRemoved) {
				lastNotification.title[0] = 0;
				lastNotification.notifUID = notifUID;
				ancs_context.state = ANCS_NOTIFY_USER;
				Ancs_Mgr();
			}
		}// ANCSNotificationSourceCharValueHdle

		// 3. Parse Detail Infomations, Perform Notification Action or Get APP Attributes
		if (pr->Attribute_Handle == ancs_context.ANCSDataSourceCharValueHdle)
		{
			CommandID cmdID	= (CommandID)pr->Attribute_Value[0];
			uint32_t notifUID	= (uint32_t)0x00000000;

			uint16_t AttributeLength;
			uint8_t *AttributeList;

			char AppIdentifier[128]={0};
			uint16_t AppIdentifierLength;

			/***********************************************************************************/
			if (cmdID == CommandIDGetNotificationAttributes)
			{
				APP_DBG_MSG("3.1 Parse Detail Infomation of Notification Attribute, CommandIDGetNotificationAttributes Response: \n\r");
				notifUID = (uint32_t)(*((uint32_t *)&pr->Attribute_Value[1]));
				AttributeLength = pr->Attribute_Value_Length - (1+4);
				AttributeList = (uint8_t *)&pr->Attribute_Value[1+4];

				// 3.1 Parse Detail Infomation of Notification Attribute
				ANCS_Parse_GetNotificationAttr_Resp(notifUID, AttributeLength, AttributeList);
			}
			UNUSED(notifUID);

			if (cmdID == CommandIDGetAppAttributes)
			{
				APP_DBG_MSG("3.3 Parse Detail Infomation of APP Attribute, CommandIDGetAppAttributes Response: \n\r");
				strcpy(AppIdentifier,(char *)&pr->Attribute_Value[1]);
				AppIdentifierLength = strlen((char *)&pr->Attribute_Value[1]);
				AttributeLength = pr->Attribute_Value_Length - (1+AppIdentifierLength+1);
				AttributeList = (uint8_t *)&pr->Attribute_Value[1+AppIdentifierLength+1];

				// 3.3 Parse Detail Infomation of App Attribute
				ANCS_Parse_GetAppAttr_Resp(cmdID,AttributeLength,AttributeList);
			}

			Ancs_Mgr();
			/***********************************************************************************/
		}/* ANCSDataSourceCharValueHdle */
	}
	else
		APP_DBG_MSG("ACI_GATT_NOTIFICATION_VSEVT_CODE, failed handle not found in connection table !\n\r");
}

/**
 * @brief  ANCS Event handler
 * @param  Event: Address of the buffer holding the Event
 * @retval Ack: Return whether the Event has been managed or not
 */
static SVCCTL_EvtAckStatus_t ANCS_Client_Event_Handler( void *Event )
{
	SVCCTL_EvtAckStatus_t return_value;
	hci_event_pckt * event_pckt;
	evt_blecore_aci * blecore_evt;
	Connection_Context_t Notification;

	return_value = SVCCTL_EvtAckFlowEnable;
	event_pckt = (hci_event_pckt *) (((hci_uart_pckt*) Event)->data);

	switch (event_pckt->evt)
	{
	case HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE:
	{
		blecore_evt = (evt_blecore_aci*) event_pckt->data;
		switch (blecore_evt->ecode)
		{
		case ACI_ATT_READ_BY_GROUP_TYPE_RESP_VSEVT_CODE:
		{
			aci_att_read_by_group_type_resp_event_rp0 *pr = (void*) blecore_evt->data;

			//APP_DBG_MSG(" ACI_ATT_READ_BY_GROUP_TYPE_RESP_VSEVT_CODE\n");
			GattParseServices((aci_att_read_by_group_type_resp_event_rp0 *)pr);
		}
		break;
		case ACI_ATT_READ_BY_TYPE_RESP_VSEVT_CODE:
		{
			aci_att_read_by_type_resp_event_rp0 *pr = (void*) blecore_evt->data;
			//APP_DBG_MSG(" ACI_ATT_READ_BY_TYPE_RESP_VSEVT_CODE\n");
			GattParseChars((aci_att_read_by_type_resp_event_rp0 *)pr);
		}
		break;
		case ACI_ATT_FIND_INFO_RESP_VSEVT_CODE:
		{
			aci_att_find_info_resp_event_rp0 *pr = (void*) blecore_evt->data;
			//APP_DBG_MSG(" ACI_ATT_FIND_INFO_RESP_VSEVT_CODE\n");
			GattParseDescs((aci_att_find_info_resp_event_rp0 *)pr);
		}
		break; /*ACI_ATT_FIND_INFO_RESP_VSEVT_CODE*/

		case ACI_GATT_DISC_READ_CHAR_BY_UUID_RESP_VSEVT_CODE:
		{
			APP_DBG_MSG(" ACI_GATT_DISC_READ_CHAR_BY_UUID_RESP_VSEVT_CODE\n");
		}
		break;

		case ACI_ATT_FIND_BY_TYPE_VALUE_RESP_VSEVT_CODE:
		{
			aci_att_find_by_type_value_resp_event_rp0 *pr = (void*) blecore_evt->data;
			APP_DBG_MSG(" ACI_ATT_FIND_BY_TYPE_VALUE_RESP_VSEVT_CODE\n");
			GattParseServicesByUUID((aci_att_find_by_type_value_resp_event_rp0 *)pr);
		}
		break;

		case ACI_GATT_NOTIFICATION_VSEVT_CODE:
		{
			aci_gatt_notification_event_rp0 *pr = (void*) blecore_evt->data;
			GattParseNotification((aci_gatt_notification_event_rp0 *)pr);
		}
		break;/* end ACI_GATT_NOTIFICATION_VSEVT_CODE */

		case ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE:
		{
			aci_gatt_attribute_modified_event_rp0 *attribute_modified = (aci_gatt_attribute_modified_event_rp0*) blecore_evt->data;
			APP_DBG_MSG("ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE Attr_Handle=0x%04X Offset=0x%04X Attr_Data_Length=0x%04X \n\r",
					attribute_modified->Attr_Handle,
					attribute_modified->Offset,
					attribute_modified->Attr_Data_Length);
		}/* end ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE */
		break;

		case ACI_ATT_EXCHANGE_MTU_RESP_VSEVT_CODE:
		{
			aci_att_exchange_mtu_resp_event_rp0 * exchange_mtu_resp;
			exchange_mtu_resp = (aci_att_exchange_mtu_resp_event_rp0 *)blecore_evt->data;
			APP_DBG_MSG("ACI_ATT_EXCHANGE_MTU_RESP_VSEVT_CODE Connection_Handle=0x%04X Server_RX_MTU = %d ==> ANCS_MTU_EXCHANGE_COMPLETE \n",
					exchange_mtu_resp->Connection_Handle,
					exchange_mtu_resp->Server_RX_MTU );

			Notification.Evt_Opcode = ANCS_MTU_EXCHANGE_COMPLETE;
			Notification.connection_handle = exchange_mtu_resp->Connection_Handle;
			ANCS_App_Notification(&Notification);
		}
		break;

		case ACI_GATT_PROC_COMPLETE_VSEVT_CODE:
		{
			aci_gatt_proc_complete_event_rp0 *pr = (void*) blecore_evt->data;
			if(pr->Error_Code != ERR_CMD_SUCCESS)
			{
				APP_DBG_MSG("ACI_GATT_PROC_COMPLETE_VSEVT_CODE - Connection_Handle=0x%04x,Error_Code=0x%02X (0x41: Failed)\n\r", pr->Connection_Handle,pr->Error_Code);
			}

			if (ancs_context.connection_handle == pr->Connection_Handle)
			{
				gatt_cmd_resp_release(0);
			}
			else
				APP_DBG_MSG("ACI_GATT_PROC_COMPLETE_VSEVT_CODE, failed handle not found in connection table !\n\r");
		}/*ACI_GATT_PROC_COMPLETE_VSEVT_CODE*/
		break;
		case ACI_GATT_ERROR_RESP_VSEVT_CODE:
		{
			aci_gatt_error_resp_event_rp0 *error_resp= (void*) blecore_evt->data;
			APP_DBG_MSG("ACI_GATT_ERROR_RESP_VSEVT_CODE Connection_Handle=0x%04X Req_Opcode=0x%02X Attribute_Handle=0x%04X Error_Code=0x%02X (0x05: Insufficient authentication,0x0A: Attribute not found)\n",
					error_resp->Connection_Handle,error_resp->Req_Opcode,error_resp->Attribute_Handle,error_resp->Error_Code);
		}/*ACI_GATT_ERROR_RESP_VSEVT_CODE*/
		break;

		case ACI_GATT_PROC_TIMEOUT_VSEVT_CODE:
		{
			aci_gap_terminate(ancs_context.connection_handle, 0);
			APP_DBG_MSG("SERIOUS TIMEOUT\n");
			HAL_Delay(100);
			ANCS_Client_Reset();
		}
		break;

		default:
			APP_DBG_MSG("invalid ecode 0x%04X\n",blecore_evt->ecode);
			return_value = SVCCTL_EvtNotAck;
			break;
		}
	}
	break; /* HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE */

	default:
		return_value = SVCCTL_EvtNotAck;
		break;
	}

	return (return_value);
}/* end BLE_CTRL_Event_Acknowledged_Status_t */

void ANCS_App_Update_Service( )
{
	switch(ancs_context.state)
	{
	case ANCS_MTU_UPDATE:
		GattProcReq(GATT_PROC_MTU_UPDATE);
		break;

	case ANCS_DISCOVER_ANCS_SERVICE:
		/* GattProcReq(GATT_PROC_MTU_UPDATE); */ /* enable it if you want to set ATT_MTU */
		GattProcReq(GATT_PROC_DISC_ANCS_SERVICE);
		GattProcReq(GATT_PROC_DISC_ALL_PRIMARY_SERVICES);
		GattProcReq(GATT_PROC_DISC_ALL_CHARS);
		GattProcReq(GATT_PROC_DISC_ALL_DESCS);

		if ( (ancs_context.ANCSServiceStartHandle != 0x0000) && (ancs_context.ANCSServiceEndHandle != 0x0000) )
		{
			APP_BLE_Slave_Security_Request();
		}

		GattProcReq(GATT_PROC_ENABLE_ALL_NOTIFICATIONS);
		break;


	case ANCS_GET_NOTIFICATION_ATTRIBUTE:
		AncsProcReq(ANCS_PROC_GET_NOTIFICATION_ATTRIBUTE);
		break;

	case ANCS_GET_APP_ATTRIBUTE:
		AncsProcReq(ANCS_PROC_GET_APP_ATTRIBUTE);
		break;

	case ANCS_PERFORM_NOTIFICATION_ACTION:
		AncsProcReq(ANCS_PROC_PERFORM_NOTIFICATION_ACTION);
		break;

	case ANCS_NOTIFY_USER:
		TFTShowNotification(&lastNotification);
		break;

	case ANCS_IDLE:
		break;

	default:
		APP_DBG_MSG("invalid ancs_context.state=%d \n\r",ancs_context.state);
		break;
	}
}

static void GattProcReq(GattProcId_t GattProcId)
{
	tBleStatus result = BLE_STATUS_SUCCESS;

	switch (GattProcId)
	{
	case GATT_PROC_MTU_UPDATE:
	{
		ancs_context.state = ANCS_SET_DATA_LENGTH;
		result = hci_le_set_data_length(ancs_context.connection_handle,CFG_BLE_MAX_ATT_MTU,CFG_BLE_MAX_ATT_MTU_TX_TIME);
		if (result == BLE_STATUS_SUCCESS)
		{
			APP_DBG_MSG("ANCS_SET_DATA_LENGTH set data length %d %d ok ==>aci_gatt_exchange_config \n",CFG_BLE_MAX_ATT_MTU,CFG_BLE_MAX_ATT_MTU_TX_TIME);
			ancs_context.state = ANCS_MTU_UPDATE;
			result = aci_gatt_exchange_config(ancs_context.connection_handle);
			gatt_cmd_resp_wait(GATT_DEFAULT_TIMEOUT);
			if (result == BLE_STATUS_SUCCESS)
			{
				APP_DBG_MSG("aci_gatt_exchange_config cmd ok \n\r");
			}
			else
			{
				APP_DBG_MSG("aci_gatt_exchange_config cmd KO result=0x%02x \n\r",result);
			}
		}
	}
	break; /* GATT_PROC_MTU_UPDATE */

	case GATT_PROC_DISC_ANCS_SERVICE:
	{
		ancs_context.state = ANCS_DISCOVER_ANCS_SERVICE;
		/* discover ancs service for discover notification source / data source / control point characteristic */
		UUID_t ancs_service_uuid;
		COPY_ANCS_SERVICE_UUID(ancs_service_uuid.UUID_128);
		result = aci_gatt_disc_primary_service_by_uuid(ancs_context.connection_handle,UUID_TYPE_128,(UUID_t *)&ancs_service_uuid);
		if (result == BLE_STATUS_SUCCESS)
		{
			APP_DBG_MSG("GATT_PROC_DISC_ANCS_SERVICE aci_gatt_disc_primary_service_by_uuid cmd ok\n");
		}
		else
		{
			APP_DBG_MSG("GATT_PROC_DISC_ANCS_SERVICE aci_gatt_disc_primary_service_by_uuid cmd NOK status =0x%02X \n",result);
		}

		gatt_cmd_resp_wait(GATT_DEFAULT_TIMEOUT);
		if ( (ancs_context.ANCSServiceStartHandle == 0x0000) && (ancs_context.ANCSServiceEndHandle == 0x0000) )
		{
			APP_DBG_MSG("GATT_PROC_DISC_ANCS_SERVICE ANCS Service is NOT found !!! ==> connected to Android Phone/Pad \n\r");
		}
		else if ( (ancs_context.ANCSServiceStartHandle != 0x0000) && (ancs_context.ANCSServiceEndHandle != 0x0000) )
		{
			APP_DBG_MSG("GATT_PROC_DISC_ANCS_SERVICE ANCS Service [0x%04X - 0x%04X] is found !!! ==> connected to iOS Phone/Pad \n\r",ancs_context.ANCSServiceStartHandle,ancs_context.ANCSServiceEndHandle);
		}
	}
	break; /* GATT_PROC_DISC_ANCS_SERVICE */

	case GATT_PROC_DISC_ALL_PRIMARY_SERVICES:
	{
		ancs_context.state = ANCS_DISCOVER_ALL_SERVICES;
		/* discover all services */
		result = aci_gatt_disc_all_primary_services(ancs_context.connection_handle);
		gatt_cmd_resp_wait(GATT_DEFAULT_TIMEOUT);
		if (result == BLE_STATUS_SUCCESS)
		{
			APP_DBG_MSG("GATT_PROC_DISC_ALL_PRIMARY_SERVICES ALL services discovered Successfully \n");
		}
		else
		{
			APP_DBG_MSG("GATT_PROC_DISC_ALL_PRIMARY_SERVICES aci_gatt_disc_all_primary_services cmd NOK status =0x%02X \n",result);
		}

		if ( (ancs_context.ANCSServiceStartHandle == 0x0000) && (ancs_context.ANCSServiceEndHandle == 0x0000) )
		{
			APP_DBG_MSG("GATT_PROC_DISC_ALL_PRIMARY_SERVICES ANCS Service is NOT found !!! \n\r");
		}
		else if ( (ancs_context.ANCSServiceStartHandle != 0x0000) && (ancs_context.ANCSServiceEndHandle != 0x0000) )
		{
			APP_DBG_MSG("GATT_PROC_DISC_ALL_PRIMARY_SERVICES ANCS Service [0x%04X - 0x%04X] is found !!! \n\r",ancs_context.ANCSServiceStartHandle,ancs_context.ANCSServiceEndHandle);
			APP_DBG_MSG("GATT_PROC_DISC_ALL_PRIMARY_SERVICES GATT Service [0x%04X - 0x%04X] is found !!! \n\r",ancs_context.GATTServiceStartHandle,ancs_context.GATTServiceEndHandle);
		}
	}
	break; /* GATT_PROC_DISC_ALL_PRIMARY_SERVICES */

	case GATT_PROC_DISC_ALL_CHARS:
	{
		ancs_context.state = ANCS_DISCOVER_ALL_CHARS;
		APP_DBG_MSG("ANCS_DISCOVER_ALL_CHARS connection_handle=0x%04X ALLServiceHandle[%04X - %04X] ANCSServiceHandle[%04X - %04X]\n",
				ancs_context.connection_handle,
				ancs_context.ALLServiceStartHandle,	ancs_context.ALLServiceEndHandle,
				ancs_context.ANCSServiceStartHandle, ancs_context.ANCSServiceStartHandle);

		result = aci_gatt_disc_all_char_of_service(
				ancs_context.connection_handle,
				ancs_context.ALLServiceStartHandle,
				ancs_context.ALLServiceEndHandle);
		gatt_cmd_resp_wait(GATT_DEFAULT_TIMEOUT);
		if (result == BLE_STATUS_SUCCESS)
		{
			APP_DBG_MSG("ALL characteristics discovered Successfully \n\r");
		}
		else
		{
			APP_DBG_MSG("ALL characteristics discovery Failed \n\r");
		}
	}
	break; /* GATT_PROC_DISC_ALL_CHARS */

	case GATT_PROC_DISC_ALL_DESCS:
	{
		ancs_context.state = ANCS_DISCOVER_ALL_CHAR_DESCS;
		APP_DBG_MSG("ANCS_DISCOVER_ALL_CHAR_DESCS [%04X - %04X]\n",
				ancs_context.ALLServiceStartHandle,
				ancs_context.ALLServiceEndHandle);
		result = aci_gatt_disc_all_char_desc(
				ancs_context.connection_handle,
				ancs_context.ALLServiceStartHandle,
				ancs_context.ALLServiceEndHandle);
		gatt_cmd_resp_wait(GATT_DEFAULT_TIMEOUT);
		if (result == BLE_STATUS_SUCCESS)
		{
			APP_DBG_MSG("All characteristic descriptors discovered Successfully \n\r");
		}
		else
		{
			APP_DBG_MSG("All characteristic descriptors discovery Failed \n\r");
		}
	}
	break; /* GATT_PROC_DISC_ALL_DESCS */

	case GATT_PROC_ENABLE_ALL_NOTIFICATIONS:
	{
		uint16_t enable = 0x0001;

		if(ancs_context.ANCSDataSourceCharDescHdle != 0x0000)
		{
			ancs_context.state = ANCS_ENABLE_NOTIFICATION_DATA_SOURCE_DESC;
			APP_DBG_MSG("ANCS_ENABLE_NOTIFICATION_DATA_SOURCE_DESC 0x%04X 0x%04X \n",
					ancs_context.connection_handle,
					ancs_context.ANCSDataSourceCharDescHdle);
			result = aci_gatt_write_char_desc(
					ancs_context.connection_handle,
					ancs_context.ANCSDataSourceCharDescHdle,
					2,
					(uint8_t *) &enable);
			gatt_cmd_resp_wait(GATT_DEFAULT_TIMEOUT);
			if (result == BLE_STATUS_SUCCESS)
			{
				APP_DBG_MSG("ANCSDataSourceCharDescHdle notification enabled Successfully \n\r");
			}
			else
			{
				APP_DBG_MSG("ANCSDataSourceCharDescHdle=0x%04X notification enabled Failed BLE_STATUS_NOT_ALLOWED=0x%02x result=0x%02X\n",
						ancs_context.ANCSDataSourceCharDescHdle,BLE_STATUS_NOT_ALLOWED,result);
			}
		}

		if(ancs_context.ANCSNotificationSourceCharDescHdle != 0x0000)
		{
			ancs_context.state = ANCS_ENABLE_NOTIFICATION_NOTIFICATION_SOURCE_DESC;
			APP_DBG_MSG("ANCS_ENABLE_NOTIFICATION_NOTIFICATION_SOURCE_DESC 0x%04X 0x%04X \n",
					ancs_context.connection_handle,
					ancs_context.ANCSNotificationSourceCharDescHdle);
			result = aci_gatt_write_char_desc(
					ancs_context.connection_handle,
					ancs_context.ANCSNotificationSourceCharDescHdle,
					2,
					(uint8_t *) &enable);
			gatt_cmd_resp_wait(GATT_DEFAULT_TIMEOUT);
			if (result == BLE_STATUS_SUCCESS)
			{
				APP_DBG_MSG("ANCSNotificationSourceCharDescHdle notification enabled Successfully \n\r");
			}
			else
			{
				APP_DBG_MSG("ANCSNotificationSourceCharDescHdle notification enabled Failed result=0x%02X\n",result);
			}
		}
	}
	break; /* GATT_PROC_ENABLE_ALL_NOTIFICATIONS */

	default:
		break;
	}
}

static void AncsProcReq(AncsProcId_t AncsProcId)
{
	switch (AncsProcId)
	{

	case ANCS_PROC_GET_NOTIFICATION_ATTRIBUTE:
	{
		ancs_context.state = ANCS_GET_NOTIFICATION_ATTRIBUTE;
		if ( (ancs_context.notifyEntry == INVALID_NOTIFY_ENTRY) || (ancs_context.notifyEntry >= MAX_NMB_NOTIFY) )
		{
			APP_DBG_MSG("ANCS_GET_NOTIFICATION_ATTRIBUTE INVALID_NOTIFY_ENTRY %d \n\r",ancs_context.notifyEntry);
			break;
		}
		else
		{
			APP_DBG_MSG("ANCS_GET_NOTIFICATION_ATTRIBUTE interact with iOS NotificationID 0x%08lx, retrieve more information\n\r",
					ancs_context.notifyList[ancs_context.notifyEntry].notifUID);
		}

		notificationAttr_type attr;
		attr.UID = ancs_context.notifyList[ancs_context.notifyEntry].notifUID;
		attr.appID_flag = FALSE;
		attr.title_flag = TRUE;
		attr.title_max_size = MAX_DATA_LEN;
		attr.subtitle_flag = TRUE;
		attr.subtitle_max_size = MAX_DATA_LEN;
		attr.message_flag = TRUE;
		attr.message_max_size = MAX_DATA_LEN;
		attr.messageSize_flag = TRUE;
		attr.date_flag = FALSE;
		attr.positiveAction_flag = FALSE;
		attr.negativeAction_flag = FALSE;

		ANCS_Cmd_GetNotificationAttr(attr);
	}
	break; /* ANCS_PROC_GET_NOTIFICATION_ATTRIBUTE */

	case ANCS_PROC_GET_APP_ATTRIBUTE:
	{
		ancs_context.state = ANCS_GET_APP_ATTRIBUTE;
		APP_DBG_MSG("ANCS_GET_APP_ATTRIBUTE AppIdentifierLength=%d AppIdentifier:%s \n\r",
				ancs_context.AppIdentifierLength,ancs_context.AppIdentifier);
		ANCS_Cmd_GetAppAttr(ancs_context.AppIdentifierLength, (char*)ancs_context.AppIdentifier);
	}
	break; /* ANCS_PROC_GET_APP_ATTRIBUTE */

	case ANCS_PROC_PERFORM_NOTIFICATION_ACTION:
	{
		ancs_context.state = ANCS_PERFORM_NOTIFICATION_ACTION;
		ANCS_Cmd_PerformNotificationAction(lastNotification.notifUID, lastNotification.actID);
	}
	break; /* ANCS_PROC_PERFORM_NOTIFICATION_ACTION */

	default:
		break;
	}
}

void ANCS_App_Notification( Connection_Context_t *pNotification )
{
	switch (pNotification->Evt_Opcode)
	{
	case ANCS_CONNECTED:
	{
		ancs_context.connection_handle = pNotification->connection_handle; /* register */
		ancs_context.state = ANCS_DISCOVER_ANCS_SERVICE;
		APP_DBG_MSG("ANCS_CONNECTED ==> ANCS_DISCOVER_ANCS_SERVICE \n\r");
		Ancs_Mgr();

		// Light up green led
		HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
	}
	break;

	case ANCS_DISCONNECTING:
	{
		APP_DBG_MSG("ANCS_DISCONNECTING \n\r");
		ancs_context.state = ANCS_DISCONNECTING;
		ANCS_Client_Reset();
	}
	break;

	case ANCS_DISCONN_COMPLETE:
	{
		APP_DBG_MSG("ANCS_DISCONN_COMPLETE \n\r");
		ancs_context.state = ANCS_DISCONN_COMPLETE;
		ANCS_Client_Reset();

		// Switch off green led
		HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
	}
	break;

	case ANCS_MTU_UPDATE:
	{
		APP_DBG_MSG("ANCS_MTU_UPDATE \n\r");
		ancs_context.state = ANCS_MTU_UPDATE;
		Ancs_Mgr();
	}
	break;

	case ANCS_MTU_EXCHANGE_COMPLETE:
	{
		APP_DBG_MSG("ANCS_MTU_EXCHANGE_COMPLETE \n\r");
		if(ancs_context.state == ANCS_MTU_UPDATE)
			gatt_cmd_resp_release(0);

		ancs_context.state = ANCS_MTU_EXCHANGE_COMPLETE;
	}
	break;

	case ANCS_DISCOVER_ANCS_SERVICE:
	{
		APP_DBG_MSG("ANCS_DISCOVER_ANCS_SERVICE \n\r");
		ancs_context.state = ANCS_DISCOVER_ANCS_SERVICE;
		Ancs_Mgr();
	}
	break;

	case ANCS_PERFORM_NOTIFICATION_ACTION:
	{
		ancs_context.state = ANCS_PERFORM_NOTIFICATION_ACTION;
		Ancs_Mgr();
	}
	break;

	default:
		break;

	}
	return;
}





/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
