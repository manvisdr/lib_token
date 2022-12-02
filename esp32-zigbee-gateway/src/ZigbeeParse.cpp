#include "zigbee_gateway.h"

/* Biến chứa địa chỉ IEEE ADDR của Coordinator */

void bytetoHexChar(uint8_t ubyte, uint8_t *uHexChar)
{
  uHexChar[1] = ((ubyte & 0x0F) < 10) ? ((ubyte & 0x0F) + '0') : (((ubyte & 0x0F) - 10) + 'A');
  uHexChar[0] = ((ubyte >> 4 & 0x0F) < 10) ? ((ubyte >> 4 & 0x0F) + '0') : (((ubyte >> 4 & 0x0F) - 10) + 'A');
}

void bytestoHexChars(uint8_t *ubyte, int32_t len, uint8_t *uHexChar)
{
  for (int8_t i = 0; i < len; i++)
  {
    bytetoHexChar(ubyte[i], (uint8_t *)&uHexChar[i * 2]);
  }
}

void hexChartoByte(uint8_t *uHexChar, uint8_t *ubyte)
{
  *ubyte = 0;
  *ubyte = ((uHexChar[0] <= '9' && uHexChar[0] >= '0') ? ((uHexChar[0] - '0') << 4) : *ubyte);
  *ubyte = ((uHexChar[0] <= 'F' && uHexChar[0] >= 'A') ? ((uHexChar[0] - 'A' + 10) << 4) : *ubyte);

  *ubyte = ((uHexChar[1] <= '9' && uHexChar[1] >= '0') ? *ubyte | (uHexChar[1] - '0') : *ubyte);
  *ubyte = ((uHexChar[1] <= 'F' && uHexChar[1] >= 'A') ? *ubyte | ((uHexChar[1] - 'A') + 10) : *ubyte);
}

void hexCharsToBytes(uint8_t *uHexChar, int32_t len, uint8_t *ubyte)
{
  for (int8_t i = 0; i < len; i += 2)
  {
    hexChartoByte((uint8_t *)&uHexChar[i], (uint8_t *)&ubyte[i / 2]);
  }
}

String convertToString(char *a, int size)
{
  String s = a;
  return s;
}

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
  {
    Serial.println("ZDO_MGMT_LEAVE_REQ");
  }
  break;

  case ZB_RECEIVE_DATA_INDICATION:
  {
    Serial.println("ZB_RECEIVE_DATA_INDICATION");
  }
  break;

  case ZDO_MGMT_PERMIT_JOIN_RSP:
  {
    Serial.println("ZDO_MGMT_PERMIT_JOIN_RSP");
    // ZdoMgmtPermitJoinRspInd_t* ZdoMgmtPermitJoinRspInd = (ZdoMgmtPermitJoinRspInd_t*)zigbee_msg.data;
    // Serial.print("\tsrcaddr: ");
    // Serial.println(ZdoMgmtPermitJoinRspInd->srcaddr);
    // Serial.print("\tstatus: ");
    // Serial.println(ZdoMgmtPermitJoinRspInd->status);
  }
  break;

  case ZDO_TC_DEV_IND:
  {
    Serial.println("ZDO_TC_DEV_IND");
  }
  break;

  case UTIL_GET_DEVICE_INFO_RESPONSE:
  {
    Serial.println("UTIL_GET_DEVICE_INFO_RESPONSE");
    bytestoHexChars(&zigbee_msg.data[1], 8, (uint8_t *)zc_ieee_addr);
    Serial.println(zc_ieee_addr);
  }
  break;

  case ZDO_BIND_RSP:
  {
    Serial.print("ZDO_BIND_RSP: ");
    Serial.println(zigbee_msg.data[2]);
  }
  break;

  case AF_DATA_REQUEST_IND:
  {
    Serial.println("AF_DATA_REQUEST_IND");
    // uint8_t* status = (uint8_t*)zigbee_msg.data;
    // Serial.print("\tstatus: ");
    // Serial.println(*status);
  }
  break;

  case AF_DATA_CONFIRM:
  {
    Serial.println("AF_DATA_CONFIRM");
    afDataConfirm_t *afDataConfirm = (afDataConfirm_t *)zigbee_msg.data;
    Serial.print("\tstatus: ");
    Serial.println(afDataConfirm->status);
    Serial.print("\tendpoint: ");
    Serial.println(afDataConfirm->endpoint);
    Serial.print("\ttransID: ");
    Serial.println(afDataConfirm->transID);
  }
  break;

  case AF_INCOMING_MSG:
  {
    afIncomingMSGPacket_t *st_af_incoming_msg = (afIncomingMSGPacket_t *)zigbee_msg.data;
    Serial.println("AF_INCOMING_MSG");

#if defined(DBG_ZB_FRAME)
    Serial.print("group_id: ");
    Serial.println(st_af_incoming_msg->group_id, HEX);
    Serial.print("cluster_id: ");
    Serial.println(st_af_incoming_msg->cluster_id, HEX);
    Serial.print("src_addr: ");
    Serial.println(st_af_incoming_msg->src_addr, HEX);
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
    Serial.println(st_af_incoming_msg->time_stamp, HEX);
    Serial.print("trans_seq_num: ");
    Serial.println(st_af_incoming_msg->trans_seq_num, HEX);
    Serial.print("len: ");
    Serial.println(st_af_incoming_msg->len);
    Serial.print("data: ");
    for (int i = 0; i < st_af_incoming_msg->len; i++)
    {
      Serial.print(st_af_incoming_msg->payload[i], HEX);
      Serial.print(" ");
    }
    Serial.println("");
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
      Serial.println("ZCL_CLUSTER_ID_GEN_ON_OFF");
      uint8_t retGenOnOff;
      if (st_af_incoming_msg->len > 9)
      {
        control_switch_address = st_af_incoming_msg->src_addr;
        retGenOnOff = st_af_incoming_msg->payload[st_af_incoming_msg->len - 8];
        Serial.println(retGenOnOff);
      }
      else
      {
        retGenOnOff = st_af_incoming_msg->payload[st_af_incoming_msg->len - 1];
        Serial.println(retGenOnOff);
      }
      break;

    default:
      break;
    }
  }
  break;

  case ZDO_MGMT_LEAVE_RSP:
  {
    Serial.println("ZDO_MGMT_LEAVE_RSP");
  }
  break;

  case ZDO_END_DEVICE_ANNCE_IND:
  {
    Serial.println("ZDO_END_DEVICE_ANNCE_IND");
    ZDO_DeviceAnnce_t *ZDO_DeviceAnnce = (ZDO_DeviceAnnce_t *)zigbee_msg.data;
    Serial.print("\tSrcAddr: ");
    Serial.println(ZDO_DeviceAnnce->SrcAddr, HEX);
    Serial.print("\tnwkAddr: ");
    Serial.println(ZDO_DeviceAnnce->nwkAddr, HEX);
    Serial.print("\textAddr: ");
    for (int i = 0; i < Z_EXTADDR_LEN; i++)
    {
      Serial.print(ZDO_DeviceAnnce->extAddr[i], HEX);
      control_switch_ieee[i] = ZDO_DeviceAnnce->extAddr[i];
    }

    Serial.println("");
    bytestoHexChars(control_switch_ieee, 8, (uint8_t *)zr_ieee_addr);
    Serial.print("extAddr: ");
    Serial.println(zr_ieee_addr);

    Serial.print("\n");
    /***
     * Specifies the MAC capabilities of the device.
     * Bit: 0 – Alternate PAN Coordinator
     * 1 – Device type: 1- ZigBee Router; 0 – End Device
     * 2 – Power Source: 1 Main powered
     * 3 – Receiver on when idle
     * 4 – Reserved
     * 5 – Reserved
     * 6 – Security capability
     * 7 – Reserved
     */
    Serial.print("\tcapabilities: ");
    Serial.println(ZDO_DeviceAnnce->capabilities);
  }
  break;
  }
}