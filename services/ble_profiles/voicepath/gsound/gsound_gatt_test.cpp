#include "bms_gatt_handler.h"

#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "app_key.h"

class GSoundGattTest : public ::testing::Test {
 protected:
  GSoundGattTest() {}

  virtual ~GSoundGattTest() {}

  virtual void SetUp() {}

  virtual void TearDown() {}

};


TEST_F(GSoundGattTest, BmsHandlerMisc) {

  EXPECT_TRUE(BmsIsHandleValid(BISTO_IDX_BMS_START));
  EXPECT_TRUE(BmsIsHandleValid(BISTO_IDX_BMS_END));
  EXPECT_FALSE(BmsIsHandleValid(BISTO_IDX_BMS_NUM));

  BmsSetCccd(BISTO_IDX_BMS_ACTIVE_APP_VAL, 0xFF);
  EXPECT_EQ(0x03, BmsGetCccd(BISTO_IDX_BMS_ACTIVE_APP_VAL));
  EXPECT_EQ(0x03, BmsGetCccd(BISTO_IDX_BMS_ACTIVE_APP_NTF_CFG));
  EXPECT_TRUE(BmsIsNotifiable(BISTO_IDX_BMS_ACTIVE_APP_NTF_CFG));

  BmsSetCccd(BISTO_IDX_BMS_ACTIVE_APP_VAL, 0x01);
  EXPECT_EQ(0x01, BmsGetCccd(BISTO_IDX_BMS_ACTIVE_APP_VAL));
  EXPECT_EQ(0x01, BmsGetCccd(BISTO_IDX_BMS_ACTIVE_APP_NTF_CFG));
  EXPECT_TRUE(BmsIsNotifiable(BISTO_IDX_BMS_ACTIVE_APP_NTF_CFG));

  BmsSetCccd(BISTO_IDX_BMS_ACTIVE_APP_VAL, 0x02);
  EXPECT_EQ(0x02, BmsGetCccd(BISTO_IDX_BMS_ACTIVE_APP_VAL));
  EXPECT_EQ(0x02, BmsGetCccd(BISTO_IDX_BMS_ACTIVE_APP_NTF_CFG));
  EXPECT_FALSE(BmsIsNotifiable(BISTO_IDX_BMS_ACTIVE_APP_NTF_CFG));

  BmsSetCccd(BISTO_IDX_BMS_ACTIVE_APP_VAL, 0x00);
  EXPECT_EQ(0x00, BmsGetCccd(BISTO_IDX_BMS_ACTIVE_APP_VAL));
  EXPECT_EQ(0x00, BmsGetCccd(BISTO_IDX_BMS_ACTIVE_APP_NTF_CFG));
  EXPECT_FALSE(BmsIsNotifiable(BISTO_IDX_BMS_ACTIVE_APP_NTF_CFG));

  BmsSetCccd(BISTO_IDX_BMS_BROADCAST_VAL, 0x00);
  BmsSetCccd(BISTO_IDX_BMS_MEDIA_CMD_VAL, 0x00);
  BmsSetCccd(BISTO_IDX_BMS_MEDIA_STATUS_VAL, 0x00);

  BmsSetCccd(BISTO_IDX_BMS_BROADCAST_VAL, 0xF2);
  BmsSetCccd(BISTO_IDX_BMS_MEDIA_CMD_NTF_CFG, 0xF2);
  BmsSetCccd(BISTO_IDX_BMS_MEDIA_STATUS_NTF_CFG, 0xF2);

  EXPECT_EQ(0x02, BmsGetCccd(BISTO_IDX_BMS_BROADCAST_NTF_CFG));
  EXPECT_EQ(0x02, BmsGetCccd(BISTO_IDX_BMS_MEDIA_CMD_VAL));
  EXPECT_EQ(0x02, BmsGetCccd(BISTO_IDX_BMS_MEDIA_STATUS_VAL));
  EXPECT_FALSE(BmsIsNotifiable(BISTO_IDX_BMS_MEDIA_CMD_VAL));

}

TEST_F(GSoundGattTest, BmsHandlerReadWrite) {
  BmsAttributeData read_info;
  char const *test_string = "bms is working";
  uint8_t data_out[BMS_SERVER_AAPP_SIZE];
  uint8_t data_in[BMS_SERVER_AAPP_SIZE];
  uint32_t result;
  bool notifiable;

  strncpy((char *)data_out, test_string, sizeof(data_out));
  read_info.value = data_in;

  // Test Invalid Reads
  read_info.status = -1;
  BmsHandleReadReq(BISTO_IDX_BMS_BROADCAST_VAL, &read_info);
  EXPECT_EQ(read_info.status, BLE_ATT_READ_NOT_ALLOWED);

  read_info.status = -1;
  BmsHandleReadReq(BISTO_IDX_BMS_MEDIA_CMD_VAL, &read_info);
  EXPECT_EQ(read_info.status, BLE_ATT_READ_NOT_ALLOWED);

  read_info.status = -1;
  BmsHandleReadReq(BISTO_IDX_BMS_MEDIA_STATUS_VAL, &read_info);
  EXPECT_EQ(read_info.status, BLE_ATT_READ_NOT_ALLOWED);

  BmsHandleReadReq(BISTO_IDX_BMS_MEDIA_STATUS_CHAR, &read_info);
  EXPECT_EQ(read_info.status, BLE_ATT_INVALID_HANDLE);

  // Test Read / Write to Active App
  BmsSetCccd(BISTO_IDX_BMS_ACTIVE_APP_VAL, 0x01);
  notifiable = BmsHandleWriteRequest(BISTO_IDX_BMS_ACTIVE_APP_VAL, data_out, 20, &result);
  BmsHandleReadReq(BISTO_IDX_BMS_ACTIVE_APP_VAL, &read_info);
  EXPECT_STREQ((const char *)read_info.value, (const char *)data_out);
  EXPECT_EQ(read_info.length, BMS_SERVER_AAPP_SIZE);
  EXPECT_EQ(read_info.status, BLE_ATT_OK);
  EXPECT_EQ(result, BLE_ATT_OK);
  EXPECT_TRUE(notifiable);

  BmsSetCccd(BISTO_IDX_BMS_ACTIVE_APP_VAL, 0x02);
  notifiable = BmsHandleWriteRequest(BISTO_IDX_BMS_ACTIVE_APP_VAL, data_out, 20, &result);
  EXPECT_STREQ((const char *)data_out, (const char *)test_string);
  EXPECT_EQ(result, BLE_ATT_OK);
  EXPECT_FALSE(notifiable);

  notifiable = BmsHandleWriteRequest(BISTO_IDX_BMS_ACTIVE_APP_VAL, data_out, 10, &result);
  EXPECT_STREQ((const char *)data_out, (const char *)test_string);
  EXPECT_EQ(result, BLE_ATT_INVALID_ATT_VAL_EN);
  EXPECT_FALSE(notifiable);


  BmsSetCccd(BISTO_IDX_BMS_MEDIA_CMD_VAL, 0x01);
  notifiable = BmsHandleWriteRequest(BISTO_IDX_BMS_MEDIA_CMD_VAL, data_out, 0, &result);
  EXPECT_STREQ((const char *)data_out, (const char *)test_string);
  EXPECT_EQ(result, BLE_ATT_INVALID_ATT_VAL_EN);
  EXPECT_FALSE(notifiable);

  notifiable = BmsHandleWriteRequest(BISTO_IDX_BMS_MEDIA_CMD_VAL, data_out, 5, &result);
  EXPECT_STREQ((const char *)data_out, (const char *)test_string);
  EXPECT_EQ(result, BLE_ATT_OK);
  EXPECT_TRUE(notifiable);

  data_out[0] = 0x01;
  notifiable = BmsHandleWriteRequest(BISTO_IDX_BMS_ACTIVE_APP_NTF_CFG, data_out, 2, &result);
  BmsHandleReadReq(BISTO_IDX_BMS_ACTIVE_APP_NTF_CFG, &read_info);
  EXPECT_EQ(read_info.value[0], 0x01);
  EXPECT_EQ(read_info.length, 2);
  EXPECT_EQ(read_info.status, BLE_ATT_OK);
  EXPECT_EQ(BmsGetCccd(BISTO_IDX_BMS_ACTIVE_APP_NTF_CFG), 0x01);
  EXPECT_EQ(result, BLE_ATT_OK);
  EXPECT_FALSE(notifiable);


  data_out[0] = 0xFF;
  notifiable = BmsHandleWriteRequest(BISTO_IDX_BMS_BROADCAST_NTF_CFG, data_out, 2, &result);
  BmsHandleReadReq(BISTO_IDX_BMS_BROADCAST_NTF_CFG, &read_info);
  EXPECT_EQ(read_info.value[0], 0x03);
  EXPECT_EQ(read_info.length, 2);
  EXPECT_EQ(read_info.status, BLE_ATT_OK);
  EXPECT_EQ(BmsGetCccd(BISTO_IDX_BMS_BROADCAST_NTF_CFG), 0x03);
  EXPECT_EQ(result, BLE_ATT_OK);
  EXPECT_FALSE(notifiable);

  data_out[0] = 0x02;
  notifiable = BmsHandleWriteRequest(BISTO_IDX_BMS_MEDIA_CMD_NTF_CFG, data_out, 2, &result);
  BmsHandleReadReq(BISTO_IDX_BMS_MEDIA_CMD_NTF_CFG, &read_info);
  EXPECT_EQ(read_info.value[0], 0x02);
  EXPECT_EQ(read_info.length, 2);
  EXPECT_EQ(read_info.status, BLE_ATT_OK);
  EXPECT_EQ(BmsGetCccd(BISTO_IDX_BMS_MEDIA_CMD_NTF_CFG), 0x02);
  EXPECT_EQ(result, BLE_ATT_OK);
  EXPECT_FALSE(notifiable);

  data_out[0] = 0x00;
  notifiable = BmsHandleWriteRequest(BISTO_IDX_BMS_MEDIA_STATUS_NTF_CFG, data_out, 2, &result);
  BmsHandleReadReq(BISTO_IDX_BMS_MEDIA_STATUS_NTF_CFG, &read_info);
  EXPECT_EQ(read_info.value[0], 0x00);
  EXPECT_EQ(read_info.length, 2);
  EXPECT_EQ(read_info.status, BLE_ATT_OK);
  EXPECT_EQ(BmsGetCccd(BISTO_IDX_BMS_MEDIA_STATUS_NTF_CFG), 0x00);
  EXPECT_EQ(result, BLE_ATT_OK);
  EXPECT_FALSE(notifiable);

  data_out[0] = 0x00;
  notifiable = BmsHandleWriteRequest(BISTO_IDX_BMS_MEDIA_STATUS_CHAR, data_out, 2, &result);
  EXPECT_EQ(result, BLE_ATT_INVALID_HANDLE);
  EXPECT_FALSE(notifiable);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
