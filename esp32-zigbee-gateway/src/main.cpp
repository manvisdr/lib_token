#include "zigbee_gateway.h"

U8G2_SSD1306_128X64_NONAME_1_HW_I2C lcd(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
// SoftwareSerial znp_serial(17, 16);
zb_znp zigbee_network(&Serial2);
GEM_u8g2 menu(lcd);
OneButton MIDButton(PIN_NAV_MID, true);

EthernetClient ethClient;
PubSubClient mqttClientEth(ethClient);
#define MQTT_USER_ID "anyone"

char zc_ieee_addr[17];
char zr_ieee_addr[17];

/* Biến xử lý điều khiển switch */
uint8_t control_switch_ieee[17];
uint8_t control_switch_cmd_seq = 0;
uint16_t control_switch_address = 0;

void setup()
{
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, 17, 16);
  Serial2.setTimeout(100);

  // sys_LanInit();

  /* Khởi động coodinatior */
  Serial.println("\nstart_coordinator(0)");
  if (zigbee_network.start_coordinator(0) == 0)
  {
    Serial.println("OK");
  }
  else
  {
    Serial.println("Failed to start coordinator");
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
        Serial.println("OK");
      }
      else
      {
        Serial.println("NG");
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

      /* yêu cầu Toggle công tắc */
    case '3':
    {
      Serial.println("TOOGLE Switch Req !\n");
      /*
       * Frame Control, Transaction Sequence Number, Value control
       * Value control -> 0x00: off, 0x01: on, 0x02: toogle
       */
      if (control_switch_address)
      {
        uint8_t st_buffer[3] = {/* Frame control */ 0x01,
                                /* Transaction Sequence Number */ 0x00, /* control_switch_cmd_seq++ */
                                /* Value Control */ 0x02};              /* Value Control [ 0x00:OFF , 0x01:ON , 0x02:TOOGLE ] */
        st_buffer[1] = control_switch_cmd_seq++;

        af_data_request_t st_af_data_request;
        st_af_data_request.cluster_id = ZCL_CLUSTER_ID_PI_GENERIC_TUNNEL;
        st_af_data_request.dst_address = control_switch_address;
        st_af_data_request.dst_endpoint = 0x01;
        st_af_data_request.src_endpoint = 0x01;
        st_af_data_request.trans_id = 0x00;
        st_af_data_request.options = 0x10;
        st_af_data_request.radius = 0x0F;
        st_af_data_request.len = sizeof(st_buffer);
        st_af_data_request.data = st_buffer;

        zigbee_network.send_af_data_req(st_af_data_request);
      }
      else
      {
        Serial.println("Please join Switch !\n");
      }
    }
    break;

      /******************************************************************
       *  Ví dụ:
       * gửi data từ Gateway(coodinator) đến các thiết bị / cảm biến
       * các thông số cần thiết cho quá trình này bao gồm
       * 1. short address, là địa chỉ đc coodinator cấp khi thiết bị / cảm biến join vào mạng
       * 2. độ dài của mảng data cần truyền
       * 3. data

    case 's': {
      uint8_t st_buffer[10];
      af_data_request_t st_af_data_request;
      st_af_data_request.cluster_id    = ZCL_CLUSTER_ID_PI_GENERIC_TUNNEL;
      st_af_data_request.dst_address   = [ Địa chỉ đích của thiết bị / sensor ] ví du: control_switch_address
      st_af_data_request.dst_endpoint  = 0x01;
      st_af_data_request.src_endpoint  = 0x01;
      st_af_data_request.trans_id      = 0x00;
      st_af_data_request.options       = 0x10;
      st_af_data_request.radius        = 0x0F;
      st_af_data_request.len           = [ Độ dài data cần gửi đi ] ví dụ: sizeof(st_buffer)
      st_af_data_request.data          = [ data ] ví dụ: st_buffer
      zigbee_network.send_af_data_req(st_af_data_request);
    }
      break;
      ********************************************************************/

      /* Get Coordinator IEEE ADDR */
    case '4':
    {
      zigbee_network.util_get_device_info();
    }
    break;

      /* Send Binding request */
    case '5':
    {
      binding_req_t binding;

      hexCharsToBytes((uint8_t *)convertToString((char *)zc_ieee_addr, 16).c_str(), 16, binding.dst_address);
      hexCharsToBytes((uint8_t *)convertToString((char *)zr_ieee_addr, 16).c_str(), 16, binding.src_ieee_addr);
      memcpy(binding.src_short_addr, (uint8_t *)&control_switch_address, 2);

      binding.dst_mode = 3;                           /* default mode */
      binding.cluster_id = ZCL_CLUSTER_ID_GEN_ON_OFF; /* clusster want to binding */
      binding.src_endpoint = 1;                       /* endpoint device want to binding */
      binding.dst_endpoint = 1;                       /* endpoint coodinator want to binding */
      zigbee_network.zdo_binding_req(binding);
    }
    break;

    default:
      break;
    }
  }
}
