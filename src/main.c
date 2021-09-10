#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <coreinit/ios.h>

#include <whb/proc.h>
#include <whb/log.h>
#include <whb/log_console.h>
#include <nsysuhs/uhs.h>
#include <vpad/input.h>
#include <fcntl.h>

#include "hidhacking.h"

void init_uhs_config(UhsConfig *config);
void teardown_uhs_config(UhsConfig *config);
void uhs_acquire_callback(void *context, int32_t p1, int32_t p2);

void print_profile_device(UhsInterfaceProfile *profile);
void print_profile_config(UhsInterfaceProfile *profile);
void print_profile_interface(UhsInterfaceProfile *profile);
void print_profile_endpoints(UhsInterfaceProfile *profile);

struct _usb_context {
   UhsHandle *handle;
   UhsConfig *config;
   UhsInterfaceProfile *profile;
};

typedef struct _usb_context usb_context;

int main(int argc, char **argv)
{
   int result;
   bool acquired = false;
   usb_context ctx = {0};
   
   UhsHandle  *handle = alloc_cachealigned_zeroed(sizeof(UhsHandle), NULL);
   UhsConfig *config = alloc_cachealigned_zeroed(sizeof(UhsConfig), NULL);
   init_uhs_config(config);

   
   WHBProcInit();
   WHBLogConsoleInit();
   WHBLogPrintf("HID Hacking!");
   WHBLogConsoleDraw();
   ctx.handle = handle;
   ctx.config = config;

   result = UhsClientOpen(handle, config);
   switch(result) {
      case UHS_STATUS_OK:
         WHBLogPrintf("Successfully opened UHS client!\n");
         break;
      case UHS_STATUS_HANDLE_INVALID_ARGS:
         WHBLogPrintf("UhsClientOpen: invalid arguments\n");
         goto error;
         break;
      case UHS_STATUS_HANDLE_INVALID_STATE:
         WHBLogPrintf("UhsClientOpen: invalid state\n");
         goto error;
         break;
   }
   WHBLogConsoleDraw();

   UhsInterfaceProfile profile = {0};
   UhsInterfaceFilter filter = {
      .match_params = MATCH_ANY
   };

   result = UhsQueryInterfaces(handle, &filter, &profile, 1);
   switch(result) {
      case UHS_STATUS_OK:
         WHBLogPrintf("Successfully queried USB interface\n");
         break;
      case UHS_STATUS_HANDLE_INVALID_ARGS:
         WHBLogPrintf("UhsQueryInterfaces: invalid arguments\n");
         goto error;
         break;
      case UHS_STATUS_HANDLE_INVALID_STATE:
         WHBLogPrintf("UhsQueryInterfaces: invalid state\n");
         goto error;
         break;
      default:
         if(result < 0) {
            WHBLogPrintf("UhsQueryInterfaces: got unexpected UhsResult 0x%08x\n", result);
            goto error;
         }
         WHBLogPrintf("Query found %d USB devices\n", result);
         WHBLogPrintf("if_handle = 0x%08x\n", profile.if_handle);
         ctx.profile = &profile;
         break;
   }
   WHBLogConsoleDraw();

   result = UhsAcquireInterface(handle, profile.if_handle, &ctx, uhs_acquire_callback);
   switch(result) {
      case UHS_STATUS_OK:
         WHBLogPrintf("Successfully aquired device interface\n");
         break;
      case UHS_STATUS_HANDLE_INVALID_ARGS:
         WHBLogPrintf("UhsAcquireInterface: invalid arguments\n");
         goto error;
         break;
      case UHS_STATUS_HANDLE_INVALID_STATE:
         WHBLogPrintf("UhsAcquireInterface: invalid state\n");
         goto error;
         break;
      default:
         if(result < 0) {
            WHBLogPrintf("UhsAcquireInterface: got unexpected UhsResult 0x%08x\n", result);
            goto error;
         }
         WHBLogPrintf("UhsAcquireInterface: returned %d\n", result);
         break;
   }
   acquired = true;
   WHBLogConsoleDraw();
   uint8_t buffer[8192] = {0};

   result = UhsSubmitControlRequest(handle, 
                        profile.if_handle, 
                        buffer, 
                        ENDPOINT_TRANSFER_IN, 
                        0x06, // LIBUSB_REQUEST_GET_DESCRIPTOR
                        0x03 << 8, 
                        0, 
                        8192, 
                        1000);
   switch(result) {
      case UHS_STATUS_OK:
         WHBLogPrintf("Control request successful\n");
         break;
      case UHS_STATUS_HANDLE_INVALID_ARGS:
         WHBLogPrintf("UhsSubmitControlRequest: invalid arguments\n");
         goto error;
         break;
      case UHS_STATUS_HANDLE_INVALID_STATE:
         WHBLogPrintf("UhsSubmitControlRequest: invalid state\n");
         goto error;
         break;
      default:
         if(result < 0) {
            WHBLogPrintf("UhsSubmitControlRequest: got unexpected UhsResult 0x%08x\n", result);
            goto error;
         }
         break;
   }
   WHBLogConsoleDraw();
   buffer[8191] = '\0'; // just in case
   WHBLogPrintf("buffer output: %s\n", buffer);
   error:
   WHBLogConsoleDraw();
   int outcmd = 0;

   while(WHBProcIsRunning()) {
      VPADStatus vpad = {0};
      VPADReadError error = 0;
      if(VPADRead(VPAD_CHAN_0, &vpad, 1, &error)) {
         if(vpad.trigger & VPAD_BUTTON_A) {
            switch(outcmd % 4) {
               case 0:
                  print_profile_device(&profile);
                  break;
               case 1:
                  print_profile_config(&profile);
                  break;
               case 2:
                  print_profile_interface(&profile);
                  break;
               case 3:
                  print_profile_endpoints(&profile);
                  break;
            }
            outcmd++;
            WHBLogConsoleDraw();
         }
      }

      OSSleepTicks(OSMillisecondsToTicks(100));
   }


   WHBLogPrintf("Exiting... good bye.");
   WHBLogConsoleDraw();
   if(acquired) {
      UhsReleaseInterface(handle, profile.if_handle, true);
   }

   if(handle->state > UHS_HANDLE_STATE_INIT) {
      UhsClientClose(handle);
   }

   teardown_uhs_config(config);
   free(handle);
   free(config);
   OSSleepTicks(OSMillisecondsToTicks(1000));

   WHBLogConsoleFree();
   WHBProcShutdown();
   return 0;
}

void init_uhs_config(UhsConfig *config) {
   config->controller_num = 0;
   void *buffer = NULL;
   uint32_t size = 0;

   init_cachealigned_buffer(5120, &buffer, &size);
   config->buffer = buffer;
   config->buffer_size = size;
}

void teardown_uhs_config(UhsConfig *config) {
   if(config->buffer) {
      free(config->buffer);
      config->buffer = NULL;
      config->buffer_size = 0;
   }
}

void uhs_acquire_callback(void *context, int32_t p1, int32_t p2) {
   usb_context *ctx = (usb_context *)context;
   WHBLogPrintf("p1: %08x p2: %08x", p1, p2);
   WHBLogConsoleDraw();
}

void print_profile_device(UhsInterfaceProfile *profile) {
   WHBLogPrintf("Device Descriptor:\n");
   WHBLogPrintf("  bLength = %d\n", profile->dev_desc.bLength);
   WHBLogPrintf("  bDescriptorType = 0x%02x\n", profile->dev_desc.bDescriptorType);
   WHBLogPrintf("  bcdUsb = 0x%04x\n", profile->dev_desc.bcdUsb);
   WHBLogPrintf("  bDeviceClass = 0x%02x\n", profile->dev_desc.bDeviceClass);
   WHBLogPrintf("  bDeviceSubclass = 0x%02x\n", profile->dev_desc.bDeviceSubclass);
   WHBLogPrintf("  bDeviceProtocol = 0x%02x\n", profile->dev_desc.bDeviceProtocol);
   WHBLogPrintf("  bMaxPacketSize = %d\n", profile->dev_desc.bMaxPacketSize);
   WHBLogPrintf("  idVendor = 0x%04x\n", profile->dev_desc.idVendor);
   WHBLogPrintf("  idProduct = 0x%04x\n", profile->dev_desc.idProduct);
   WHBLogPrintf("  bcdDevice = 0x%04x\n", profile->dev_desc.bcdDevice);
   WHBLogPrintf("  iManufacturer = 0x%02x\n", profile->dev_desc.iManufacturer);
   WHBLogPrintf("  iProduct = 0x%02x\n", profile->dev_desc.iProduct);
   WHBLogPrintf("  iSerialNumber = 0x%02x\n", profile->dev_desc.iSerialNumber);
   WHBLogPrintf("  bNumConfigurations = %d\n", profile->dev_desc.bNumConfigurations);
}

void print_profile_config(UhsInterfaceProfile *profile) {
   WHBLogPrintf("Config Descriptor:\n");
   WHBLogPrintf("  bLength = %d\n", profile->cfg_desc.bLength);
   WHBLogPrintf("  bDescriptorType = 0x%02x\n", profile->cfg_desc.bDescriptorType);
   WHBLogPrintf("  wTotalLength = %d\n", profile->cfg_desc.wTotalLength);
   WHBLogPrintf("  bNumInterfaces = %d\n", profile->cfg_desc.bNumInterfaces);
   WHBLogPrintf("  bConfigurationValue = 0x%02x\n", profile->cfg_desc.bConfigurationValue);
   WHBLogPrintf("  iConfiguration = 0x%02x\n", profile->cfg_desc.iConfiguration);
   WHBLogPrintf("  bmAttributes = 0x%02x\n", profile->cfg_desc.bmAttributes);
   WHBLogPrintf("  bMaxPower = %d\n", profile->cfg_desc.bMaxPower);

}

void print_profile_interface(UhsInterfaceProfile *profile) {
   WHBLogPrintf("Interface Descriptor:\n");
   WHBLogPrintf("  bLength = %d\n", profile->if_desc.bLength);
   WHBLogPrintf("  bDescriptorType = 0x%02x\n", profile->if_desc.bDescriptorType);
   WHBLogPrintf("  bInterfaceNumber = %d\n", profile->if_desc.bInterfaceNumber);
   WHBLogPrintf("  bAlternateSetting = %d\n", profile->if_desc.bAlternateSetting);
   WHBLogPrintf("  bNumEndpoints = %d\n", profile->if_desc.bNumEndpoints);
   WHBLogPrintf("  bInterfaceClass = 0x%02x\n", profile->if_desc.bInterfaceClass);
   WHBLogPrintf("  bInterfaceSubClass = 0x%02x\n", profile->if_desc.bInterfaceSubClass);
   WHBLogPrintf("  bInterfaceProtocol = 0x%02x\n", profile->if_desc.bInterfaceProtocol);
   WHBLogPrintf("  iInterface = 0x%02x\n", profile->if_desc.iInterface);
}

void print_profile_endpoints(UhsInterfaceProfile *profile) {
   for(int i = 0; i < 16; i++) {
      if(profile->in_endpoints[i].bLength > 0) {
         WHBLogPrintf("Input endpoint:\n");
         WHBLogPrintf("  bLength = %d\n", profile->in_endpoints[i].bLength);
         WHBLogPrintf("  bDescriptorType = 0x%02x\n", profile->in_endpoints[i].bDescriptorType);
         WHBLogPrintf("  bEndpointAddress = 0x%02x\n", profile->in_endpoints[i].bEndpointAddress);
         WHBLogPrintf("  bmAttributes = 0x%02x\n", profile->in_endpoints[i].bmAttributes);
         WHBLogPrintf("  wMaxPacketSize = %d\n", profile->in_endpoints[i].wMaxPacketSize);
         WHBLogPrintf("  bInterval = 0x%02x\n", profile->in_endpoints[i].bInterval);
      }
      if(profile->out_endpoints[i].bLength > 0) {
         WHBLogPrintf("Output endpoint:\n");
         WHBLogPrintf("  bLength = %d\n", profile->out_endpoints[i].bLength);
         WHBLogPrintf("  bDescriptorType = 0x%02x\n", profile->out_endpoints[i].bDescriptorType);
         WHBLogPrintf("  bEndpointAddress = 0x%02x\n", profile->out_endpoints[i].bEndpointAddress);
         WHBLogPrintf("  bmAttributes = 0x%02x\n", profile->out_endpoints[i].bmAttributes);
         WHBLogPrintf("  wMaxPacketSize = %d\n", profile->out_endpoints[i].wMaxPacketSize);
         WHBLogPrintf("  bInterval = 0x%02x\n", profile->out_endpoints[i].bInterval);
      }
   }
}