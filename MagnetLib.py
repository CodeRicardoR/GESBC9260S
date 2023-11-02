#! /usr/bin/python3

import sys
import os
import time
import shutil
import platform
import math

ListParams = ["Station","IAGA","Magnetometer","Latitude","Longitude","Altitude","Serial",
"Set","Enable","ScaleH","ScaleD","ScaleZ","OffsetH","OffsetD","OffsetZ","OffsetT1",
"OffsetT2","LBaseH","LBaseD","LBaseZ","Hmean","Dmean","Zmean","cte1","cte2","Temp","TH",
"TZ","PathS","PathM","PathZ","PathT","Server1","Server2","Server3","SFTP","USB"]

DicMonths = {0:['Cero','cer',0],1:['January','jan',31],2:['February','feb',28],3:["March",'mar',31],4:["April",'apr',30],
5:["May",'may',31],6:["June",'jun',30],7:["July",'jul',31],8:["August",'aug',31],9:["September",'sep',30],
10:["October",'oct',31],11:["November",'nov',30],12:["December",'dec',31]}

def CreateFileSetuplog(NameFile):

	sis = platform.system()
	if sis == "Linux":
		port = "/dev/ttyUSB0"
	elif sis == "Windows":
		port = "COM1"
	else:
		port = "None"

	f = open(NameFile,"w")
	f.write('"IGP-ROJ Magnetometer logging setup file Ver 08.21"\n')
	line_time = time.strftime('"%Y-%m-%d %H:%M:%S"')
	f.write('"Updated :                ",' + line_time + '\n\n')
	f.write('"No borrar las comillas ni las comas."\n')
	f.write('"Sirven para separar tipos de datos."\n\n')

	f.write('"Station                  ","STATION"\n')
	f.write('"IAGA code                ","STA"\n')
	f.write('"Magnetometer             ","MAGROJ-XX"\n')
	f.write('"Latitude                 ",x x%c'%39 + ' x%c\n'%34)
	f.write('"Longitude                ",y y%c'%39 + ' y%c\n'%34)
	f.write('"Altitude(m)              ",100\n')
	f.write('"Serial Port              ","' + port +'"\n')
	f.write('"Set interval time (min)  ",3\n')
	f.write('"Enable GPS               ","no"\n\n')

	f.write('"Valores de calibracion instrumental:"\n')
	f.write('"ScaleH value(mV/nT)     ",2.5\n')
	f.write('"ScaleD value(mV/nT)     ",2.5\n')
	f.write('"ScaleZ value(mV/nT)     ",2.5\n\n')
	
	f.write('"Offset Electronico:"\n')
	f.write('"OffsetH(v)               ",.0001\n')
	f.write('"OffsetD(v)               ",.0001\n')
	f.write('"OffsetZ(v)               ",.0001\n')
	f.write('"OffsetT1(v)              ",.0001\n')
	f.write('"OffsetT2(v)              ",.0001\n\n')
	
	f.write('"Linea de base, valor de referencia del lugar (IGRF):"\n')
	f.write('"LBaseH(nT)               ",25750\n')
	f.write('"LBaseD(Grados)           ",-1.4\n')
	f.write('"LBaseZ(nT)               ",350\n\n')
	
	f.write('"Valor promedio anual para la epoca 2020:"\n')
	f.write('"Hmean(nT)                ",25500\n')
	f.write('"Dmean(Grados)            ",-1.4\n')
	f.write('"Zmean(nT)                ",365\n\n')

	f.write('"Constantes de ajuste lineal:"\n')
	f.write('"cte1 Pendiente           ",.30517578125\n')
	f.write('"cte2 Intersecto          ",.00000000001\n\n')

	f.write('"Constantes de ajuste por temperatura (25 C)"\n')
	f.write('"Temp reference(mv)       ",250\n')
	f.write('"TH factor(mv)            ",.0\n')
	f.write('"TZ factor(mv)            ",.0\n\n')

	#Info de ruta de datos:
	DataPathserver1 = "/home/cesar/isr/magnetometer/test/jica"
	DataPathserver2 = "/media/ricardo/datos/magnet/datamin"
	DataPathserver3 = "/home/cesar/isr/magnetometer/test/jica"

	f.write('"Rutas de almacenamiento local (No editar):"\n')
	f.write('"PathS segundos            ","USB Memory flash no mount"\n')
	f.write('"PathM minutos             ","USB Memory flash no mount"\n')
	f.write('"PathZ zipdata             ","USB Memory flash no mount"\n')
	f.write('"PathT tempdata            ","USB Memory flash no mount"\n')
	f.write('"Server1 Path (using FTP)  ","' + DataPathserver1 + '"\n')
	f.write('"Server2 Path (using FTP)  ","' + DataPathserver2 + '"\n')
	f.write('"SFTP allow to send        ","no"\n')
	f.write('"Server3 Path (using SFTP) ","' + DataPathserver3 + '"\n')
	f.write('"USB Device                ","NoUsed"\n')
	f.close()
	print ("Msg : Archivo Setuplog.cfg creado")
	return
	
def ReadFileSetup(NameFile):

	fsetup = open(NameFile,'r')
	lines = fsetup.readlines()
	fsetup.close()
	
	LocParams = {}		
	for line in lines:
		for cadena in ListParams:
			LocParams.setdefault(cadena,"none")
			value = line.find(cadena,0,len(cadena) + 1)
			if value != -1:
				value = line.find(',')
				if value != -1:
					param = line[value + 1:len(line) - 1]
					value = param.find('"',0,2)
					if value != -1:
						LocParams[cadena] = param[1:len(param) - 1]
					else:
						LocParams[cadena] = param
			
	return LocParams

def UpdateSetupFile(NameFile,DataDic):

	fsetup = open(NameFile,'r')
	lines = fsetup.readlines()
	fsetup.close()
	
	params = DataDic.keys()
	for line in lines:
		pos = lines.index(line)
		for key in params:
			value = line.find(key,0,14)
			if value != -1:
				value = line.find(',')
				if line[value + 1] == '"':
					lines[pos] = line[:value + 2] + DataDic[key] + '"\n'
				else:
					lines[pos] = line[:value + 1] + DataDic[key] + '\n'

	shutil.copyfile(NameFile,NameFile + time.strftime('_%d%m%y.bck'))
	os.remove(NameFile)
	
	NewFile = open(NameFile,'w')
	for i in lines:
		NewFile.write(i)
	
	NewFile.close()
	return

def FechaHora(DataDic,StrigTime):

	DataHour = int(''.join(StrigTime[:2]))
	DataMinute = int(''.join(StrigTime[2:4]))
	DataSecond = int(''.join(StrigTime[4:6]))
	DataDay = int(''.join(StrigTime[6:8]))
	DataMon = int(''.join(StrigTime[8:10]))
	DataYea = int(''.join(StrigTime[10:]))

	if (DataYea % 4) == 0:
		DicMonths[2][2] = 29
	
	NumDays = 0
	for i in range(DataMon):
		NumDays = NumDays + DicMonths[i][2]
	NumDays = NumDays + DataDay

	StringFileMin = DataDic['IAGA'].lower() + '%02d'%DataDay + DicMonths[DataMon][1] + '.%02d'%DataYea
	StringFileSec = DataDic['IAGA'].lower() + '%03d'%NumDays + '%02d'%DataHour + '.%02ds'%DataYea
	StringFileZip = DataDic['IAGA'].lower() + '%03d'%NumDays + '%02d.zip'%DataHour
	StringDateMin = '%02d '%DataDay + '%02d '%DataMon + '20%02d '%DataYea + '%02d '%DataHour + '%02d '%DataMinute
	StringDateSec = '%02d '%DataHour + '%02d '%DataMinute + '%02d '%DataSecond
	StringDateUi = '%02d/'%DataDay + '%02d/'%DataMon + '20%02d '%DataYea + '%02d:'%DataHour + '%02d:'%DataMinute + '%02d'%DataSecond

	#Update Data Dirs
	DirMin = DataDic['PathM']
	DirSec = DataDic['PathS']
	DirZip = DataDic['PathZ']
	DirTem = DataDic['PathT']

	DirMon = DicMonths[DataMon][1] + '%02d'%DataDay
	DirSec = os.path.join(DirSec,DirMon)
	DirZip = os.path.join(DirZip,DirMon)

	if not os.path.isdir(DirMin):
		os.makedirs(DirMin)
	if not os.path.isdir(DirSec):
		os.makedirs(DirSec)
	if not os.path.isdir(DirZip):
		os.makedirs(DirZip)
	if not os.path.isdir(DirTem):
		os.makedirs(DirTem)
	
	return (StringFileMin, StringFileSec, StringFileZip, StringDateMin, StringDateSec, StringDateUi, DirSec, DirZip, NumDays)

def CreateFileMinM(DicComp, DataDic,string_DateMin, NameFile, NumDay):

	if not os.path.isfile(NameFile):
		fileMinM = open(NameFile,'w')
		#escribiendo cabecera:
		fileMinM.write(DataDic['Station'] + ' ' + DataDic['Magnetometer'] + ' ' + '<%03d>'%NumDay + ' 1 Min. Reported data.\n\n')
		fileMinM.write('DD MM YYYY HH MM   D(deg)   H(nT)    Z(nT)    I(deg)   F(nT)\n\n')
		fileMinM.close()
	
	if len(DicComp) != 0:
		#Convert to mV
		DicChannelmV = {'X':['0',DataDic['OffsetH']],'Y':['0',DataDic['OffsetD']],'Z':['0',DataDic['OffsetZ']],
		'T1':['0',DataDic['OffsetT1']],'T2':['0',DataDic['OffsetT2']]}

		ValInt = 0.0001
		for key in DicComp:
			ValInt = DicComp[key]*float(DataDic['cte1']) + float(DataDic['cte2']) + float(DicChannelmV[key][1])
			DicChannelmV[key][0] = '%+010.4f'%ValInt
		
		#Correct by temp.
		factorH = (float(DataDic['Temp']) - float(DicChannelmV['T1'][0]))*float(DataDic['TH'])
		factorZ = (float(DataDic['Temp']) - float(DicChannelmV['T1'][0]))*float(DataDic['TZ'])
		ValXCorr = float(DicChannelmV['X'][0]) + factorH
		ValZCorr = float(DicChannelmV['Z'][0]) + factorZ
		DicChannelmV['X'][0] = '%+010.4f'%ValXCorr
		DicChannelmV['Z'][0] = '%+010.4f'%ValZCorr
		
		#Convert to nT
		HnT = float(DataDic['LBaseH']) + float(DicChannelmV['X'][0])/float(DataDic['ScaleH'])
		ZnT = float(DataDic['LBaseZ']) + float(DicChannelmV['Z'][0])/float(DataDic['ScaleZ'])
		DnT = float(DataDic['LBaseD']) + (float(DicChannelmV['Y'][0])/float(DataDic['ScaleD']))*(3438.0/(float(DataDic['Hmean'])*60))
		InT = math.atan2(ZnT,HnT)*(180/3.1416)
		FnT = math.sqrt(math.pow(HnT,2) + math.pow(ZnT,2))

		fileMinM = open(NameFile,'a')
		fileMinM.write(string_DateMin)
		fileMinM.write('%+08.4f '%DnT + '%+08.1f '%HnT + '%+08.1f '%ZnT + '%+08.4f '%InT + '%+08.1f\n'%FnT)
		fileMinM.close()

def CreateFileMinV(DicComp, DataDic,string_DateMin, NameFile, NumDay):

	if not os.path.isfile(NameFile):
		fileMinV = open(NameFile,'w')
		#escribiendo cabecera:
		fileMinV.write(DataDic['Station'] + ' ' + DataDic['Magnetometer'] + ' ' + '<%03d>'%NumDay + ' 1 Min. Reported data.\n\n')
		fileMinV.write('DD MM YYYY HH MM     H(mv)       D(mv)      Z(mv)       Tc(mv)     Ts(mv)\n\n')
		fileMinV.close()
	
	if len(DicComp) != 0:
		#Convert to mV
		DicChannelmV = {'X':['0',DataDic['OffsetH']],'Y':['0',DataDic['OffsetD']],'Z':['0',DataDic['OffsetZ']],
		'T1':['0',DataDic['OffsetT1']],'T2':['0',DataDic['OffsetT2']]}
		ValInt = 0.0001
		for key in DicComp:
			ValInt = DicComp[key]*float(DataDic['cte1']) + float(DataDic['cte2']) + float(DicChannelmV[key][1])
			DicChannelmV[key][0] = '%+011.4f'%ValInt
		
		#Correct by temp.
		factorH = (float(DataDic['Temp']) - float(DicChannelmV['T1'][0]))*float(DataDic['TH'])
		factorZ = (float(DataDic['Temp']) - float(DicChannelmV['T1'][0]))*float(DataDic['TZ'])
		ValXCorr = float(DicChannelmV['X'][0]) + factorH
		ValZCorr = float(DicChannelmV['Z'][0]) + factorZ
		DicChannelmV['X'][0] = '%+011.4f'%ValXCorr
		DicChannelmV['Z'][0] = '%+011.4f'%ValZCorr

		fileMinV = open(NameFile,'a')
		fileMinV.write(string_DateMin)
		fileMinV.write(DicChannelmV['X'][0] + ' ' + DicChannelmV['Y'][0] + ' ' + DicChannelmV['Z'][0] + ' ' + DicChannelmV['T1'][0] + ' ' + DicChannelmV['T2'][0] + '\n')
		fileMinV.close()

def CreateFileSec(DicComp, DataDic,string_DateSec, NameFile, NumDay):

	if not os.path.isfile(NameFile):
		fileSec = open(NameFile,'w')
		#escribiendo cabecera:
		fileSec.write(DataDic['Station'] + ' ' + DataDic['Magnetometer'] + ' ' + '<%03d>'%NumDay + ' 1 Sec. Raw data.\n')
		fileSec.write('ValCte1: ' + DataDic['cte1'] + ' ValCte2: ' + DataDic['cte2'] + ' Temp_Ref:' + DataDic['Temp'] + " TH: " + DataDic['TH'] + ' TZ: ' + DataDic['TZ'] + '\n\n')
		fileSec.write('HH MM SS  CH(H)  CH(D)  CH(Z) CH(Tc) CH(Ts)\n\n')
		fileSec.close()

	if len(DicComp) != 0:
		fileSec = open(NameFile,'a')
		fileSec.write(string_DateSec)
		fileSec.write('%+06d '%DicComp['X'] + '%+06d '%DicComp['Y'] + '%+06d '%DicComp['Z'] + '%+06d '%DicComp['T1'] + '%+06d\n'%DicComp['T2'])
		fileSec.close()


def CreateFilesHistory(DataDic, string_Date, NameFile):

	fileHis = open(NameFile,'a')
	fileHis.write('---------------------------------------------------------\n')
	fileHis.write(string_Date + '\n')
	fileHis.write('Start logging: OK\n')
	fileHis.write('Startup parameters: ' + DataDic['Magnetometer'] + ' ' + DataDic['LBaseH'] + ' ' + DataDic['LBaseD'] + ' ' + DataDic['LBaseZ'] + ' ' + DataDic['ScaleH'] + ' ' + DataDic['ScaleD'] + ' ' + DataDic['ScaleZ'] + '\n')
	fileHis.write('Temp. parameters: ' + DataDic['Temp'] + ' ' + DataDic['TH'] + ' ' + DataDic['TZ'] + '\n')
	fileHis.close()

def CreateFgxfiles(DicServers, FileMinM, FileMinV, FileSec, FileZip, string_Date, NameFile):

	fileFg = open(NameFile,'w')
	fileFg.write('Current FG files list ready for ZIP and FTP\n')
	fileFg.write('Updated : ' + string_Date + '\n')
	fileFg.write(os.path.basename(FileSec) + '\n')
	fileFg.write(os.path.basename(FileMinM) + '\n')
	fileFg.write(os.path.basename(FileMinV) + '\n')
	fileFg.write(os.path.basename(FileZip) + '\n')
	fileFg.write(os.path.dirname(FileSec) + '\n')
	fileFg.write(os.path.dirname(FileMinM) + '\n')
	fileFg.write(os.path.dirname(FileMinV) + '\n')
	fileFg.write(os.path.dirname(FileZip) + '\n')
	fileFg.write(DicServers[1] + '\n')
	fileFg.write(DicServers[2] + '\n')
	fileFg.close()
