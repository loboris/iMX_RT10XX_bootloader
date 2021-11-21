#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import time
from datetime import datetime
import argparse
import struct
import binascii
import traceback
import os
import hashlib
try:
    import serial
except ImportError:
    print("PySerial must be installed, run `pip3 install pyserial`\r\n")
    sys.exit(1)

termios_used = False
termios = None

try:
    import termios
    termios_used = True
    import fcntl
except ImportError:
    termios_used = False
    print("termios not oaded\r\n")


CMD_GET_VERSION          = 0x0000D001
CMD_READ_FLASH           = 0x0000D102
CMD_WRITE_FLASH          = 0x0000D103
CMD_APP_RECORD_READ      = 0x0000D204
CMD_APP_RECORD_WRITE     = 0x0000D205
CMD_APP_GETSHA256        = 0x0000D206

CMD_ERR_OK               = 0x00000000
CMD_ERR_CRC              = 0x0000E101
CMD_ERR_FLASH_WRITE      = 0x0000E102
CMD_ERR_UNKNOWN_CMD      = 0x0000E103
CMD_ERR_ADDRESS          = 0x0000E104
CMD_ERR_LENGTH           = 0x0000E105
CMD_ERR_DATA             = 0x0000E106
CMD_ERR_DATACRC          = 0x0000E107
CMD_ERR_APPREC_READ      = 0x0000E108
CMD_ERR_SHA256           = 0x0000E109
CMD_ERR_BOOTREC_READ     = 0x0000E10A
CMD_ERR_BOOTREC_WRITE    = 0x0000E10B
CMD_ERR_BKPBOOTREC_WRITE = 0x0000E10C
CMD_ERR_FLASHDATACRC     = 0x0000E10D
CMD_ERR_FLASHERASE       = 0x0000E10E

FCFB_BLOCK_ID            = 0x42464346
IVT_BLOCK_ID             = 0x412000D1

DATA_BLOK_SIZE           = 4096
DATA_TX_BLOK_SIZE        = 4096
BOOT_RECORD_SIZE         = 140
APP_RECORD_SIZE          = 60

VERSION = "1.0.1"
UART_DEVICE = '/dev/ttyACM0'
uart = None
uart_is_open = False
debug = False

if termios_used is True:
    stdinfd    = sys.stdin.fileno()
    oldflags   = fcntl.fcntl(stdinfd, fcntl.F_GETFL)
    oldterm    = termios.tcgetattr(stdinfd)
    newattr    = termios.tcgetattr(stdinfd)
    newattr[3] = newattr[3] & ~termios.ICANON & ~termios.ECHO
    termios.tcsetattr(stdinfd, termios.TCSANOW, newattr)
    print("raw data mode selected")

#-----------------
def err_str(code):
    if code == CMD_ERR_OK:
        res = "OK"
    elif code == 0:
        res = "ok"
    elif code == CMD_ERR_CRC:
        res = "Command CRC error"
    elif code == CMD_ERR_FLASH_WRITE:
        res = "Flash write error"
    elif code == CMD_ERR_UNKNOWN_CMD:
        res = "Unknown command"
    elif code == CMD_ERR_ADDRESS:
        res = "Wrong address received"
    elif code == CMD_ERR_LENGTH:
        res = "Wrong length received"
    elif code == CMD_ERR_DATA:
        res = "Data receive error"
    elif code == CMD_ERR_DATACRC:
        res = "Data CRC error"
    elif code == CMD_ERR_FLASHDATACRC:
        res = "Flash Data CRC error"
    elif code == CMD_ERR_APPREC_READ:
        res = "Error reading app record"
    elif code == CMD_ERR_SHA256:
        res = "Firmware SHA error"
    elif code == CMD_ERR_BOOTREC_READ:
        res = "Boot record read error"
    elif code == CMD_ERR_BOOTREC_WRITE:
        res = "Main boot record write error"
    elif code == CMD_ERR_BKPBOOTREC_WRITE:
        res = "Backup boot record write error"
    elif code == CMD_ERR_FLASHERASE:
        res = "Flash erase error"
    elif code == 1000:
        res = "CMD: No response to command"
    elif code == 9999:
        res = "CMD: Exception processing response"
    elif code == 1001:
        res = "CMD: Response CRC error"
    elif code == 1002:
        res = "CMD: No valid response data"
    elif code == 1003:
        res = "CMD: Response data CRC error"
    else:
        res = "unknown response code ({})".format(hex(code))
    return res

#------------------------------
def do_exit(msg = None, err=1):
    if termios_used is True:
        termios.tcsetattr(stdinfd, termios.TCSAFLUSH, oldterm)
        fcntl.fcntl(stdinfd, fcntl.F_SETFL, oldflags)
    if msg is not None:
        if err != 0:
            print("ERROR: {}".format(msg))
        else:
            print("{}".format(msg))
    if uart is not None:
        try:
            uart.close()
        except:
            print("ERROR closing UART")
    sys.exit(err)

#--------------------
def debug_print(msg):
    if debug is True:
        print("DEBUG: {}".format(msg))

#---------------
def uart_init():
    global uart, uart_is_open
    try:
        uart = serial.Serial(
            port     = UART_DEVICE,
            baudrate = 115200,
            bytesize = serial.EIGHTBITS,
            parity   = serial.PARITY_NONE,
            stopbits = serial.STOPBITS_ONE,
            timeout  = 1.25,
            xonxoff  = 0,
            rtscts   = 0,
            interCharTimeout=None
        )
        uart_is_open = True
        time.sleep(1)
    except:
        do_exit("Cannot open UART port ({})".format(UART_DEVICE))

#------------------
def get_response():
    err = 0
    err1 = 0
    resp_data = None
    try:
        resp = uart.read(20)
        if len(resp) == 20:
            debug_print("[get_response] received: {}".format(binascii.hexlify(resp).decode()))
            resp_crc = binascii.crc32(resp[0:16])
            response = struct.unpack("IIIII", resp)
            if response[4] == resp_crc:
                err1 = response[3]
                if response[2] > 0:
                    # some data follows
                    debug_print("[get_response] Read response data (len={})".format(response[2]))
                    resp_data = uart.read(response[2])
                    if len(resp_data) == response[2]:
                        resp_crc = binascii.crc32(resp_data)
                        if resp_crc != response[3]:
                            debug_print("[get_response] Response data CRC error ({} <> {})".format(hex(response[3]), hex(resp_crc)))
                            err = 1003
                    else:
                        debug_print("[get_response] No response data: {} expected, {} received".format(response[2], len(resp_data)))
                        err = 1002
                else:
                    debug_print("[get_response] No response data expected")
                if response[0] != CMD_ERR_OK:
                    debug_print("[get_response] Response error ({})".format(err_str(response[0])))
                    err = response[0]
            else:
                debug_print("[get_response] Response CRC error ({} <> {})".format(hex(response[4]), hex(resp_crc)))
                err = 1001
        else:
            debug_print("[get_response] No response [{}]".format(resp))
            err = 1000
    except Exception as error:
        error_string = repr(error)
        debug_print("[get_response] {}".format(error_string))
        err = 9999
    return (err, resp_data, err1)

#------------------------------------------------
def send_command(cmd, par=0, size=0, data_crc=0):
    # format binary comman request [cmd_code, param, data_len, data_crc, crc]
    cmd_buf = struct.pack('IIII', cmd, par, size, data_crc)
    cmd_crc = binascii.crc32(cmd_buf)
    cmd_bytes = cmd_buf + struct.pack('I', cmd_crc)
    debug_print("[send command] [{}], len={}".format(binascii.hexlify(cmd_bytes).decode(), len(cmd_bytes)))
    # write command to device
    uart.write(cmd_bytes)
    #uart.flush()

    # read response
    res = get_response()
    return res

#-----------------------
def send_data(data_buf):
    # write data to device
    debug_print("[send data] len={}; [{}]".format(len(data_buf), binascii.hexlify(data_buf[0:4]).decode()))
    uart.write(data_buf)
    res = get_response()
    return res

'''
----------------------
Boot record structure:
----------------------
typedef struct _app_ {
	char     	name[16];	// application name, NULL terminated string
	uint32_t 	address;	// application address in Flash (min addr 0x600100000)
	uint32_t 	size;		// application size, 24-bil; upper 8-bits are flags
	uint32_t	timestamp;  // application timestamp;
	uint8_t 	sha256[32];	// application's SHA-256 hash calculated over 'size' bytes from 'address'
}	app_rec_t;				// size: 60 bytes

typedef struct _boot_rec_ {
	char     	ID[16];
	app_rec_t	apps[2];
	uint32_t 	crc;
}	boot_rec_t;				// size: 140 bytes
'''

#---------------
def app_time(t):
    tstr = datetime.fromtimestamp(t).strftime('%H:%M:%S %Y/%m/%d')
    return tstr

#-------------------
def get_boot_info():
    if uart_is_open is False:
        uart_init()
    res = send_command(CMD_APP_RECORD_READ | 0x00030000)
    if res[0] == 0:
        if res[1] is not None:
            if len(res[1]) == (APP_RECORD_SIZE*2):
                print("Boot applications records:")
                print("--------------------------")
                try:
                    apps = []
                    # [name[16] address size sha[32]]
                    idx = 0
                    apps.append(struct.unpack('16sIII32s', res[1][0:APP_RECORD_SIZE]))
                    apps.append(struct.unpack('16sIII32s', res[1][APP_RECORD_SIZE:]))
                    while idx < 2:
                        if (apps[idx][1] == 0) or (apps[idx][2] == 0):
                            if idx > 0:
                                print("")
                            print("Aplication {}:\r\n  Not configured".format(idx+1))
                        else:
                            print("Application {}:".format(idx+1))
                            print("     Name: '{}'".format(apps[idx][0].strip(b'\x00').decode()))
                            print("  Address: {}".format(hex(apps[idx][1])))
                            print("     Size: {}".format(apps[idx][2] & 0x00FFFFFF))
                            print("Timestamp: {}".format(app_time(apps[idx][3])))
                            if (apps[idx][2] & 0x01000000) == 0:
                                print("   Active: No")
                            else:
                                print("   Active: Yes")
                            print("   SHA256: [{}]".format(binascii.hexlify(apps[idx][4]).decode().upper()))
                        idx += 1
                except Exception as error:
                    print("Error while decoding app boot records.")
                    error_string = repr(error)
                    debug_print("Get response: {}".format(error_string))
                print("--------------------------\r\n")
            else:
                print("Wrong response received (len={})".format(len(res[1])))
                debug_print("[{}]".format(binascii.hexlify(res[1]).decode()))
        else:
            print("No response data received")
    else:
        print("Error requesting app boot record ({})".format(err_str(res[0])))

#--------------
def get_info():
    if uart_is_open is False:
        uart_init()
    res = send_command(CMD_GET_VERSION)
    if res[0] == 0:
        print("Device detected: {}\r\n".format(res[1].decode()))
        get_boot_info()
    else:
        print("Error requesting device information ({})\r\n".format(err_str(res[0])))

#---------------------------------------------------
def check_address(addr, length, minaddr=0x60010000):
    if (addr < minaddr) or (addr > 0x607FF000):
        print("ERROR: Address not in range {} - 0x607FF000".format(hex(minaddr)))
        return False
    if (addr % DATA_BLOK_SIZE) != 0:
        print("ERROR: Address must be alligned to 4KB")
        return False
    if (length <= 0) or ((length % DATA_BLOK_SIZE) != 0):
        print("ERROR: Length must be greater tha 0 and 4KB multiple")
        return False
    if ((addr + length) > 0x607FF000):
        print("ERROR: End address greater than 0x607FF000")
        return False

#------------------------------------------
def read_data(address, length, fname=None):
    if length <= 2048:
        length *= DATA_BLOK_SIZE
    if check_address(address, length, 0x60000000) is False:
        return
    if uart_is_open is False:
        uart_init()

    dest_file = None
    if fname is not None:
        try:
            dest_file = open(fname, 'wb')
        except:
            print("Error opening destination file")
            return

    total_length = length
    print("Reading Flash data...")
    tstart = time.time()

    while length > 0:
        res = send_command(CMD_READ_FLASH, address, DATA_BLOK_SIZE)
        if res[0] == 0:
            if res[1] is not None:
                if len(res[1]) == DATA_BLOK_SIZE:
                    #print("  Read at adress {} OK".format(hex(address)))
                    if dest_file is not None:
                        try:
                            dest_file.write(res[1])
                        except:
                            print("  Error writing data at {} to dest file".format(hex(address)))
                    address += DATA_BLOK_SIZE
                    length -= DATA_BLOK_SIZE
                else:
                    print("error at address {}: wrong data length ({})".format(hex(address), len(res)))
                    break
            else:
                print("error at address {}: no data".format(hex(address)))
                break
        else:
            print("error at address {}: no valid response ({})".format(hex(address), err_str(res[0])))
            break

    if dest_file is not None:
        dest_file.close()
        tofile = " to '{}'".format(fname)
    else:
        tofile = ""
    if length == 0:
        tellapsed = time.time() - tstart
        print("{} bytes received in {:.3f} seconds ({:.2f} KB/sec){}".format(total_length, tellapsed, (total_length / tellapsed) / 1024.0, tofile))

#---------------------------
def check_fw_file(src_file):
    addr = 0
    try:
        src_file.seek(0, os.SEEK_SET)
        buf = src_file.read(4)
        info = struct.unpack('I', buf)
        if info[0] != FCFB_BLOCK_ID:
            print("File nat a firmware file: FCFB missing")
            return 0

        src_file.seek(0x1000, os.SEEK_SET)
        buf = src_file.read(8)
        info = struct.unpack('II', buf)
        ivt_id = info[0]
        addr = info[1] & 0xFFFF0000
        if ivt_id != IVT_BLOCK_ID:
            print("File nat a firmware file: IVT id missing")
            return 0
        if (addr < 0x60010000) or (addr > 0x60200000):
            print("File nat a firmware file: wrong address")
            return 0
        return addr
    except:
        addr = 0
    return addr

#--------------------------------------
def get_app_sha(address, size, fw_sha):
    sha = b''
    try:
        res = send_command(CMD_APP_GETSHA256, address, size)
        if res[0] == 0:
            if res[1] is not None:
                if len(res[1]) == (32):
                    fwsha = struct.unpack('32s', res[1])
                    sha = fwsha[0]
                    print("SHA256:")
                    print("----------------------")
                    print("Flashed: [{}]".format(binascii.hexlify(sha).decode().upper()))
                    print("   File: [{}]".format(binascii.hexlify(fw_sha).decode().upper()))
                    print("----------------------\r\n")
                else:
                    print("Wrong response received (len={})".format(len(res[1])))
                    debug_print("[{}]".format(binascii.hexlify(res[1]).decode()))
            else:
                print("No response data received")
        else:
            print("Error requesting app sha ({})".format(err_str(res[0])))
    except:
        sha = b''
    return sha

#-------------------------------------------------
def write_firmware(fname, app_name="MicroPython"):
    try:
        filesize = os.path.getsize(fname)
        src_file = open(fname, 'rb')
    except:
        print("Error opening firmware file")
        return

    if (filesize < 0x10000) or (filesize > 0x200000):
        print("File size must be greater than 64KB and less tham 2MB")
        src_file.close()
        return

    address = check_fw_file(src_file)
    if address == 0:
        src_file.close()
        return

    # read the content of the file into buffer
    src_file.seek(0, os.SEEK_SET)
    srcbuf = src_file.read()
    src_file.close()

    # adjust the file so taht the size is multiple of 4096
    add_bytes = len(srcbuf) % DATA_TX_BLOK_SIZE
    if add_bytes != 0:
        srcbuf = srcbuf + b'\xFF'*(DATA_TX_BLOK_SIZE-add_bytes)
    # Calculate SHA256 of the file
    file_sha = hashlib.sha256(srcbuf).digest()
    if uart_is_open is False:
        uart_init()

    fw_address = address
    fw_length = len(srcbuf)
    length = filesize
    total_length = 0
    print("Write file to flash address {}, size={} ...".format(hex(address), filesize))
    tstart = time.time()
    fidx = 0
    retries = 0

    while length > 0:
        #if fidx > (DATA_TX_BLOK_SIZE*3):
        #    length = 0
        #    break
        buf = srcbuf[fidx:fidx+DATA_TX_BLOK_SIZE]
        data_crc = binascii.crc32(buf)
        # send write to flash request
        blksize = DATA_TX_BLOK_SIZE
        if length == DATA_TX_BLOK_SIZE:
            # last bloc
            blksize |= 0x01000000
        retry = 0
        while retry < 5:
            retry += 1
            res = send_command(CMD_WRITE_FLASH, address, blksize, data_crc)
            if res[0] == 0:
                time.sleep(0.01)
                res = send_data(buf)
                if res[0] == 0:
                    address += DATA_TX_BLOK_SIZE
                    length -= DATA_TX_BLOK_SIZE
                    total_length += DATA_TX_BLOK_SIZE
                    fidx += DATA_TX_BLOK_SIZE
                    break
                else:
                    #print("  error sending data at address {} ({}), try={}".format(hex(address), err_str(res[0]), retry))
                    retries += 1
            else:
                #print("  error sending command at address {} ({})".format(hex(address), err_str(res[0]), retry))
                retries += 1
        if res[0] != 0:
            erridx = ""
            if res[0] == CMD_ERR_FLASHDATACRC:
                erridx = "idx={} ".format(res[2])
            print("  {}[{}]".format(erridx, binascii.hexlify(buf[0:32]).decode()))
            if res[1] is not None:
                print("  [{}]".format(binascii.hexlify(res[1]).decode()))
            break

    if length <= 0:
        tellapsed = time.time() - tstart
        print("{} bytes written in {:.3f} seconds ({:.2f} KB/sec) from '{}'; retries:{}".format(total_length, tellapsed, (total_length / tellapsed) / 1024.0, fname, retries))
        fwsha = get_app_sha(fw_address, fw_length, file_sha)
        if fwsha == file_sha:
            # write the app boot record for the file
            print("Write boot record")
            is_ok = False
            boot_rec = struct.pack('16sIII32s', app_name.encode(), fw_address, fw_length, int(time.time()), file_sha)
            data_crc = binascii.crc32(boot_rec)
            res = send_command(CMD_APP_RECORD_WRITE, address, len(boot_rec), data_crc)
            if res[0] == 0:
                res = send_data(boot_rec)
                if res[0] == 0:
                    is_ok = True
                else:
                    print("  error writting boot record ({})".format(err_str(res[0])))
            else:
                print("  error sending app boot record ({})".format(err_str(res[0])))

#=========================
if __name__ == '__main__':
    def auto_int(x):
        return int(x, 0)

    try:
        parser = argparse.ArgumentParser(
        description="i.MX RT10XX Flash loader.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
        )

        parser.add_argument("-p", "--port", help="COM Port", default="/dev/ttyACM0")
        parser.add_argument("-W", "--write", help="Write file to Flash", default=False, action="store_true")
        parser.add_argument("-E", "--erase", help="Erase the Falsh chip!", default=False, action="store_true")
        parser.add_argument("-R", "--read", help="Read data from Flash", default=False, action="store_true")
        parser.add_argument("-L", "--rdlen", type=auto_int, help="Length of data to read from Flash", default=0)
        parser.add_argument("-a", "--address", type=auto_int, help="Load firmware/data to Flash at address", default=0)
        parser.add_argument("-D", "--debug", help="Print debug messages", default=False, action="store_true")
        parser.add_argument("firmware", nargs='?', help="firmware bin path, can be omited for read and erase commands", default=None)

        args = parser.parse_args()

        print("\r\n---------------------------------")
        print("MX RT10XX firmware loader v.{}".format(VERSION))
        print("---------------------------------\r\n")

        if args.debug is True:
            debug = True

        UART_DEVICE = args.port
        get_info()

        if args.read is True:
            read_data(args.address, args.rdlen, args.firmware)
            do_exit("Finished.", 0)

        if args.write is True:
            if args.firmware is not None:
                write_firmware(args.firmware)
            else:
                print("No firmware file name given.")
            do_exit("Finished.", 0)

        do_exit("Finished.", 0)
    except Exception:
        traceback.print_exc(file=sys.stdout)
        do_exit("Error im main loop.")
