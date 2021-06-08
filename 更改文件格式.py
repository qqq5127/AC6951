import chardet
import os

def strJudgeCode(str):
 return chardet.detect(str)

def readFile(path):
 try:
     f = open(path, 'r')
     filecontent = f.read()
 finally:
     if f:
         f.close()

 return filecontent

def WriteFile(str, path):
 try:
     f = open(path, 'w')
     f.write(str)
 finally:
     if f:
         f.close()

def converCode(path):
 file_con = readFile(path)
 result = strJudgeCode(file_con)
 #print(file_con)
 if result['encoding'] == 'utf-8':
     #os.remove(path)
     a_unicode = file_con.decode('utf-8')
     gb2312 = a_unicode.encode('utf-8')    
     WriteFile(gb2312, path)

def listDirFile(dir):
 list = os.listdir(dir)
 for line in list:
     filepath = os.path.join(dir, line)
     if os.path.isdir(filepath):
         listDirFile(filepath)
     else:
         print(line)
         converCode(filepath)            

if __name__ == '__main__':
 listDirFile(u'.\cpu')