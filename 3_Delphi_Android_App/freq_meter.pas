unit freq_meter;

interface

uses
  System.SysUtils, System.Types, System.UITypes, System.Classes, System.Variants,
  FMX.Types, FMX.Controls, FMX.Forms, FMX.Graphics, FMX.Dialogs, FMX.StdCtrls,
  FMX.Controls.Presentation,
  Androidapi.Helpers, Androidapi.JNIBridge, Androidapi.JNI.JavaTypes,
  Androidapi.JNI.GraphicsContentViewText, Androidapi.JNI.App, UsbSerialJNI;

type
  TForm1 = class(TForm)
    lblFrequency: TLabel;
    btnConnect: TButton;
    tmrPermission: TTimer;
    procedure btnConnectClick(Sender: TObject);
    procedure tmrPermissionTimer(Sender: TObject);
    procedure FormDestroy(Sender: TObject);
  private
    FManager: JUsbManager;
    FDriver: JUsbSerialDriver;
    FConnection: JUsbDeviceConnection;
    FPort: JUsbSerialPort;
    FReading: Boolean;
    RxBuffer: string;
    procedure OpenPort;
    procedure StartReading;
    procedure PostChunk(const Chunk: string);
    procedure HandleIncoming(const Chunk: string);
  public
  end;

var
  Form1: TForm1;

implementation

{$R *.fmx}

const
  FLAG_MUTABLE = $02000000;

function FormatFreq(Hz: Int64): string;
begin
  if Hz >= 1000000 then
    Result := Format('%.3f MHz', [Hz / 1e6], TFormatSettings.Invariant)
  else if Hz >= 1000 then
    Result := Format('%.3f kHz', [Hz / 1e3], TFormatSettings.Invariant)
  else
    Result := Format('%d Hz', [Hz]);
end;

procedure TForm1.HandleIncoming(const Chunk: string);
var
  P: Integer;
  Line: string;
  Freq: Int64;
begin
  RxBuffer := RxBuffer + Chunk;
  P := RxBuffer.IndexOf(#10);
  while P >= 0 do
  begin
    Line := RxBuffer.Substring(0, P).Trim;
    RxBuffer := RxBuffer.Substring(P + 1);
    if TryStrToInt64(Line, Freq) then
      lblFrequency.Text := FormatFreq(Freq);
    P := RxBuffer.IndexOf(#10);
  end;
end;

procedure TForm1.PostChunk(const Chunk: string);
var
  S: string;
  Proc: TThreadProcedure;
begin
  S := Chunk;
  Proc := procedure
  begin
    HandleIncoming(S);
  end;
  TThread.Queue(nil, Proc);
end;

procedure TForm1.StartReading;
begin
  FReading := True;
  TThread.CreateAnonymousThread(
    procedure
    var
      Buf: TJavaArray<Byte>;
      Bytes: TBytes;
      N: Integer;
    begin
      Buf := TJavaArray<Byte>.Create(256);
      try
        try
          while FReading do
          begin
            N := FPort.read(Buf, 200);
            if N > 0 then
            begin
              SetLength(Bytes, N);
              Move(Buf.Data^, Bytes[0], N);
              PostChunk(TEncoding.ASCII.GetString(Bytes));
            end;
          end;
        except
          FReading := False;
        end;
      finally
        Buf.Free;
      end;
    end).Start;
end;

procedure TForm1.OpenPort;
begin
  FConnection := FManager.openDevice(FDriver.getDevice);
  if FConnection = nil then
  begin
    lblFrequency.Text := 'Cannot open device';
    Exit;
  end;
  FPort := TJUsbSerialPort.Wrap((FDriver.getPorts.get(0) as ILocalObject).GetObjectID);
  FPort.open(FConnection);
  FPort.setParameters(9600, 8, 1, 0);
  lblFrequency.Text := 'Waiting for data...';
  StartReading;
end;

procedure TForm1.btnConnectClick(Sender: TObject);
var
  Drivers: JList;
  Dev: JUsbDevice;
  Intent: JIntent;
  PI: JPendingIntent;
begin
  FManager := TJUsbManager.Wrap(
    (TAndroidHelper.Context.getSystemService(StringToJString('usb')) as ILocalObject).GetObjectID);

  Drivers := TJUsbSerialProber.JavaClass.getDefaultProber.findAllDrivers(FManager);
  if Drivers.isEmpty then
  begin
    lblFrequency.Text := 'No USB device';
    Exit;
  end;

  FDriver := TJUsbSerialDriver.Wrap((Drivers.get(0) as ILocalObject).GetObjectID);
  Dev := FDriver.getDevice;

  if FManager.hasPermission(Dev) then
    OpenPort
  else
  begin
    Intent := TJIntent.JavaClass.init(StringToJString('com.freqmeter.USB_PERMISSION'));
    Intent.setPackage(TAndroidHelper.Context.getPackageName);
    PI := TJPendingIntent.JavaClass.getBroadcast(TAndroidHelper.Context, 0, Intent, FLAG_MUTABLE);
    FManager.requestPermission(Dev, PI);
    lblFrequency.Text := 'Allow USB access...';
    tmrPermission.Enabled := True;
  end;
end;

procedure TForm1.tmrPermissionTimer(Sender: TObject);
begin
  if FManager.hasPermission(FDriver.getDevice) then
  begin
    tmrPermission.Enabled := False;
    OpenPort;
  end;
end;

procedure TForm1.FormDestroy(Sender: TObject);
begin
  FReading := False;
  try
    if FPort <> nil then FPort.close;
  except
  end;
end;

end.
