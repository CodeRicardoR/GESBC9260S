#! /usr/bin/python2.7

import serial
import time
import threading
import sys
import ftplib
import zipfile
from MagnetLib import *



#--------------------------------------------------------------------
def ConvertCMD(letras):

    rx = ''
    for i in letras:
        rx = rx + '%c'%i

    pos = rx.find('JRO')
    if (pos == -1) or (pos > 8):
        id_cmd = 'X'
    else:
        id_cmd = rx[pos + 6]
    
    return id_cmd

def ConvertDAT(letras):

    datos = [0]*12
    rx = ''
    for i in letras:
        rx = rx + '%c'%i
    
    pos = rx.find('JRO')
    if (pos == -1) or (pos > 8):
        id_dat = 'X'
    else:
        id_dat = rx[pos + 6]
        for i in range(len(datos)):
            datos[i] = rx[pos + 7 + i]
    
    return (id_dat,datos)

def SendCMD(ID_CMD):
    global ser
    
    cmd = 'NNNNJROCMD' + ID_CMD + chr(13) + chr(10)

    cmd_tx = bytearray()
    for c in cmd:
        cmd_tx.append(ord(c))

    ser.write(cmd_tx)

    letras = ser.read(13)
    ID = ConvertCMD(letras)
    if ID_CMD != ID:
        print('ERROR: CMD = ' + ID)
        return False
    else:
        return True

class ReceiveThread (threading.Thread):

    def run(self):
        global DicDataSec
        global DataTime
        global DataStream

        self.ser2 = serial.Serial(port= DicParams['Serial'], baudrate=38400, stopbits=1, parity=serial.PARITY_NONE, bytesize=8, timeout = 3)
        self.ser2.flushInput()
        self.ser2.flushOutput()
        #-----------------------------------
        print("Starting receive data...in 1 seconds")
        Second1 = int(time.strftime('%S'))
        while(Second1 == int(time.strftime('%S'))):
                pass
        Second1 = int(time.strftime('%S'))
        while(EnaStream):
            if(self.SendCMD2('9')):
                #Read Header: TimeDate = X..X
                InBUF = self.ser2.read(23)

                #Read Data
                ValX = 0
                ValY = 0
                ValZ = 0
                ValTC = 0
                ValTS = 0
                for s in range(5):
                    InBUF = self.ser2.read(19)
                    
                    RxData = ''
                    for l in InBUF:
                        RxData = RxData + '%c'%l
                    
                    pos = RxData.find('JRO')
                    if (pos == -1) or (pos > 8):
                        count = 255
                        print("JRO No found")
                    else:
                        count = ord(RxData[pos +  6])
                        ValueX  =  ord(RxData[pos +  7]) + ord(RxData[pos +  8])*256
                        ValueY  =  ord(RxData[pos +  9]) + ord(RxData[pos + 10])*256
                        ValueZ  =  ord(RxData[pos + 11]) + ord(RxData[pos + 12])*256
                        ValueTC =  ord(RxData[pos + 13]) + ord(RxData[pos + 14])*256
                        ValueTS =  ord(RxData[pos + 15]) + ord(RxData[pos + 16])*256
                        Max_pos = 2**15 - 1

                        if ValueX > Max_pos:
                            ValueX = ValueX - 2**16
                        if ValueY > Max_pos:
                            ValueY = ValueY - 2**16
                        if ValueZ > Max_pos:
                            ValueZ = ValueZ - 2**16
                        if ValueTC > Max_pos:
                            ValueTC = ValueTC - 2**16
                        if ValueTS > Max_pos:
                            ValueTS = ValueTS - 2**16
                        
                        ValX = ValX + ValueX
                        ValY = ValY + ValueY
                        ValZ = ValZ + ValueZ
                        ValTC = ValTC + ValueTC
                        ValTS = ValTS + ValueTS
						
                DicDataSec['X'] = int(ValX/5.0)
                DicDataSec['Y'] = int(ValY/5.0)
                DicDataSec['Z'] = int(ValZ/5.0)
                DicDataSec['T1'] = int(ValTC/5.0)
                DicDataSec['T2'] = int(ValTS/5.0)
                DataTime = time.strftime("%H%M%S%d%m%y")
                DataStream = True

            #Wait 1 second
            while(Second1 == int(time.strftime('%S'))):
                pass
            Second1 = int(time.strftime('%S'))
        
        #Rutina to stop digitizer:
        while(self.SendCMD2('5') != True):
            time.sleep(0.7)
            print('Stoping Digitizer')
        DataStream = False
        #-----------------------------------	
        print("Finish receive data.")
        self.ser2.close()
    
    def SendCMD2(self, ID_CMD):
        
        cmd = 'NNNNJROCMD' + ID_CMD + chr(13) + chr(10)

        cmd_tx = bytearray()
        for c in cmd:
            cmd_tx.append(ord(c))

        self.ser2.write(cmd_tx)

        letras = self.ser2.read(13)
        ID = self.ConvertCMD2(letras)
        if ID_CMD != ID:
            print('ERROR: CMD = ' + ID)
            return False
        else:
            return True
    
    def ConvertCMD2(self, letras):
        
        rx = ''
        for i in letras:
            rx = rx + '%c'%i

        pos = rx.find('JRO')
        if (pos == -1) or (pos > 8):
            id_cmd = 'X'
        else:
            id_cmd = rx[pos + 6]
        
        return id_cmd
#---------------------------------------------------------------------------------
def thread_SendfileFTP(PathServer, PathData, Pathlisn):

    #Separando strings:
    newpath = os.path.dirname(PathData)
    newfile = os.path.basename(PathData)

    inList = open(Pathlisn, 'r').readlines()

    conta = 0
    for line in inList:
        pos_chr = line.rfind(' ')
        if conta == 0:
            host_ip = line[pos_chr + 1:len(line) - 1]
        elif conta == 1:
            user = line[pos_chr + 1:len(line) - 1]
        elif conta == 2:
            password = line[pos_chr + 1:len(line) - 1]
        conta = conta + 1
    
    os.chdir(newpath)
    resultado = "Error"
    try:
        ftp = ftplib.FTP(host = host_ip, timeout = 30)
        ftp.login(user,password)
        ftp.cwd(PathServer)
        
        f = open(newfile,'rb')
        ftp_cmd = 'STOR ' + newfile
        resultado = ftp.storbinary(ftp_cmd,f)
        f.close()
        ftp.quit()
    except:
        pass

    return resultado


def thread_CompressFile(datapath1, datapath2):

    #Separando las rutas:
    pathzip = os.path.dirname(datapath1)
    filezip = os.path.basename(datapath1)
    
    pathseg = os.path.dirname(datapath2)
    fileseg = os.path.basename(datapath2)

    os.chdir(pathzip)
    zf = zipfile.ZipFile(filezip, mode = "w")
    
    os.chdir(pathseg)
    try:
        zf.write(fileseg,compress_type = zipfile.ZIP_DEFLATEDn)
        zf.close()
    except:
        pass 

    return


DicDataSec = {'X':0, 'Y':0, 'Z':0, 'T1':0, 'T2':0}
DicSumSec  = {'X':0, 'Y':0, 'Z':0, 'T1':0, 'T2':0}
DicPathServers = {1:'PathServer1', 2:'PathServer2'}
ContaSec = 0
ContaMin = 0
DataStream = False

DirWork = "C:\\Users\\magdas\\Documents\\Magnet10V"
PathData = os.path.join(DirWork, 'DataMag')

PathLISN1 = os.path.join(DirWork,'lisn_logon1.txt')
PathLISN2 = os.path.join(DirWork,'lisn_logon1.txt')


FileSetuplog = os.path.join(DirWork, 'Setuplog.cfg')
if not os.path.isfile(FileSetuplog):
    CreateFileSetuplog(FileSetuplog)

#read File Setuplog.cfg
DicParams = ReadFileSetup(FileSetuplog)

print('PathData: ', PathData)
print('Dir Work', DirWork)
os.chdir(DirWork)

DataTime = time.strftime("%H%M%S%d%m%y")
#Update Date/time Files
YearFull = '20' + ''.join(DataTime[10:])
PathMin = os.path.join(PathData,YearFull)
PathMin = os.path.join(PathMin,'DataMin')
PathSec = os.path.join(PathData,YearFull)
PathSec = os.path.join(PathSec,'DataSec')
PathZip = os.path.join(PathData,YearFull)
PathZip = os.path.join(PathZip,'DataZip')
PathTem = os.path.join(PathData,YearFull)
PathTem = os.path.join(PathTem,'DataTem')

DicParams['PathM'] = PathMin
DicParams['PathS'] = PathSec
DicParams['PathZ'] = PathZip
DicParams['PathT'] = PathTem

#Update info in Setuplog.cfg
UpdateSetupFile(FileSetuplog, DicParams)

(DateFileMin, DateFileSec, DateFileZip, FormatDateMin, FormatDateSec, FormatDateUi, DataPathSEC, DataPathZIP, Numday) = FechaHora(DicParams,DataTime)
DataPathMinM = os.path.join(DicParams['PathM'], DateFileMin + 'm')
DataPathMinV = os.path.join(DicParams['PathM'], DateFileMin + 'v')

DataPathSec = os.path.join(DataPathSEC, DateFileSec)
DataPathZip = os.path.join(DataPathZIP, DateFileZip)
SecToZip = DataPathSec


CreateFileMinM({}, DicParams, FormatDateMin, DataPathMinM, Numday)
CreateFileMinV({}, DicParams, FormatDateMin, DataPathMinV, Numday)
CreateFileSec({}, DicParams, FormatDateSec, DataPathSec, Numday)

FileHistory = os.path.join(DirWork,'Historial.log')
CreateFilesHistory(DicParams, FormatDateUi, FileHistory)

DicPathServers[1] = DicParams['Server1'] + '/' + YearFull
DicPathServers[2] = DicParams['Server2'] + '/' + YearFull

FileFgx = os.path.join(DirWork,'Fgxfiles.txt')
CreateFgxfiles(DicPathServers, DataPathMinM, DataPathMinV, DataPathSec, DataPathZip, FormatDateUi, FileFgx)

#Test 1:
print('Path SEC: ', SecToZip)
print('Path ZIP: ', DataPathZip)

ser = serial.Serial(port= DicParams['Serial'], baudrate=38400, stopbits=1, parity=serial.PARITY_NONE, bytesize=8, timeout = 3)
ser.flushInput()
ser.flushOutput()

time.sleep(1)
FlagReset = False
conta = 0
while(SendCMD('5') != True):
    time.sleep(1.3)
    print('Stoping Digitizer')
    conta = conta + 1
    if conta > 50:
        print("Digitizer failed!...")
        sys.exit("***END***")
FlagReset = True

time.sleep(1)
if(FlagReset and SendCMD('0')):
    print("Digitizer OK")
else:
    print("Digitizer failed!...")
    sys.exit("***END***")

ser.close()

time.sleep(1)
print('------------------------')
EnaStream = True
threadRX = ReceiveThread()
threadRX.start()

while(not DataStream):
	pass

ContaSec = 0
SecondNow = int("".join(DataTime[4:6]))
while(True):
#for i in range(600):
    while(SecondNow == int("".join(DataTime[4:6]))):
          pass
    
    #---------------------------------------------------------------------
    # Write Data in files
    #Update Date/time Files
    YearFull = '20' + ''.join(DataTime[10:])
    PathMin = os.path.join(PathData,YearFull)
    PathMin = os.path.join(PathMin,'DataMin')
    PathSec = os.path.join(PathData,YearFull)
    PathSec = os.path.join(PathSec,'DataSec')
    PathZip = os.path.join(PathData,YearFull)
    PathZip = os.path.join(PathZip,'DataZip')
    PathTem = os.path.join(PathData,YearFull)
    PathTem = os.path.join(PathTem,'DataTem')

    DicParams['PathM'] = PathMin
    DicParams['PathS'] = PathSec
    DicParams['PathZ'] = PathZip
    DicParams['PathT'] = PathTem

    (DateFileMin, DateFileSec, DateFileZip, FormatDateMin, FormatDateSec, FormatDateUi, DataPathSEC, DataPathZIP, Numday) = FechaHora(DicParams, DataTime)
    DataPathMinM = os.path.join(DicParams['PathM'], DateFileMin + 'm')
    DataPathMinV = os.path.join(DicParams['PathM'], DateFileMin + 'v')
    DataPathSec = os.path.join(DataPathSEC, DateFileSec)

    NewDataPathZip = os.path.join(DataPathZIP, DateFileZip)
    NewSecToZip = DataPathSec

    ValueSec = int(''.join(DataTime[4:6]))
    ValueMin = int(''.join(DataTime[2:4]))
        
    CreateFileSec(DicDataSec, DicParams, FormatDateSec, DataPathSec, Numday)
    ContaSec = ContaSec + 1
        
    for key in DicDataSec:
        DicSumSec[key] = DicSumSec[key] + DicDataSec[key]
        
    #--------------------------------------------------------------------
    if ValueSec == 59:
        for key in DicSumSec:
            DicSumSec[key] = DicSumSec[key]/ContaSec
            
        CreateFileMinM(DicSumSec, DicParams, FormatDateMin, DataPathMinM, Numday)
        CreateFileMinV(DicSumSec, DicParams, FormatDateMin, DataPathMinV, Numday)
        ContaMin = ContaMin + 1
        print('Data in last minute: ' + str(ContaSec))
        #Reset to zero
        for key in DicSumSec:
            DicSumSec[key] = 0
        ContaSec = 0
    #--------------------------------------------------------------------
    if ValueMin == 0:
        if ValueSec == 20:
            #Compress SecFile
            compress1 = threading.Thread(target=thread_CompressFile, args=(DataPathZip, SecToZip,))
            compress1.start()
            print("Comprimiendo 1")
            #string_CMD = 'python CompressFile.py ' + DataPathZip +  ' ' + SecToZip + ' &'
            #os.system(string_CMD)
            
        if ValueSec == 30:
            #Send ZipFile to server1
            ftp_send5 = threading.Thread(target=thread_SendfileFTP, args=(DicPathServers[1], DataPathZip, PathLISN1,))
            ftp_send5.start()
            print("Enviado...FTP5")

            #Send ZipFile to server2
            ftp_send6 = threading.Thread(target=thread_SendfileFTP, args=(DicPathServers[2], DataPathZip, PathLISN2,))
            ftp_send6.start()
            print("Enviado...FTP6")
            
                
            #Update FilesZip
            DataPathZip = NewDataPathZip
            SecToZip = NewSecToZip
            CreateFgxfiles(DicPathServers, DataPathMinM, DataPathMinV, DataPathSec, DataPathZip, FormatDateUi, FileFgx)
            ContaMin = 0
    #------------------------------------------------------------------
    if ContaMin == int(DicParams['Set']):
        if ValueSec == 15:
            ContaMin = 0
            #Send MinuteFile to server1
            ftp_send1 = threading.Thread(target=thread_SendfileFTP, args=(DicPathServers[1], DataPathMinM, PathLISN1,))
            ftp_send1.start()
            print("Enviado...FTP1")
            
            ftp_send2 = threading.Thread(target=thread_SendfileFTP, args=(DicPathServers[1], DataPathMinV, PathLISN1,))
            ftp_send2.start()
            print("Enviado...FTP2")
            
                
            #Send MinuteFile to server2
            ftp_send3 = threading.Thread(target=thread_SendfileFTP, args=(DicPathServers[2], DataPathMinM, PathLISN2,))
            ftp_send3.start()
            print("Enviado...FTP3")
            
            ftp_send4 = threading.Thread(target=thread_SendfileFTP, args=(DicPathServers[2], DataPathMinV, PathLISN2,))
            ftp_send4.start()
            print("Enviado...FTP4")
           
            
        #Update files and servers
        DicParams = ReadFileSetup(FileSetuplog)
        DicPathServers[1] = DicParams['Server1'] + '/' + YearFull
        DicPathServers[2] = DicParams['Server2'] + '/' + YearFull
        #---------------------------------------------------------------------
        
    SecondNow = int("".join(DataTime[4:6]))

EnaStream = False
while(DataStream):
	pass

print("*** FIN ***")
quit()