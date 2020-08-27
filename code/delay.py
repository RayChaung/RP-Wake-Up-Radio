import subprocess
import os
import time
from datetime import datetime


def writelog(str):
    LogDIR = "/home/pi/Desktop/log/"
    date = datetime.today().strftime('%Y-%m-%d')
    flog = open(LogDIR + date +".log", "a+")
    flog.write(str)
    flog.write("\n")
    flog.close()

def readStartTime():
    LogDIR = "/home/pi/Desktop/log/"
    date = datetime.today().strftime('%Y-%m-%d')
    try:
        with open(LogDIR + date +".log", "r") as flog:
            for last_line in flog:
                pass
        #print(last_line.split(":"))
        if last_line.split(":")[0] == "start":
            result = last_line.strip().split(":")[1]
        else:
            result = 0
        return int(result)
        
    except:
        return 0



while True:
    proc = subprocess.Popen(["/usr/local/bin/rfrespond"], stdout = subprocess.PIPE)

    expect = "ACKed"
    while True:
        line = proc.stdout.readline()
        
        if not line:
            print("Terminate process")
            break
        print(line.decode().strip())
        if expect in line.decode().strip().split(" "):
            milli_sec = int(round(time.time() * 1000))
            print("start:%d" % milli_sec)
            log = "start:" + str(milli_sec)
            writelog(log)
            print("reboot")
            cmd = "/sbin/reboot"
            os.system(cmd)
            
            

