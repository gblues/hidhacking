#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <coreinit/ios.h>

#include <whb/proc.h>
#include <whb/log.h>
#include <whb/log_console.h>
#include <nsysuhs/uhs.h>
#include <fcntl.h>

#include "hidhacking.h"

void init_uhs_config(UhsConfig *config);
void teardown_uhs_config(UhsConfig *config);

int main(int argc, char **argv)
{
   int result;
   int uhs_fd = -1;
   UhsHandle  *handle = alloc_cachealigned_zeroed(sizeof(UhsHandle), NULL);
   UhsConfig *config = alloc_cachealigned_zeroed(sizeof(UhsConfig), NULL);
   init_uhs_config(config);

   
   WHBProcInit();
   WHBLogConsoleInit();
   WHBLogPrintf("HID Hacking!");
   WHBLogConsoleDraw();

   uhs_fd = IOS_Open("/dev/uhs/0", IOS_OPEN_READWRITE);
   if(uhs_fd >= 0) {
      WHBLogPrintf("Successfully opened /dev/uhs/0\n");
      handle->handle = uhs_fd;
   }

   result = UhsClientOpen(handle, config);
   switch(result) {
      case UHS_STATUS_OK:
         WHBLogPrintf("Successfully opened UHS client!");
         break;
      case UHS_STATUS_HANDLE_INVALID_ARGS:
         WHBLogPrintf("UHS failed: invalid arguments");
         break;
      case UHS_STATUS_HANDLE_INVALID_STATE:
         WHBLogPrintf("UHS failed: invalid state");
         break;
   }
   WHBLogConsoleDraw();
   while(WHBProcIsRunning()) {
      
      OSSleepTicks(OSMillisecondsToTicks(100));
   }

   WHBLogPrintf("Exiting... good bye.");
   WHBLogConsoleDraw();
   if(result == UHS_STATUS_OK) {
      UhsClientClose(handle);
   }
   if(uhs_fd >= 0) {
      IOS_Close(uhs_fd);
      uhs_fd = -1;
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

   init_cachealigned_buffer(1024, &buffer, &size);
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