import serial,sys,time
s=serial.Serial('/dev/ttyACM0',115200,timeout=1)
t=time.time()
while time.time()-t<20:
    l=s.readline()
    if not l: continue
    d=l.decode('utf8','replace')
    if 'BLE' in d or 'ALRM' in d: sys.stdout.write(d); sys.stdout.flush()
