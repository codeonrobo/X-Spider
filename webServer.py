#!/usr/bin/env/python
# File name   : server.py
# Production  : RaspTankPro
# Website     : www.adeept.com
# Author      : William
# Date        : 2020/03/17

import time
import threading
import os
import info
import socket

#websocket
import asyncio
import websockets

import json
import app

import serial

ser = serial.Serial("/dev/ttyS0",9600)


def ap_thread():
    os.system("sudo create_ap wlan0 eth0 RPiRobot 12345678")


def robotCtrl(command_input, response):
    cmdSend = '0'
    if 'forward' == command_input:
        # ser.write("1\n".encode())
        # ser.write("1".encode())
        cmdSend = '1'
        print(1)
    
    elif 'backward' == command_input:
        # ser.write("2\n".encode())
        # ser.write("2".encode())
        cmdSend = '2'
        print(2)

    elif 'DS' in command_input:
        # ser.write("0\n".encode())
        # ser.write("0".encode())
        cmdSend = '0'
        print(0)


    elif 'left' == command_input:
        cmdSend = '5'
        # ser.write("3\n".encode())

    elif 'right' == command_input:
        cmdSend = '6'
        # ser.write("4\n".encode())

    elif 'TS' in command_input:
        cmdSend = '0'
        # ser.write("0".encode())

    ser.write(cmdSend.encode())


def wifi_check():
    try:
        s =socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
        s.connect(("1.1.1.1",80))
        ipaddr_check=s.getsockname()[0]
        s.close()
        print(ipaddr_check)
    except:
        RL.pause()
        RL.setColor(0,255,64)
        ap_threading=threading.Thread(target=ap_thread)   
        ap_threading.setDaemon(True)                          
        ap_threading.start()
        time.sleep(10)


async def check_permit(websocket):
    while True:
        recv_str = await websocket.recv()
        cred_dict = recv_str.split(":")
        if cred_dict[0] == "admin" and cred_dict[1] == "123456":
            response_str = "congratulation, you have connect with server\r\nnow, you can do something else"
            await websocket.send(response_str)
            return True
        else:
            response_str = "sorry, the username or password is wrong, please submit again"
            await websocket.send(response_str)


async def recv_msg(websocket):
    while True: 
        response = {
            'status' : 'ok',
            'title' : '',
            'data' : None
        }

        data = ''
        data = await websocket.recv()
        try:
            data = json.loads(data)
        except Exception as e:
            print('not A JSON')

        if not data:
            continue

        if isinstance(data,str):
            robotCtrl(data, response)

            if 'get_info' == data:
                response['title'] = 'get_info'
                response['data'] = [info.get_cpu_tempfunc(), info.get_cpu_use(), info.get_ram_info()]

        # print(data)
        response = json.dumps(response)
        await websocket.send(response)


async def main_logic(websocket, path):
    await check_permit(websocket)
    await recv_msg(websocket)


if __name__ == '__main__':
    HOST = ''
    PORT = 10223                              #Define port serial 
    BUFSIZ = 1024                             #Define buffer size
    ADDR = (HOST, PORT)

    global flask_app
    flask_app = app.webapp()
    flask_app.startthread()

    while  1:
        wifi_check()
        try:                  #Start server,waiting for client
            start_server = websockets.serve(main_logic, '0.0.0.0', 8888)
            asyncio.get_event_loop().run_until_complete(start_server)
            print('waiting for connection...')
            # print('...connected from :', addr)
            break
        except Exception as e:
            print(e)

    try:
        asyncio.get_event_loop().run_forever()
    except Exception as e:
        print(e)