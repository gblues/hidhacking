#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <coreinit/ios.h>

#include <whb/proc.h>
#include <whb/log.h>
#include <whb/log_console.h>
#include <nsysuhs/uhs.h>
#include <nsyshid/hid.h>
#include <vpad/input.h>
#include <fcntl.h>
#include <stdio.h>

#include "hidhacking.h"

// UHSStatus UhsGetDescriptorString(UhsHandle *handle, uint32_t if_handle, uint8_t string_index, uint8_t unknwn, void *out_buffer, size_t out_buffer_size);

int32_t hid_attach_callback(HIDClient *client, HIDDevice *device, HIDAttachEvent attach);

#define HOST_TO_DEVICE 0x00
#define DEVICE_TO_HOST 0x80
#define TYPE_STANDARD  0x00
#define TYPE_CLASS     0x20
#define TYPE_VENDOR    0x40
#define TYPE_RESERVED  0x60
#define TO_DEVICE      0x00
#define TO_INTERFACE   0x01
#define TO_ENDPOINT    0x02
#define TO_OTHER       0x03

int main(int argc, char **argv)
{
   WHBProcInit();
   WHBLogConsoleInit();
   WHBLogPrintf("HID Hacking!");
   WHBLogConsoleDraw();
   HIDClient hid = {0};
   HIDAddClient(&hid, hid_attach_callback);

   while(WHBProcIsRunning()) {
      OSSleepTicks(OSMillisecondsToTicks(100));
   }

   WHBLogPrintf("Exiting... good bye.");
   WHBLogConsoleDraw();

   HIDDelClient(&hid);
   OSSleepTicks(OSMillisecondsToTicks(1000));
   WHBLogConsoleFree();
   WHBProcShutdown();
   return 0;
}

void read_descriptor_string(HIDDevice *device);

int32_t hid_attach_callback(HIDClient *client, HIDDevice *device, HIDAttachEvent attach) {
   int32_t want_disconnect_event = 0;


   WHBLogPrintf("HID device %s", attach ? "connected" : "disconnected\n");
   if(attach) {
      WHBLogPrintf("handle            : 0x%08x\n", device->handle);
      WHBLogPrintf("physicalDeviceInst: 0x%08x\n", device->physicalDeviceInst);
      WHBLogPrintf("vendor_id         : 0x%04x\n", device->vid);
      WHBLogPrintf("product_id        : 0x%04x\n", device->pid);
      WHBLogPrintf("interfaceIndex    : %d\n", device->interfaceIndex);
      WHBLogPrintf("subClass          : %d\n", device->subClass);
      WHBLogPrintf("protocol          : %d\n", device->protocol);
      WHBLogPrintf("rx_packet_size    : %d\n", device->maxPacketSizeRx);
      WHBLogPrintf("tx_packet_size    : %d\n", device->maxPacketSizeTx);
      WHBLogConsoleDraw();
      read_descriptor_string(device);
      want_disconnect_event = 1;
   }
   WHBLogConsoleDraw();

   return want_disconnect_event;
}

void read_descriptor_string(HIDDevice *device) {
   int32_t result;
   size_t read_size;
   char fmtbuffer[256];
   char *top = &fmtbuffer[0];
   uint8_t *read_buffer = (uint8_t *)alloc_cachealigned_zeroed(device->maxPacketSizeRx, &read_size);

   result = HIDGetDescriptor(device->handle, 3, 0, 0, read_buffer, read_size, NULL, NULL);
   for(int i = 0; i < result; i++) {
      sprintf(top, "%02x ", read_buffer[i]);
      top += 3;
   }
   WHBLogPrintf("%s\n", fmtbuffer);
   for(int i = 1; i < 4; i++) {
      result = HIDGetDescriptor(device->handle, 3, i, 0, read_buffer, read_size, NULL, NULL);
      if(result > 0) {
         WHBLogPrintf("result: %d (0x%08x)\n", result, result);
         memset(fmtbuffer, 0, 256);
         top = &fmtbuffer[0];
         for(int i = 2; i < result; i += 2) {
            top[0] = read_buffer[i];
            top++;
         }
         WHBLogPrintf("descriptor string %d: %s\n", i, fmtbuffer);
      }
   }
   
   free(read_buffer);
}