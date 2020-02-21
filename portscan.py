#!/usr/bin/env python3
import telnetlib


def portscan(ip, port):
        i = 0
        server = telnetlib.Telnet()
        try:
                server.open(ip, port)
                print('{0} is open'.format(port))
        except Exception as err:
                i += 1
        finally:
                server.close()


n = 50000

sum = 0
counter = 1
while counter <= n:
#       sum = sum + counter
        counter += 1
        portscan('192.168.225.1',counter)
        if (counter%1000 == 0):
                print('now is {0}'.format(counter))

print('end')