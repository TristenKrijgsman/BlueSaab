#include "CDCStatus.h"
#include "SaabCan.h"
#include "MessageSender.h"

CDCStatus cdcStatus;

unsigned char cdcPoweronCmd[NODE_STATUS_TX_MSG_SIZE][8] = {
		{ 0x32, 0x00, 0x00, 0x03, 0x01, 0x02, 0x00, 0x00 },
		{ 0x42, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00 },
		{ 0x52, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00 },
		{ 0x62, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00 }
};
unsigned char cdcActiveCmd[NODE_STATUS_TX_MSG_SIZE][8] = {
		{ 0x32, 0x00, 0x00, 0x16, 0x01, 0x02, 0x00, 0x00 },
		{ 0x42, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x00 },
		{ 0x52, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x00 },
		{ 0x62, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x00 }
};
unsigned char cdcPowerdownCmd[NODE_STATUS_TX_MSG_SIZE][8] = {
		{ 0x32, 0x00, 0x00, 0x19, 0x01, 0x00, 0x00, 0x00 },
		{ 0x42, 0x00, 0x00, 0x38, 0x01, 0x00, 0x00, 0x00 },
		{ 0x52, 0x00, 0x00, 0x38, 0x01, 0x00, 0x00, 0x00 },
		{ 0x62, 0x00, 0x00, 0x38, 0x01, 0x00, 0x00, 0x00 }
};

MessageSender cdcPoweronCmdSender(NODE_STATUS_TX_CDC,cdcPoweronCmd, 4, NODE_STATUS_TX_INTERVAL);
MessageSender cdcActiveCmdSender(NODE_STATUS_TX_CDC, cdcActiveCmd, 4, NODE_STATUS_TX_INTERVAL);
MessageSender cdcPowerdownCmdSender(NODE_STATUS_TX_CDC, cdcPowerdownCmd, 4, NODE_STATUS_TX_INTERVAL);

void CDCStatus::initialize() {
	saabCan.attach(NODE_STATUS_RX_IHU, callback(this, &CDCStatus::onIhuStatusFrame));
	thread.start(callback(this, &CDCStatus::run));
}

void CDCStatus::onIhuStatusFrame(CANMessage& frame) {
    /*
     Here be dragons... This part of the code is responsible for causing lots of headache
     We look at the bottom half of 3rd byte of '6A1' frame to determine what the "reply" should be
     */
    switch (frame.data[3] & 0x0F){
        case (0x3):
			cdcPoweronCmdSender.send();
            break;
        case (0x2):
			cdcActiveCmdSender.send();
            break;
        case (0x8):
			cdcPowerdownCmdSender.send();
            break;
    }
}

void CDCStatus::run() {
	bool cdcStatusResendNeeded = false;
	bool cdcStatusResendDueToCdcCommand = false;
	bool cdcActive = true;

	while(1) {
		sendCdcStatus(cdcStatusResendNeeded, cdcStatusResendDueToCdcCommand, cdcActive);
		Thread::wait(CDC_STATUS_TX_BASETIME);
	}
}

void CDCStatus::sendCdcStatus(bool event, bool remote, bool cdcActive) {

    /* Format of GENERAL_STATUS_CDC frame:
     ID: CDC node ID
     [0]:
     byte 0, bit 7: FCI NEW DATA: 0 - sent on basetime, 1 - sent on event
     byte 0, bit 6: FCI REMOTE CMD: 0 - status change due to internal operation, 1 - status change due to CDC_COMMAND frame
     byte 0, bit 5: FCI DISC PRESENCE VALID: 0 - disc presence signal is not valid, 1 - disc presence signal is valid
     [1]: Disc presence validation (boolean)
     byte 1-2, bits 0-15: DISC PRESENCE: (bitmap) 0 - disc absent, 1 - disc present. Bit 0 is disc 1, bit 1 is disc 2, etc.
     [2]: Disc presence (bitmap)
     byte 1-2, bits 0-15: DISC PRESENCE: (bitmap) 0 - disc absent, 1 - disc present. Bit 0 is disc 1, bit 1 is disc 2, etc.
     [3]: Disc number currently playing
     byte 3, bits 7-4: DISC MODE
     byte 3, bits 3-0: DISC NUMBER
     [4]: Track number currently playing
     [5]: Minute of the current track
     [6]: Second of the current track
     [7]: CD changer status; D0 = Married to the car
     */

    unsigned char cdcGeneralStatusCmd[8] = {0,0,0,0, 0xFF, 0xFF, 0xFF, 0xD0};
    cdcGeneralStatusCmd[0] = ((event ? 0x07 : 0x00) | (remote ? 0x00 : 0x01)) << 5;
    cdcGeneralStatusCmd[1] = (cdcActive ? 0xFF : 0x00);                             // Validation for presence of six discs in the magazine
    cdcGeneralStatusCmd[2] = (cdcActive ? 0x3F : 0x01);                             // There are six discs in the magazine
    cdcGeneralStatusCmd[3] = (cdcActive ? 0x41 : 0x01);                             // ToDo: check 0x01 | (discMode << 4) | 0x01
//    cdcGeneralStatusCmd[4] = 0xFF;
//    cdcGeneralStatusCmd[5] = 0xFF;
//    cdcGeneralStatusCmd[6] = 0xFF;
//    cdcGeneralStatusCmd[7] = 0xD0;

    saabCan.sendCanFrame(GENERAL_STATUS_CDC, cdcGeneralStatusCmd);
}

