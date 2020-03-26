import os, sys
import time

# This program must be run as root
if not os.geteuid()==0:
    sys.exit("Hint: call me as root")

time.sleep(45) # Wait for "rfresponse" send ACK

cmd = "/usr/local/bin/rfwait"
os.system(cmd)

time.sleep(10) # Wait for ~~~

print("Bye!\n")

cmd = "/sbin/shutdown now"
os.system(cmd)

