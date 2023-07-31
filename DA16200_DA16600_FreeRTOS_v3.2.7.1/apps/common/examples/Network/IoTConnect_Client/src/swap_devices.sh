#!/bin/sh
echo "ATE" > /dev/ttyS8

#
# swap to "justatoken"
#
echo "AT+NWICCPID avtds" > /dev/ttyS8
echo "AT+NWICENV avnetpoc" > /dev/ttyS8
echo "AT+NWICAT 1" > /dev/ttyS8
echo "AT+NWICDUID justatoken" > /dev/ttyS8
echo "AT+NWICSK ''" > /dev/ttyS8
echo "AT+NWICSYNC 0" > /dev/ttyS8
sleep 5
echo "AT+NWICSYNC 1" > /dev/ttyS8

sleep 10

#
# swap to "left2myowndevice"
#
echo "AT+NWICCPID avtds" > /dev/ttyS8
echo "AT+NWICENV avnetpoc" > /dev/ttyS8
echo "AT+NWICAT 5" > /dev/ttyS8
echo "AT+NWICDUID left2myowndevice" > /dev/ttyS8
echo "AT+NWICSK cGFzc3dvcmRwYXNzd29yZAo-" > /dev/ttyS8
echo "AT+NWICSYNC 0" > /dev/ttyS8
sleep 5
echo "AT+NWICSYNC 1" > /dev/ttyS8

sleep 10

#
# swap to "binarytestdevice"
#
echo "AT+NWICCPID avtds" > /dev/ttyS8
echo "AT+NWICENV avnetpoc" > /dev/ttyS8
echo "AT+NWICAT 5" > /dev/ttyS8
echo "AT+NWICDUID binarytestdevice" > /dev/ttyS8
echo "AT+NWICSK YzlgdRbYcreYW1fhjwxO4b3X7hBlDY3OVuw6q9wDbAo-" > /dev/ttyS8
echo "AT+NWICSYNC 0" > /dev/ttyS8
sleep 5
echo "AT+NWICSYNC 1" > /dev/ttyS8
