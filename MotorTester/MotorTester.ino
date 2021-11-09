#define reversePIN 2
#define driverPUL 7
#define driverDIR 6

int pd = 700;
boolean setdir = LOW;

void startstop()
{
  if( pd == 700 )
  {
    pd = 0;
  }
  else if ( pd == 0 )
  {
    pd = 700;
  }
}

void setup() {
  // put your setup code here, to run once:
  pinMode( driverPUL, OUTPUT );
  pinMode( driverDIR, OUTPUT );
  attachInterrupt(digitalPinToInterrupt(reversePIN), startstop, FALLING);
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(driverDIR, setdir);
  digitalWrite(driverPUL, HIGH);
  delayMicroseconds(pd);
  digitalWrite(driverPUL, LOW);
  delayMicroseconds(pd);

}
