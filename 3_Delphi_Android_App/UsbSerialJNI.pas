unit UsbSerialJNI;

interface

uses
  Androidapi.JNIBridge,
  Androidapi.JNI.JavaTypes,
  Androidapi.JNI.App;   // JPendingIntent

type
  JUsbDevice = interface;
  JUsbDeviceConnection = interface;
  JUsbManager = interface;
  JUsbSerialPort = interface;
  JUsbSerialDriver = interface;
  JUsbSerialProber = interface;

  // ---- android.hardware.usb.UsbDevice ----
  JUsbDeviceClass = interface(JObjectClass)
    ['{7C1E5A42-3B0D-4E6F-9A11-5D2C8F4B6E01}']
  end;
  [JavaSignature('android/hardware/usb/UsbDevice')]
  JUsbDevice = interface(JObject)
    ['{2F9B8D13-6A47-4C25-B3E8-0A7D1C5F9E02}']
    function getDeviceName: JString; cdecl;
  end;
  TJUsbDevice = class(TJavaGenericImport<JUsbDeviceClass, JUsbDevice>) end;

  // ---- android.hardware.usb.UsbDeviceConnection ----
  JUsbDeviceConnectionClass = interface(JObjectClass)
    ['{B84D2E6A-1F93-47C0-8D5B-3E6A9C0F1D03}']
  end;
  [JavaSignature('android/hardware/usb/UsbDeviceConnection')]
  JUsbDeviceConnection = interface(JObject)
    ['{5A3C7F91-D2E4-4B68-A0F7-8C1B6E3D2A04}']
    procedure close; cdecl;
  end;
  TJUsbDeviceConnection = class(TJavaGenericImport<JUsbDeviceConnectionClass, JUsbDeviceConnection>) end;

  // ---- android.hardware.usb.UsbManager ----
  JUsbManagerClass = interface(JObjectClass)
    ['{E6D1A4B8-9C35-4F72-B1A0-7D4E2F8C5B05}']
  end;
  [JavaSignature('android/hardware/usb/UsbManager')]
  JUsbManager = interface(JObject)
    ['{19F7C3A5-4E8B-4D26-9C3A-6B0D1E7F8A06}']
    function hasPermission(device: JUsbDevice): Boolean; cdecl;
    procedure requestPermission(device: JUsbDevice; pi: JPendingIntent); cdecl;
    function openDevice(device: JUsbDevice): JUsbDeviceConnection; cdecl;
  end;
  TJUsbManager = class(TJavaGenericImport<JUsbManagerClass, JUsbManager>) end;

  // ---- com.hoho.android.usbserial.driver.UsbSerialPort (a Java interface) ----
  JUsbSerialPortClass = interface(IJavaClass)
    ['{C2B5E8D4-7A13-4F90-A6C2-1D9E3B5F7A07}']
  end;
  [JavaSignature('com/hoho/android/usbserial/driver/UsbSerialPort')]
  JUsbSerialPort = interface(IJavaInstance)
    ['{8D4A1F6C-3E92-4B57-8F0D-2A6C9E1B4D08}']
    procedure open(connection: JUsbDeviceConnection); cdecl;
    procedure close; cdecl;
    procedure setParameters(baudRate, dataBits, stopBits, parity: Integer); cdecl;
    function read(dest: TJavaArray<Byte>; timeout: Integer): Integer; cdecl;
    procedure write(src: TJavaArray<Byte>; timeout: Integer); cdecl;
  end;
  TJUsbSerialPort = class(TJavaGenericImport<JUsbSerialPortClass, JUsbSerialPort>) end;

  // ---- com.hoho.android.usbserial.driver.UsbSerialDriver (a Java interface) ----
  JUsbSerialDriverClass = interface(IJavaClass)
    ['{F3A7D2E9-5B41-4C86-B9D3-0E8A1C6F2B09}']
  end;
  [JavaSignature('com/hoho/android/usbserial/driver/UsbSerialDriver')]
  JUsbSerialDriver = interface(IJavaInstance)
    ['{40E9B6C1-8D2A-4F73-A5E1-9B3C7D0F6A10}']
    function getDevice: JUsbDevice; cdecl;
    function getPorts: JList; cdecl;
  end;
  TJUsbSerialDriver = class(TJavaGenericImport<JUsbSerialDriverClass, JUsbSerialDriver>) end;

  // ---- com.hoho.android.usbserial.driver.UsbSerialProber (a class, has a static method) ----
  JUsbSerialProberClass = interface(JObjectClass)
    ['{D7C4F1A8-2B6E-4930-8E5A-1F9D3B7C0E11}']
    function getDefaultProber: JUsbSerialProber; cdecl;
  end;
  [JavaSignature('com/hoho/android/usbserial/driver/UsbSerialProber')]
  JUsbSerialProber = interface(JObject)
    ['{6B2E9D05-A4C8-4F17-B3D6-8E1A5C7F9B12}']
    function findAllDrivers(manager: JUsbManager): JList; cdecl;
  end;
  TJUsbSerialProber = class(TJavaGenericImport<JUsbSerialProberClass, JUsbSerialProber>) end;

implementation

end.
