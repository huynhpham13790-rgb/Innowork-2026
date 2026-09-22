import serial,sys,time
s=serial.Serial('/dev/ttyACM0',115200,timeout=1)
t=time.time()
while time.time()-t<150:
    l=s.readline()
    if not l: continue
    d=l.decode('utf8','replace')
    if 'ALRM' in d or 'lenh' in d: sys.stdout.write('%5.1fs %s'%(time.time()-t,d)); sys.stdout.flush()
