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
#include "sl_sleeptimer.h"

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;
// Storage of the timer
static sl_sleeptimer_timer_handle_t timer_handle;

uint32_t timeout_question_21 = 1000;
uint32_t timeout_21;

struct timer_struct {
  int myVariable;
  int connection;
  int characteristic;
};

struct timer_struct temperature_data = {
    .myVariable =1,
};
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

// Used in the question 21 for set the timer of notify
 void CallbackFunction(sl_sleeptimer_timer_handle_t *handle, void *data) {
           sl_status_t sc;
           bool running = false;
           struct timer_struct * my_ts = (struct timer_struct *) data;
           int16_t bleTemperature_for_notify;

           handle  = handle;
           app_log_info("Timer step: %d\n", my_ts->myVariable);

          sc = sl_sleeptimer_is_timer_running(handle, &running);
          if (sc == SL_STATUS_OK && running == true) {
              sc = sl_sleeptimer_stop_timer(handle);
              app_assert_status(sc);
          }
          // use function from temperature.h
          sl_status_t status_notify = getAndConvertTemperatureToBLE(&bleTemperature_for_notify);

          if (status_notify == SL_STATUS_OK) {
            app_log_info("Temperature (BLE format): %d\n", bleTemperature_for_notify);
            app_log_info("connection: %d\n", my_ts->connection);
            app_log_info("characteristic: %d\n", my_ts->characteristic);
            int sz = sizeof(bleTemperature_for_notify);
            app_log_info("data sz: %d\n", sz);

            // we use the read response with the parameter above for setup the sending.
            status_notify = sl_bt_gatt_server_send_notification(my_ts->connection,
                                                                my_ts->characteristic,
                                                               sz,
                                                               (uint8_t*)&bleTemperature_for_notify);

            app_log_info("and send notif Status: %lu\n", status_notify);

          } else {
            app_log_info("Failed to get temperature (Status: %lu)\n", (unsigned long)status_notify);
          }

          my_ts->myVariable += 1;
        }

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

      }else{

          app_log_info("Houston we have a problem! the read button (for temperature) isn't link to the identifier temperature characteristic! : \n");
      }

      // Here we start to init for get and convert our temperature
      break;



      // -------------------------------
      // This event indicates that we notification status is ON.
      case sl_bt_evt_gatt_server_characteristic_status_id:
        app_log_info("%s: notification status is ON!\n", __FUNCTION__);

        uint16_t characteristic_notify_button = evt->data.evt_gatt_server_characteristic_status.characteristic;
        uint8_t connection_notify_button = evt->data.evt_gatt_server_characteristic_status.connection;
        temperature_data.characteristic = characteristic_notify_button;
        temperature_data.connection = connection_notify_button;

        app_log_info("connection : %d\n", evt->data.evt_gatt_server_characteristic_status.connection);

        app_log_info("Temperature characteristic identifier by the notify button : %d\n", characteristic_notify_button);



         // here we do the test condition for see if we have indeed the same identifier
        if (characteristic_notify_button == gattdb_temperature){

            app_log_info("The notify button is indeed link to the identifier temperature characteristic  : \n");
            uint32_t status_flags = evt->data.evt_gatt_server_characteristic_status.status_flags;

            app_log_info("Status flag of our notif button after been activated : %lu\n", status_flags);

             if (status_flags==sl_bt_gatt_server_client_config){

                      app_log_info("we are indeed clicking on the notify button.\n");
                      int config = evt->data.evt_gatt_server_characteristic_status.client_config_flags;
                      app_log_info("The value of the server_client_config is : %d\n", config);

                      switch (config) {
                           case 0:
                             app_log_info("the actual value of notify is %d  which mean 'Disable notifications and indications'\n", config);
                             sl_status_t stop_status = sl_sleeptimer_stop_timer(&timer_handle);
                             if (stop_status == SL_STATUS_OK) {
                                 app_log_info("Timer stopped successfully.\n");
                             } else {
                                 app_log_info("Failed to stop the timer (Status: %lu)\n", (unsigned long)stop_status);
                             }
                           break;
                           case 1:
#if 0
                             app_log_info("the actual value of notify is %d  which mean 'The characteristic value shall be notified'\n", config);
                             // Here we did the question 21:
                             timeout_21 =sl_sleeptimer_ms_to_tick(timeout_question_21);
                             // Start the timer and provide the callback function and data
                             sl_sleeptimer_start_periodic_timer(&timer_handle,
                                                                timeout_21,
                                                                &CallbackFunction,
                                                                (void *)&temperature_data,
                                                                0,
                                                                0);
#else
                             CallbackFunction(NULL, &temperature_data);
#endif
                           break;
                           case 2:
                             app_log_info("the actual value of notify is %d  which mean 'The characteristic value shall be indicated'\n", config);
                           break;
                           case 3:
                             app_log_info("the actual value of notify is %d  which mean 'The characteristic value notification and indication are enabled, application decides which one to send.'\n", config);
                           break;
                           // -------------------------------
                           // Default event handler.
                           default:
                           break;
                         }

                  }else{

                      app_log_info("We have a problem because it's not good my button notify it's ON but he didn't send any message to the server for say it's clicked");
                  }

                  // Here we check if the status flag is the same of the server client config

        }else{

            app_log_info("Houston we have a problem! the notify button (for temperature) isn't link to the identifier temperature characteristic! : \n");
        }


        // Here we will see what the actual value of notify (activate, deasable, other ?)
        //sl_bt_gatt_server_client_configuration_t config = (sl_bt_gatt_server_client_configuration_t)


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
