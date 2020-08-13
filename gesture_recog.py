import sqlite3
import datetime

accel_db =  '__HOME__/gesture_recog/accel.db'


def gcd(a, b):
    """Compute the greatest common divisor of a and b"""
    while b > 0:
        a, b = b, a % b
    return a


def lcm(a, b):
    """Compute the lowest common multiple of a and b"""
    return a * b / gcd(a, b)

def make_list(string):
    ans=[]
    temp=''
    for i in range(len(string)):
        if (string[i]!='_'):
            temp=temp+string[i]
        elif (string[i]=='_'):
            ans.append(float(temp))
            temp=''
    return ans

def downsample(x,q):
    if q < 1 or not isinstance(q, int):
        return []
    else:
        list1=[]
        for i in range(len(x)):
            if (i % (q))==0:
                list1.append(x[i])
        return list1


def upsample(x,p):
    if p < 1 or not isinstance(p, int):
        return []
    else:
        list1=[]
        for i in range(len(x)-1):
            dy=(x[i+1]-x[i])/p
            list1.append(x[i])
            for j in range(1,p):
                list1.append(x[i]+j*dy)
        list1.append(x[-1])
        dy1=list1[-1]-list1[-2]
        for m in range(p-1):
            list1.append(list1[-1]+dy1)
        return list1


def resample(inp,desired_length):
    lcm1=lcm(len(inp),desired_length)
    up=int (lcm1/len(inp))
    print(up)
    down=int(len(inp)*up/desired_length)
    print(down)
    inp= upsample(inp,up)
    inp= downsample(inp,down)
    return inp


def offset_and_normalize(inp):
    avg = sum(inp) / len(inp)
    norm = 0
    for j in range(len(inp)):
        norm+=(inp[j] - avg)**2
    norm=norm**(1/2)
    for i in range(len(inp)):
        inp[i] = (inp[i] - avg) / norm
    return inp

def correlation(x,y):
    x = offset_and_normalize(x)
    y = offset_and_normalize(y)
    corr=0
    for i in range(len(x)):
        corr+=x[i]*y[i]
    return corr

def request_handler(request):
    conn = sqlite3.connect(accel_db)
    c = conn.cursor()
    c.execute('''CREATE TABLE IF NOT EXISTS accel_table (user text, x text, timing timestamp);''')
    if request['method'] == "POST":
        sub1 = c.execute(
            '''SELECT x FROM accel_table WHERE user = ? ;''',
            ('forward_backward',)).fetchone()
        sub2 = c.execute(
            '''SELECT x FROM accel_table WHERE user = ? ;''',
            ('forward',)).fetchone()
        sub3 = c.execute(
            '''SELECT x FROM accel_table WHERE user = ? ;''',
            ('side_to_side',)).fetchone()
        x = request['form']['x']
        x=make_list(x)
        forward_backward = resample(make_list(sub1[0]),len(x))
        forward = resample(make_list(sub2[0]),len(x))
        side_to_side = resample(make_list(sub3[0]),len(x))
        return('Forward_backwards:' + str(correlation(x,forward_backward)) + ' Forward: ' + str(correlation(x,forward))+' Side to Side: ' + str(correlation(x,side_to_side)))
