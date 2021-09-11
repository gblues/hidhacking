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

UHSStatus UhsGetDescriptorString(UhsHandle *handle, uint32_t if_handle, uint8_t string_index, uint8_t unknwn, void *out_buffer, size_t out_buffer_size);

void init_uhs_config(UhsConfig *config);
void teardown_uhs_config(UhsConfig *config);
void uhs_acquire_callback(void *context, int32_t p1, int32_t p2);

void print_string_descriptors(UhsHandle * handle, UhsInterfaceProfile *profile);

typedef struct _usb_context usb_context;

int main(int argc, char **argv)
{
   int result;
   
   UhsHandle *handle = alloc_cachealigned_zeroed(sizeof(UhsHandle), NULL);
   UhsConfig *config = alloc_cachealigned_zeroed(sizeof(UhsConfig), NULL);
   init_uhs_config(config);

   WHBProcInit();
   WHBLogConsoleInit();
   WHBLogPrintf("HID Hacking!");
   WHBLogConsoleDraw();

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

   UhsInterfaceProfile profiles[10];
   UhsInterfaceFilter filter = {
      .match_params = MATCH_ANY
   };

   result = UhsQueryInterfaces(handle, &filter, profiles, 10);
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
         break;
   }
   WHBLogConsoleDraw();
   int success = 0;
   for(int i = 0;i<result;i++) {
       result = UhsAcquireInterface(handle, profiles[i].if_handle, 0, 0);

       switch (result) {
           case UHS_STATUS_OK:
               WHBLogPrintf("Successfully aquired device interface: vid: 0x%04X pid: 0x%04X\n", profiles[i].dev_desc.idVendor, profiles[i].dev_desc.idProduct);

               print_string_descriptors(handle, &profiles[i]);
               UhsReleaseInterface(handle, profiles[i].if_handle, true);
               break;
           case UHS_STATUS_HANDLE_INVALID_ARGS:
               WHBLogPrintf("UhsAcquireInterface: invalid arguments\n");
               break;
           case UHS_STATUS_HANDLE_INVALID_STATE:
               WHBLogPrintf("UhsAcquireInterface: invalid state\n");
               break;
       }
   }
   error:
   WHBLogConsoleDraw();

   while(WHBProcIsRunning()) {
      OSSleepTicks(OSMillisecondsToTicks(100));
   }


   WHBLogPrintf("Exiting... good bye.");
   WHBLogConsoleDraw();

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

void print_string_descriptors(UhsHandle * handle, UhsInterfaceProfile *profile) {
    char buffer[512];
    for(int string_index = 0; string_index<4;string_index++){
        memset(buffer, 0, 512);
        if(UhsGetDescriptorString(handle, profile->if_handle, string_index, 0, buffer, 512) > 0) {
            WHBLogPrintf("%d: %s", string_index, buffer);
        }
    }
}
