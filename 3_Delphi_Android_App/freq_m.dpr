program freq_m;

uses
  System.StartUpCopy,
  FMX.Forms,
  freq_meter in 'freq_meter.pas' {Form1},
  UsbSerialJNI in 'UsbSerialJNI.pas';

{$R *.res}

begin
  Application.Initialize;
  Application.CreateForm(TForm1, Form1);
  Application.Run;
end.
