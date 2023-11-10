/***************************************************************************//**
 * @file
 * @brief Core application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/
#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "app.h"
#include "app_log.h"
#include "sl_sensor_rht.h"
#include "temperature.h"
#include "sl_bt_api.h"
#include "gatt_db.h"

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////
  app_log_info("%s\n", __FUNCTION__);

}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
SL_WEAK void app_process_action(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application code here!                              //
  // This is called infinitely.                                              //
  // Do not call blocking functions from here!                               //
  /////////////////////////////////////////////////////////////////////////////
}

/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
      // Create an advertising set.
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      app_assert_status(sc);

      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle,
        160, // min. adv. interval (milliseconds * 1.6)
        160, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events
      app_assert_status(sc);
      // Start advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      app_log_info("%s: connection_opened!\n", __FUNCTION__);
      sl_sensor_rht_init();
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      app_log_info("%s: connection_closed!\n", __FUNCTION__);
      sl_sensor_rht_deinit();
      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      break;

      // -------------------------------
      // This event indicates that read button was opened.
    case sl_bt_evt_gatt_server_user_read_request_id:
      app_log_info("%s: Read_button_opened!\n", __FUNCTION__);

      // Here we get all the data that we need for sending our temperature to the device

      uint8_t connection = evt->data.evt_gatt_server_user_read_request.connection;
      uint16_t characteristic = evt->data.evt_gatt_server_user_read_request.characteristic;

      app_log_info("Temperature characteristic identifier by the read button : %d\n", characteristic);


       // here we do the test condition for see if we have indeed the same identifier
      if (characteristic == gattdb_temperature){

          app_log_info("The read button is indeed link to the identifier temperature characteristic  : \n");
      }else{

          app_log_info("Houston we have a problem! the read button (for temperature) isn't link to the identifier temperature characteristic! : \n");
      }

      // Here we start to init for get and convert our temperature
      int16_t bleTemperature;

      // use function from temperature.h
      sl_status_t status = getAndConvertTemperatureToBLE(&bleTemperature);

      if (status == SL_STATUS_OK) {
        app_log_info("Temperature (BLE format): %d\n", bleTemperature);

        // Here we init the sent_len to unit16_t because its how BLE work
        uint16_t sent_len;
        // we use the read response with the parameter above for setup the sending.
        status = sl_bt_gatt_server_send_user_read_response(connection,
                                                           characteristic,
                                                           0,  // att_errorcode
                                                           sizeof(bleTemperature),
                                                           (uint8_t*)&bleTemperature,
                                                           &sent_len);
      } else {
          app_log_info("Failed to get temperature (Status: %lu)\n", (unsigned long)status);
      }
      break;



      // -------------------------------
      // This event indicates that we notification status is ON.
      case sl_bt_evt_gatt_server_characteristic_status_id:
        app_log_info("%s: notification status is ON!\n", __FUNCTION__);

        uint16_t characteristic_notify_button = evt->data.evt_gatt_server_characteristic_status.characteristic;

        app_log_info("Temperature characteristic identifier by the notify button : %d\n", characteristic_notify_button);


         // here we do the test condition for see if we have indeed the same identifier
        if (characteristic_notify_button == gattdb_temperature){

            app_log_info("The notify button is indeed link to the identifier temperature characteristic  : \n");
        }else{

            app_log_info("Houston we have a problem! the notify button (for temperature) isn't link to the identifier temperature characteristic! : \n");
        }


        break;
    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}
