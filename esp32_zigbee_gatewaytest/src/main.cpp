/***********************************************************************************************************************************************************
		@AUTHOR: EPCB-TECH
		@DATE: 2020
		@OPEN SOURCE SIMPLE ZIGBEE ESP32 GATEWAY
		@VERSION: v0.1
 ***********************************************************************************************************************************************************/
#include <Arduino.h>
#include <zb_znp.h>
#include <zb_zcl.h>

/*
	 HƯỚNG DẪN SỬ DỤNG DEMO

	 Sau khi nạp cho arduino
	 B1: Mở terminal của arduino, chỉnh baudrate về 115200
	 B2: Trên terminal, gửi 1 để yêu cầu cho phép thiết bị mới tham gia mạng.
	 B3: Nhấn giữ nút paring trên thiết bị/cảm biến cho đến khi đèn của thiết bị/cảm biến sáng lên thì nhả nút.
	 B4: Đợi vài giây đến khi nhận đc thông báo từ terminal, lúc này thông tin thiết bị/cảm biến sẽ show lên terminal

*/

#define DBG_ZB_FRAME
#define pinCtrlRelayLight LED_BUILTIN // Định nghĩa pin để điều khiển Relay

zb_znp zigbee_network(&Serial2); // Khởi tạo UART2 cho ESP32 Wrover để đọc messenge từ Zigbee

void send_msg_gateway(uint8_t msg);

/* Biến xử lý điều khiển switch */
uint8_t control_switch_cmd_seq = 0;
uint16_t control_switch_address = 0;

int zb_znp::zigbee_message_handler(zigbee_msg_t &zigbee_msg)
{
	/* zigbee start debug message */
	Serial.print("[ZB msg] len: ");
	Serial.print(zigbee_msg.len);
	Serial.print(" cmd0: ");
	Serial.print(zigbee_msg.cmd0, HEX);
	Serial.print(" cmd1: ");
	Serial.print(zigbee_msg.cmd1, HEX);
	Serial.print(" data: ");
	for (int i = 0; i < zigbee_msg.len; i++)
	{
		Serial.print(zigbee_msg.data[i], HEX);
		Serial.print(" ");
	}
	Serial.println("");
	/* zigbee stop debug message */

	uint16_t zigbee_cmd = BUILD_UINT16(zigbee_msg.cmd1, zigbee_msg.cmd0);

	switch (zigbee_cmd)
	{
	case ZDO_MGMT_LEAVE_REQ:

		Serial.println("ZDO_MGMT_LEAVE_REQ");

		break;

	case ZB_RECEIVE_DATA_INDICATION:

		Serial.println("ZB_RECEIVE_DATA_INDICATION");

		break;

	case AF_INCOMING_MSG:
	{
		afIncomingMSGPacket_t *st_af_incoming_msg = (afIncomingMSGPacket_t *)zigbee_msg.data;
		Serial.println("AF_INCOMING_MSG");

#if defined(DBG_ZB_FRAME)
		char buf[9];
		char buf1[18];
		Serial.print("group_id: ");
		sprintf(buf, "%04x", st_af_incoming_msg->group_id);
		Serial.println(buf);

		Serial.print("cluster_id: ");
		sprintf(buf, "%04x", st_af_incoming_msg->cluster_id);
		Serial.println(buf);

		Serial.print("src_addr: ");
		sprintf(buf, "%04x", st_af_incoming_msg->src_addr);
		Serial.println(buf);

		Serial.print("src_endpoint: ");
		Serial.println(st_af_incoming_msg->src_endpoint, HEX);

		Serial.print("dst_endpoint: ");
		Serial.println(st_af_incoming_msg->dst_endpoint, HEX);

		Serial.print("was_broadcast: ");
		Serial.println(st_af_incoming_msg->was_broadcast, HEX);

		Serial.print("link_quality: ");
		Serial.println(st_af_incoming_msg->link_quality, HEX);

		Serial.print("security_use: ");
		Serial.println(st_af_incoming_msg->security_use, HEX);

		Serial.print("time_stamp: ");
		sprintf(buf1, "%08x", st_af_incoming_msg->time_stamp);
		Serial.println(buf1);

		Serial.print("trans_seq_num: ");
		Serial.println(st_af_incoming_msg->trans_seq_num, HEX);

		Serial.print("len: ");
		Serial.println(st_af_incoming_msg->len, HEX);

		Serial.print("data: ");
		for (int i = 0; i < st_af_incoming_msg->len; i++)
		{
			Serial.print(st_af_incoming_msg->payload[i], HEX);
			Serial.print(" ");
		}
		Serial.println(" ");
#endif

		switch (st_af_incoming_msg->cluster_id)
		{
		case ZCL_CLUSTER_ID_MS_RELATIVE_HUMIDITY:
		{
			Serial.println("ZCL_CLUSTER_ID_MS_RELATIVE_HUMIDITY");
			uint16_t retHum = (uint16_t)((st_af_incoming_msg->payload[st_af_incoming_msg->len - 1] * 256) +
																	 st_af_incoming_msg->payload[st_af_incoming_msg->len - 2]);

			// Ví dụ: retHum = 6789, thì giá trị trả về là 67,89 %
			Serial.print(retHum / 100); // Lấy Trước dấu phẩy -> 67
			Serial.print(",");
			Serial.println(retHum % 100); // Lấy sau dấu phẩy -> 89
		}
		break;

		case ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT:
		{
			Serial.println("ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT");
			uint16_t retTemp = (uint16_t)((st_af_incoming_msg->payload[st_af_incoming_msg->len - 1] * 256) +
																		st_af_incoming_msg->payload[st_af_incoming_msg->len - 2]);

			// Ví dụ: retTemp = 2723, thì giá trị trả về là 27,23 *C
			Serial.print(retTemp / 100); // Lấy Trước dấu phẩy -> 27
			Serial.print(",");
			Serial.println(retTemp % 100); // Lấy sau dấu phẩy -> 23
		}
		break;

		case ZCL_CLUSTER_ID_MS_OCCUPANCY_SENSING:
		{
			Serial.println("ZCL_CLUSTER_ID_MS_OCCUPANCY_SENSING");
			uint8_t retOccu = st_af_incoming_msg->payload[st_af_incoming_msg->len - 1];
			Serial.println(retOccu);
		}
		break;

		case ZCL_CLUSTER_ID_GEN_ON_OFF:
		{
			Serial.println("ZCL_CLUSTER_ID_GEN_ON_OFF");
			uint8_t retGenOnOff = st_af_incoming_msg->payload[st_af_incoming_msg->len - 1];
			Serial.println(retGenOnOff);
		}
		break;
		}
	}
	break;

	case ZDO_MGMT_LEAVE_RSP:
		Serial.println("ZDO_MGMT_LEAVE_RSP");
		break;

	default:
		break;
	}
}

boolean flag_setJoin = true;
void setup()
{
	Serial.begin(115200);
	Serial2.begin(115200, SERIAL_8N1, 17, 16);
	Serial2.setTimeout(100);

	/* Cấu hình output cho chân điều khiển relay */
	pinMode(pinCtrlRelayLight, OUTPUT);

	/* Khởi động coodinatior */
	Serial.println("\nstart_coordinator(1)");
	if (zigbee_network.start_coordinator(1) == 0)
	{
		Serial.println("OK");
	}
	else
	{
		Serial.println("FAILED");
	}
}

/* ký tự tạm để xử lý yêu cầu từ terminal */
char serial_cmd;

void loop()
{
	/* hàm update() phải được gọi trong vòng lặp để xử lý các gói tin nhận được từ ZigBee Shield */
	zigbee_network.update();

	/* Kiểm tra / thực hiện các lệnh từ terminal */
	if (Serial.available())
	{
		serial_cmd = Serial.read();
		Serial.println(serial_cmd);

		switch (serial_cmd)
		{
			/* Cấu hình lại coodinator, đưa các cáu hình về mặc định.
			 * Chú ý: list thiết bị đã tham gia vào mạng trước đó sẽ bị mất */
		case '0':
		{
			Serial.println("\nstart_coordinator(1)");
			if (zigbee_network.start_coordinator(1) == 0)
			{
				Serial.println("force start coordinator successfully");
			}
			else
			{
				Serial.println("force start coordinator error");
			}
		}
		break;

			/* Cho phép thiết bị tham gia vào mạng */
		case '1':
		{
			Serial.println("set_permit_joining_req");
			/* ALL_ROUTER_AND_COORDINATOR -> cho phép thiết bị tham gia mạng từ Coodinator (ZigBee Shield)
			 * hoặc qua router (router thường là các thiết bị đc cấp điện, như ổ cắm, công tắc, bóng đèn ...
			 * 60, sau 60s nếu không có thiết bị tham gia mạng, coodinator sẽ trở về mode hoạt động bình thường
			 * người dùng muốn thêm thiết bị mới phải yêu cấu thêm lần nữa
			 * 1 , đợi thiết bị join thành công, mới thoát khỏi hàm, nếu 0, sự kiện có thiết bị mới tham gia mạng
			 * sẽ được nhận ở hàm callback int zb_znp::zigbee_message_handler(zigbee_msg_t& zigbee_msg)
			 */
			zigbee_network.set_permit_joining_req(ALL_ROUTER_AND_COORDINATOR, 60, 1);
		}
		break;

		default:
			break;
		}
	}
}